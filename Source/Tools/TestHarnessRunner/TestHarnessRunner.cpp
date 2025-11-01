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

#include "Common/HostFeatures.h"
#include "HarnessHelpers.h"
#include "TestHarnessRunner/HostRunner.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/HostFeatures.h>
#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/ArchHelpers/Arm64.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>
#include <FEXHeaderUtils/Filesystem.h>

#include <csetjmp>
#include <cstdint>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <utility>

#ifdef _M_X86_64
#include "Common/X86Features.h"
#endif

void MsgHandler(LogMan::DebugLevels Level, const char* Message) {
  fextl::fmt::print("{} {}\n", LogMan::DebugLevelStr(Level), Message);
}

void AssertHandler(const char* Message) {
  fextl::fmt::print("A {}\n", Message);

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
    for (auto& Option : Env) {
      std::string_view Key = Option.first;
      std::string_view Value_View = Option.second;
      std::optional<fextl::string> Value;

#define ENVLOADER
#include <FEXCore/Config/ConfigOptions.inl>

      if (Value) {
        EnvMap.insert_or_assign(Key, *Value);
      } else {
        EnvMap.insert_or_assign(Key, Value_View);
      }
    }

    auto GetVar = [&](const std::string_view id) -> std::optional<std::string_view> {
      const auto it = EnvMap.find(id);
      if (it == EnvMap.end()) {
        return std::nullopt;
      }

      return it->second;
    };

    for (auto& it : EnvConfigLookup) {
      if (auto Value = GetVar(it.first); Value) {
        Set(it.second, *Value);
      }
    }
  }

private:
  fextl::vector<std::pair<std::string_view, std::string_view>> Env;
};
} // namespace

