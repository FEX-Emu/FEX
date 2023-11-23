// SPDX-License-Identifier: MIT
/*
$info$
tags: Bin|TestHarnessRunner
desc: Used to run Assembly tests
$end_info$
*/

#ifdef _WIN32
#include "DummyHandlers.h"
#include "ArchHelpers/WinContext.h"
#else
#include "LinuxSyscalls/LinuxAllocator.h"
#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x32/Syscalls.h"
#include "LinuxSyscalls/x64/Syscalls.h"
#include "LinuxSyscalls/SignalDelegator.h"
#endif

#include "Common/ArgumentLoader.h"
#include "TestHarnessRunner/HostRunner.h"
#include "HarnessHelpers.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/Core/HostFeatures.h>
#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/ArchHelpers/Arm64.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>

#include <csetjmp>
#include <cstdint>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <utility>

void MsgHandler(LogMan::DebugLevels Level, char const *Message) {
  fextl::fmt::print("[{}] {}\n", LogMan::DebugLevelStr(Level), Message);
}

void AssertHandler(char const *Message) {
  fextl::fmt::print("[ASSERT] {}\n", Message);

  // make sure buffers are flushed
  fflush(nullptr);
}

namespace {
static const fextl::vector<std::pair<const char*, FEXCore::Config::ConfigOption>> EnvConfigLookup = {{
#define OPT_BASE(type, group, enum, json, default) {"FEX_" #enum, FEXCore::Config::ConfigOption::CONFIG_##enum},
#include <FEXCore/Config/ConfigValues.inl>
}};

// Claims to be a local application config layer
class TestEnvLoader final : public FEXCore::Config::Layer {
public:
  explicit TestEnvLoader(fextl::vector<std::pair<std::string_view, std::string_view>> _Env)
    : FEXCore::Config::Layer(FEXCore::Config::LayerType::LAYER_LOCAL_APP)
    , Env {std::move(_Env)} {
    Load();
  }

  void Load() override {
    fextl::unordered_map<std::string_view, std::string> EnvMap;
    for (auto &Option : Env) {
      std::string_view Key = Option.first;
      std::string_view Value_View = Option.second;
      std::optional<fextl::string> Value;

#define ENVLOADER
#include <FEXCore/Config/ConfigOptions.inl>

      if (Value) {
        EnvMap.insert_or_assign(Key, *Value);
      }
      else {
        EnvMap.insert_or_assign(Key, Value_View);
      }
    }

    auto GetVar = [&](const std::string_view id) -> std::optional<std::string_view> {
      const auto it = EnvMap.find(id);
      if (it == EnvMap.end())
        return std::nullopt;

      return it->second;
    };

    for (auto &it : EnvConfigLookup) {
      if (auto Value = GetVar(it.first); Value) {
        Set(it.second, *Value);
      }
    }
  }

private:
  fextl::vector<std::pair<std::string_view, std::string_view>> Env;
};
}

namespace LongJumpHandler {
  static jmp_buf LongJump{};
  static bool DidFault{};

#ifndef _WIN32
  void RegisterLongJumpHandler(FEX::HLE::SignalDelegator *Handler) {
    Handler->RegisterFrontendHostSignalHandler(SIGSEGV, [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) {
      constexpr uint8_t HLT = 0xF4;
      if (reinterpret_cast<uint8_t*>(Thread->CurrentFrame->State.rip)[0] != HLT) {
        DidFault = true;
        return false;
      }

      longjmp(LongJumpHandler::LongJump, 1);
      return false;
    }, true);
  }
#else
  FEX::DummyHandlers::DummySignalDelegator *Handler;

  static void LongJumpHandler() {
    longjmp(LongJump, 1);
  }

  LONG WINAPI
  VectoredExceptionHandler(struct _EXCEPTION_POINTERS *ExceptionInfo) {
    auto Thread = Handler->GetBackingTLSThread();
    PCONTEXT Context;
    Context = ExceptionInfo->ContextRecord;

    switch (ExceptionInfo->ExceptionRecord->ExceptionCode) {
      case STATUS_DATATYPE_MISALIGNMENT: {
        const auto PC = FEX::ArchHelpers::Context::GetPc(Context);
        if (!Thread->CPUBackend->IsAddressInCodeBuffer(PC)) {
          // Wasn't a sigbus in JIT code
          return EXCEPTION_CONTINUE_SEARCH;
        }

        const auto Result = FEXCore::ArchHelpers::Arm64::HandleUnalignedAccess(true, PC, FEX::ArchHelpers::Context::GetArmGPRs(Context));
        FEX::ArchHelpers::Context::SetPc(Context, PC + Result.second);
        return Result.first ?
          EXCEPTION_CONTINUE_EXECUTION :
          EXCEPTION_CONTINUE_SEARCH;
      }
      case STATUS_ACCESS_VIOLATION: {
        constexpr uint8_t HLT = 0xF4;
        if (reinterpret_cast<uint8_t*>(Thread->CurrentFrame->State.rip)[0] != HLT) {
          DidFault = true;
          return EXCEPTION_CONTINUE_SEARCH;
        }

        FEX::ArchHelpers::Context::SetPc(Context, reinterpret_cast<uint64_t>(LongJumpHandler));
        return EXCEPTION_CONTINUE_EXECUTION;
      }
      default: break;
    }

    printf("!Fault!\n");
    printf("\tExceptionCode: 0x%lx\n", ExceptionInfo->ExceptionRecord->ExceptionCode);
    printf("\tExceptionFlags: 0x%lx\n", ExceptionInfo->ExceptionRecord->ExceptionFlags);
    printf("\tExceptionRecord: 0x%p\n", ExceptionInfo->ExceptionRecord->ExceptionRecord);
    printf("\tExceptionAddress: 0x%p\n", ExceptionInfo->ExceptionRecord->ExceptionAddress);
    printf("\tNumberParameters: 0x%lx\n", ExceptionInfo->ExceptionRecord->NumberParameters);

    return EXCEPTION_CONTINUE_SEARCH;
  }

  void RegisterLongJumpHandler(FEX::DummyHandlers::DummySignalDelegator *Handler) {
    // Install VEH handler.
    AddVectoredExceptionHandler(0, VectoredExceptionHandler);

    LongJumpHandler::Handler = Handler;
  }
#endif
}