namespace LongJumpHandler {
static jmp_buf LongJump {};
static bool DidFault {};

#ifndef _WIN32
void RegisterLongJumpHandler(FEX::HLE::SignalDelegator* Handler) {
  Handler->RegisterFrontendHostSignalHandler(
    SIGSEGV,
    [](FEXCore::Core::InternalThreadState* Thread, int Signal, void* info, void* ucontext) {
      constexpr uint8_t HLT = 0xF4;
      if (reinterpret_cast<uint8_t*>(Thread->CurrentFrame->State.rip)[0] != HLT) {
        DidFault = true;
        return false;
      }

      longjmp(LongJumpHandler::LongJump, 1);
      return false;
    },
    true);
}
#else
FEX::DummyHandlers::DummySignalDelegator* Handler;

static void LongJumpHandler() {
  longjmp(LongJump, 1);
}

LONG WINAPI VectoredExceptionHandler(struct _EXCEPTION_POINTERS* ExceptionInfo) {
  auto Thread = Handler->GetBackingTLSThread();
  PCONTEXT Context;
  Context = ExceptionInfo->ContextRecord;

  switch (ExceptionInfo->ExceptionRecord->ExceptionCode) {
  case STATUS_DATATYPE_MISALIGNMENT: {
    const auto PC = FEX::ArchHelpers::Context::GetPc(Context);
    if (!Thread->CTX->IsAddressInCodeBuffer(Thread, PC)) {
      // Wasn't a sigbus in JIT code
      return EXCEPTION_CONTINUE_SEARCH;
    }

    const auto Result = FEXCore::ArchHelpers::Arm64::HandleUnalignedAccess(true, PC, FEX::ArchHelpers::Context::GetArmGPRs(Context));
    FEX::ArchHelpers::Context::SetPc(Context, PC + Result.value_or(0));
    return Result ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_CONTINUE_SEARCH;
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

void RegisterLongJumpHandler(FEX::DummyHandlers::DummySignalDelegator* Handler) {
  // Install VEH handler.
  AddVectoredExceptionHandler(0, VectoredExceptionHandler);

  LongJumpHandler::Handler = Handler;
}
#endif
} // namespace LongJumpHandler

int main(int argc, char** argv, char** const envp) {
#ifndef _WIN32
  auto SBRKPointer = FEXCore::Allocator::DisableSBRKAllocations();
#endif
  FEXCore::Allocator::GLIBCScopedFault GLIBFaultScope;
  LogMan::Throw::InstallHandler(AssertHandler);
  LogMan::Msg::InstallHandler(MsgHandler);

  FEX::Config::InitializeConfigs(FEX::Config::PortableInformation {});
  FEXCore::Config::Initialize();
  FEXCore::Config::AddLayer(FEX::Config::CreateEnvironmentLayer(envp));
  FEXCore::Config::Load();

  if (argc < 3) {
    LogMan::Msg::EFmt("Not enough arguments");
    return -1;
  }

  auto Filename = argv[1];
  auto ConfigFile = argv[2];

  if (!FHU::Filesystem::Exists(Filename)) {
    LogMan::Msg::EFmt("File {} does not exist", Filename);
    return -1;
  }

  if (!FHU::Filesystem::Exists(ConfigFile)) {
    LogMan::Msg::EFmt("File {} does not exist", ConfigFile);
    return -1;
  }

  FEX::HarnessHelper::HarnessCodeLoader Loader {Filename, ConfigFile};

  // Adds in environment options from the test harness config
  FEXCore::Config::AddLayer(fextl::make_unique<TestEnvLoader>(Loader.GetEnvironmentOptions()));
  FEXCore::Config::ReloadMetaLayer();

  FEXCore::Config::Set(FEXCore::Config::CONFIG_IS64BIT_MODE, Loader.Is64BitMode() ? "1" : "0");
#ifdef VIXL_SIMULATOR
  // If running under the vixl simulator, ensure that indirect runtime calls are enabled.
  FEXCore::Config::Set(FEXCore::Config::CONFIG_DISABLE_VIXL_INDIRECT_RUNTIME_CALLS, "0");
#endif

#ifndef _WIN32
  fextl::unique_ptr<FEX::HLE::MemAllocator> Allocator;

  if (!Loader.Is64BitMode()) {
    // Setup our userspace allocator
    FEXCore::Allocator::SetupHooks();
    Allocator = FEX::HLE::CreatePassthroughAllocator();
  }
#endif

  bool SupportsAVX = false;
  FEXCore::Core::CPUState State;

  auto HostFeatures = FEX::FetchHostFeatures();
  auto CTX = FEXCore::Context::Context::CreateNewContext(HostFeatures);

#ifndef _WIN32
  auto SignalDelegation = FEX::HLE::CreateSignalDelegator(CTX.get(), {}, HostFeatures.SupportsAVX);
#else
  // Enable exit on HLT while Wine's longjump is broken.
  //
  // Once they fix longjump, we can remove this.
  CTX->EnableExitOnHLT();
  auto SignalDelegation = FEX::WindowsHandlers::CreateSignalDelegator();
#endif

  // Skip any tests that the host doesn't support features for
  SupportsAVX = HostFeatures.SupportsAVX;

  bool TestUnsupported = (!SupportsAVX && Loader.RequiresAVX()) || (!HostFeatures.SupportsRAND && Loader.RequiresRAND()) ||
                         (!HostFeatures.SupportsSHA && Loader.RequiresSHA()) || (!HostFeatures.SupportsCLZERO && Loader.RequiresCLZERO()) ||
                         (!HostFeatures.SupportsAES256 && Loader.RequiresAES256()) || (!HostFeatures.SupportsAFP && Loader.RequiresAFP());


  bool IsHostRunner = false;
#if !defined(VIXL_SIMULATOR) && defined(_M_X86_64)
  IsHostRunner = true;
  ///< Features that are only unsupported when running using the HostRunner and the CI machine doesn't support the feature getting tested.
  FEX::X86::Features Feature {};
  const bool Supports3DNow = Feature.Feat_3dnow;
  const bool SupportsSSE4A = Feature.Feat_sse4a;
  const bool SupportsBMI1 = Feature.Feat_bmi1;
  const bool SupportsBMI2 = Feature.Feat_bmi2;
  const bool SupportsCLWB = Feature.Feat_clwb;
  const bool SupportsSSSE3 = Feature.Feat_ssse3;
  const bool SupportsSSE4_1 = Feature.Feat_sse4_1;
  const bool SupportsSSE4_2 = Feature.Feat_sse4_2;
  const bool SupportsAES = Feature.Feat_aes;
  const bool SupportsPCLMUL = Feature.Feat_pclmulqdq;
  const bool SupportsMOVBE = Feature.Feat_movbe;
  const bool SupportsADX = Feature.Feat_adx;
  const bool SupportsXSAVE = Feature.Feat_xsave;
  const bool SupportsRDPID = Feature.Feat_rdpid;
  const bool SupportsCLFLOPT = Feature.Feat_clflopt;
  const bool SupportsFSGSBase = Feature.Feat_fsgsbase;

  TestUnsupported |=
    (!Supports3DNow && Loader.Requires3DNow()) || (!SupportsSSE4A && Loader.RequiresSSE4A()) || (!SupportsBMI1 && Loader.RequiresBMI1()) ||
    (!SupportsBMI2 && Loader.RequiresBMI2()) || (!SupportsCLWB && Loader.RequiresCLWB()) || (!SupportsSSSE3 && Loader.RequiresSSSE3()) ||
    (!SupportsSSE4_1 && Loader.RequiresSSE4_1()) || (!SupportsSSE4_2 && Loader.RequiresSSE4_2()) ||
    (!SupportsAES && Loader.RequiresAES()) || (!SupportsPCLMUL && Loader.RequiresPCLMUL()) || (!SupportsMOVBE && Loader.RequiresMOVBE()) ||
    (!SupportsADX && Loader.RequiresADX()) || (!SupportsXSAVE && Loader.RequiresXSAVE()) || (!SupportsRDPID && Loader.RequiresRDPID()) ||
    (!SupportsCLFLOPT && Loader.RequiresCLFLOPT()) || (!SupportsFSGSBase && Loader.RequiresFSGSBase()) || Loader.RequiresEMMI();
#endif

#ifdef _WIN32
  TestUnsupported |= Loader.RequiresLinux();
#endif

  if (TestUnsupported) {
    return 0;
  }

#ifndef _WIN32
  auto SyscallHandler = Loader.Is64BitMode() ? FEX::HLE::x64::CreateHandler(CTX.get(), SignalDelegation.get(), nullptr) :
                                               FEX::HLE::x32::CreateHandler(CTX.get(), SignalDelegation.get(), nullptr, std::move(Allocator));

  auto DoMmap = [&](uint64_t Address, size_t Size) -> void* {
    void* Result = SyscallHandler->GuestMmap(nullptr, (void*)Address, Size, PROT_READ | PROT_WRITE | PROT_EXEC,
                                             MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
    LOGMAN_THROW_A_FMT(Result == reinterpret_cast<void*>(Address), "Map Memory mmap failed");
    return Result;
  };

#else
  auto SyscallHandler = FEX::WindowsHandlers::CreateSyscallHandler();

  auto DoMMap = [](uint64_t Address, size_t Size) -> void* {
    void* Result = FEXCore::Allocator::VirtualAlloc(reinterpret_cast<void*>(Address), Size, true);
    LOGMAN_THROW_A_FMT(Result == reinterpret_cast<void*>(Address), "Map Memory mmap failed");
    return Result;
  };
#endif

  CTX->SetSignalDelegator(SignalDelegation.get());
  CTX->SetSyscallHandler(SyscallHandler.get());

  if (!CTX->InitCore()) {
    return 1;
  }

  if (!IsHostRunner) {
    LongJumpHandler::RegisterLongJumpHandler(SignalDelegation.get());

    // Run through FEX
    if (!Loader.MapMemory(DoMmap)) {
      // failed to map
      LogMan::Msg::EFmt("Failed to map {}-bit elf file.", Loader.Is64BitMode() ? 64 : 32);
      return -ENOEXEC;
    }

    auto ParentThread = SyscallHandler->TM.CreateThread(Loader.DefaultRIP(), 0);
    SyscallHandler->TM.TrackThread(ParentThread);
    SignalDelegation->RegisterTLSState(ParentThread);

    if (!ParentThread) {
      return 1;
    }

    int LongJumpVal = setjmp(LongJumpHandler::LongJump);
    if (!LongJumpVal) {
      CTX->ExecuteThread(ParentThread->Thread);
    }

    // Just re-use compare state. It also checks against the expected values in config.
    memcpy(&State, &ParentThread->Thread->CurrentFrame->State, sizeof(State));

    __uint128_t XMM_Low[FEXCore::Core::CPUState::NUM_XMMS];
    if (SupportsAVX) {
      ///< Reconstruct the XMM registers even if they are in split view, then remerge them.
      __uint128_t YMM_High[FEXCore::Core::CPUState::NUM_XMMS];
      CTX->ReconstructXMMRegisters(ParentThread->Thread, XMM_Low, YMM_High);
      for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_XMMS; ++i) {
        memcpy(&State.xmm.avx.data[i][0], &XMM_Low[i], sizeof(__uint128_t));
        memcpy(&State.xmm.avx.data[i][2], &YMM_High[i], sizeof(__uint128_t));
      }
    } else {
      CTX->ReconstructXMMRegisters(ParentThread->Thread, reinterpret_cast<__uint128_t*>(State.xmm.sse.data), nullptr);
    }

    SignalDelegation->UninstallTLSState(ParentThread);
    FEX::HLE::_SyscallHandler->TM.DestroyThread(ParentThread, true);
  }
#ifndef _WIN32
  else {
    // Run as host
    SupportsAVX = true;
    auto ParentThread = SyscallHandler->TM.CreateThread(Loader.DefaultRIP(), 0);
    SyscallHandler->TM.TrackThread(ParentThread);
    SignalDelegation->RegisterTLSState(ParentThread);

    auto DoMmap = [&](uint64_t Address, size_t Size) -> void* {
      void* Result = mmap((void*)Address, Size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
      LOGMAN_THROW_A_FMT(Result == reinterpret_cast<void*>(Address), "Map Memory mmap failed");
      return Result;
    };
    if (!Loader.MapMemory(DoMmap)) {
      // failed to map
      LogMan::Msg::EFmt("Failed to map {}-bit elf file.", Loader.Is64BitMode() ? 64 : 32);
      return -ENOEXEC;
    }

    RunAsHost(SignalDelegation, Loader.DefaultRIP(), &State);
    SignalDelegation->UninstallTLSState(ParentThread);
    FEX::HLE::_SyscallHandler->TM.DestroyThread(ParentThread, true);
  }
#endif

  SyscallHandler.reset();

  bool Passed = !LongJumpHandler::DidFault && Loader.CompareStates(&State, nullptr, SupportsAVX);

  LogMan::Msg::IFmt("Faulted? {}", LongJumpHandler::DidFault ? "Yes" : "No");
  LogMan::Msg::IFmt("Passed? {}", Passed ? "Yes" : "No");


  SignalDelegation.reset();

  FEXCore::Config::Shutdown();

  LogMan::Throw::UnInstallHandler();
  LogMan::Msg::UnInstallHandler();

#ifndef _WIN32
  FEXCore::Allocator::ClearHooks();

  FEXCore::Allocator::ReenableSBRKAllocations(SBRKPointer);
#endif

  return Passed ? 0 : -1;
}