int main(int argc, char **argv, char **const envp) {
#ifndef _WIN32
  auto SBRKPointer = FEXCore::Allocator::DisableSBRKAllocations();
#endif
  FEXCore::Allocator::GLIBCScopedFault GLIBFaultScope;
  LogMan::Throw::InstallHandler(AssertHandler);
  LogMan::Msg::InstallHandler(MsgHandler);

  FEX::Config::InitializeConfigs();
  FEXCore::Config::Initialize();
  FEXCore::Config::AddLayer(fextl::make_unique<FEX::ArgLoader::ArgLoader>(argc, argv));
  FEXCore::Config::AddLayer(FEX::Config::CreateEnvironmentLayer(envp));
  FEXCore::Config::Load();

  auto Args = FEX::ArgLoader::Get();

  if (Args.size() < 2) {
    LogMan::Msg::EFmt("Not enough arguments");
    return -1;
  }

  FEX::HarnessHelper::HarnessCodeLoader Loader{Args[0], Args[1]};

  // Adds in environment options from the test harness config
  FEXCore::Config::AddLayer(fextl::make_unique<TestEnvLoader>(Loader.GetEnvironmentOptions()));
  FEXCore::Config::ReloadMetaLayer();

  FEXCore::Config::Set(FEXCore::Config::CONFIG_IS64BIT_MODE, Loader.Is64BitMode() ? "1" : "0");
#ifdef VIXL_SIMULATOR
  // If running under the vixl simulator, ensure that indirect runtime calls are enabled.
  FEXCore::Config::EraseSet(FEXCore::Config::CONFIG_DISABLE_VIXL_INDIRECT_RUNTIME_CALLS, "0");
#endif

  FEX_CONFIG_OPT(Core, CORE);

#ifndef _WIN32
  fextl::unique_ptr<FEX::HLE::MemAllocator> Allocator;

  if (!Loader.Is64BitMode()) {
    // Setup our userspace allocator
    uint32_t KernelVersion = FEX::HLE::SyscallHandler::CalculateHostKernelVersion();
    if (KernelVersion >= FEX::HLE::SyscallHandler::KernelVersion(4, 17)) {
      FEXCore::Allocator::SetupHooks();
    }

    if (KernelVersion < FEX::HLE::SyscallHandler::KernelVersion(4, 17)) {
      Allocator = FEX::HLE::Create32BitAllocator();
    }
    else {
      Allocator = FEX::HLE::CreatePassthroughAllocator();
    }
  }
#endif

  bool SupportsAVX = false;
  bool SupportsAVX2 = false;
  FEXCore::Core::CPUState State;

  FEXCore::Context::InitializeStaticTables(Loader.Is64BitMode() ? FEXCore::Context::MODE_64BIT : FEXCore::Context::MODE_32BIT);

  auto CTX = FEXCore::Context::Context::CreateNewContext();

  CTX->InitializeContext();

#ifndef _WIN32
  auto SignalDelegation = FEX::HLE::CreateSignalDelegator(CTX.get(), {});
#else
  // Enable exit on HLT while Wine's longjump is broken.
  //
  // Once they fix longjump, we can remove this.
  CTX->EnableExitOnHLT();
  auto SignalDelegation = FEX::WindowsHandlers::CreateSignalDelegator();
#endif

  // Skip any tests that the host doesn't support features for
  auto HostFeatures = CTX->GetHostFeatures();
  SupportsAVX = HostFeatures.SupportsAVX;
  SupportsAVX2 = HostFeatures.SupportsAVX2;

  bool TestUnsupported =
    (!HostFeatures.Supports3DNow && Loader.Requires3DNow()) ||
    (!HostFeatures.SupportsSSE4A && Loader.RequiresSSE4A()) ||
    (!SupportsAVX && Loader.RequiresAVX()) ||
    (!SupportsAVX2 && Loader.RequiresAVX2()) ||
    (!HostFeatures.SupportsRAND && Loader.RequiresRAND()) ||
    (!HostFeatures.SupportsSHA && Loader.RequiresSHA()) ||
    (!HostFeatures.SupportsCLZERO && Loader.RequiresCLZERO()) ||
    (!HostFeatures.SupportsBMI1 && Loader.RequiresBMI1()) ||
    (!HostFeatures.SupportsBMI2 && Loader.RequiresBMI2()) ||
    (!HostFeatures.SupportsCLWB && Loader.RequiresCLWB());

#ifdef _WIN32
    TestUnsupported |= Loader.RequiresLinux();
#endif

  if (TestUnsupported) {
    return 0;
  }

  if (Core != FEXCore::Config::CONFIG_CUSTOM) {
#ifndef _WIN32
    auto SyscallHandler = Loader.Is64BitMode() ? FEX::HLE::x64::CreateHandler(CTX.get(), SignalDelegation.get())
                                               : FEX::HLE::x32::CreateHandler(CTX.get(), SignalDelegation.get(), std::move(Allocator));

#else
    auto SyscallHandler = FEX::WindowsHandlers::CreateSyscallHandler();
#endif

    LongJumpHandler::RegisterLongJumpHandler(SignalDelegation.get());

    // Run through FEX
    if (!Loader.MapMemory()) {
      // failed to map
      LogMan::Msg::EFmt("Failed to map %d-bit elf file.", Loader.Is64BitMode() ? 64 : 32);
      return -ENOEXEC;
    }

    CTX->SetSignalDelegator(SignalDelegation.get());
    CTX->SetSyscallHandler(SyscallHandler.get());

    bool Result1 = CTX->InitCore();
    auto ParentThread = CTX->CreateThread(Loader.DefaultRIP(), Loader.GetStackPointer());
    ParentThread->DestroyedByParent = true;

    if (!Result1) {
      return 1;
    }

    int LongJumpVal = setjmp(LongJumpHandler::LongJump);
    if (!LongJumpVal) {
      CTX->RunUntilExit(ParentThread);
    }

    // Just re-use compare state. It also checks against the expected values in config.
    CTX->GetCPUState(ParentThread, &State);

    SyscallHandler.reset();
    CTX->DestroyThread(ParentThread);
  }
#ifndef _WIN32
  else {
    // Run as host
    SupportsAVX = true;
    SignalDelegation->RegisterTLSState((FEXCore::Core::InternalThreadState*)UINTPTR_MAX);
    if (!Loader.MapMemory()) {
      // failed to map
      LogMan::Msg::EFmt("Failed to map %d-bit elf file.", Loader.Is64BitMode() ? 64 : 32);
      return -ENOEXEC;
    }

    RunAsHost(SignalDelegation, Loader.DefaultRIP(), Loader.GetStackPointer(), &State);
  }
#endif

  bool Passed = !LongJumpHandler::DidFault && Loader.CompareStates(&State, nullptr, SupportsAVX);

  LogMan::Msg::IFmt("Faulted? {}", LongJumpHandler::DidFault ? "Yes" : "No");
  LogMan::Msg::IFmt("Passed? {}", Passed ? "Yes" : "No");


  SignalDelegation.reset();

  FEXCore::Config::Shutdown();

  LogMan::Throw::UnInstallHandlers();
  LogMan::Msg::UnInstallHandlers();

#ifndef _WIN32
  FEXCore::Allocator::ClearHooks();

  FEXCore::Allocator::ReenableSBRKAllocations(SBRKPointer);
#endif

  return Passed ? 0 : -1;
}

