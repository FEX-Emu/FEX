/*
$info$
tags: Bin|TestHarnessRunner
desc: Used to run Assembly tests
$end_info$
*/

#include "Common/ArgumentLoader.h"
#include "TestHarnessRunner/HostRunner.h"
#include "HarnessHelpers.h"
#include "Tests/LinuxSyscalls/LinuxAllocator.h"
#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include "Tests/LinuxSyscalls/SignalDelegator.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/Core/HostFeatures.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/vector.h>

#include <csetjmp>
#include <cstdint>
#include <errno.h>
#include <memory>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <vector>
#include <utility>

void MsgHandler(LogMan::DebugLevels Level, char const *Message) {
  const char *CharLevel{nullptr};

  switch (Level) {
  case LogMan::NONE:
    CharLevel = "NONE";
    break;
  case LogMan::ASSERT:
    CharLevel = "ASSERT";
    break;
  case LogMan::ERROR:
    CharLevel = "ERROR";
    break;
  case LogMan::DEBUG:
    CharLevel = "DEBUG";
    break;
  case LogMan::INFO:
    CharLevel = "Info";
    break;
  default:
    CharLevel = "???";
    break;
  }
  fmt::print("[{}] {}\n", CharLevel, Message);
}

void AssertHandler(char const *Message) {
  fmt::print("[ASSERT] {}\n", Message);

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
    std::unordered_map<std::string_view, std::string_view> EnvMap;
    for (auto &Option : Env) {
      std::string_view Key = Option.first;
      std::string_view Value = Option.second;

#define ENVLOADER
#include <FEXCore/Config/ConfigOptions.inl>

      EnvMap.insert_or_assign(Key, Value);
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

int main(int argc, char **argv, char **const envp) {
  LogMan::Throw::InstallHandler(AssertHandler);
  LogMan::Msg::InstallHandler(MsgHandler);
  FEXCore::Config::Initialize();
  FEXCore::Config::AddLayer(std::make_unique<FEX::ArgLoader::ArgLoader>(argc, argv));
  FEXCore::Config::AddLayer(FEXCore::Config::CreateEnvironmentLayer(envp));
  FEXCore::Config::Load();

  auto Args = FEX::ArgLoader::Get();

  if (Args.size() < 2) {
    LogMan::Msg::EFmt("Not enough arguments");
    return -1;
  }

  FEX::HarnessHelper::HarnessCodeLoader Loader{Args[0], Args[1].c_str()};

  // Adds in environment options from the test harness config
  FEXCore::Config::AddLayer(std::make_unique<TestEnvLoader>(Loader.GetEnvironmentOptions()));
  FEXCore::Config::ReloadMetaLayer();

  FEXCore::Config::Set(FEXCore::Config::CONFIG_IS64BIT_MODE, Loader.Is64BitMode() ? "1" : "0");

  FEX_CONFIG_OPT(Core, CORE);

  std::unique_ptr<FEX::HLE::MemAllocator> Allocator;

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

  auto SignalDelegation = std::make_unique<FEX::HLE::SignalDelegator>();
  bool DidFault = false;
  bool SupportsAVX = false;
  FEXCore::Core::CPUState State;

  FEXCore::Context::InitializeStaticTables(Loader.Is64BitMode() ? FEXCore::Context::MODE_64BIT : FEXCore::Context::MODE_32BIT);

  auto CTX = FEXCore::Context::Context::CreateNewContext();

  CTX->InitializeContext();

  // Skip any tests that the host doesn't support features for
  auto HostFeatures = CTX->GetHostFeatures();
  SupportsAVX = HostFeatures.SupportsAVX;

  bool TestUnsupported =
    (!HostFeatures.Supports3DNow && Loader.Requires3DNow()) ||
    (!HostFeatures.SupportsSSE4A && Loader.RequiresSSE4A()) ||
    (!SupportsAVX && Loader.RequiresAVX()) ||
    (!HostFeatures.SupportsRAND && Loader.RequiresRAND()) ||
    (!HostFeatures.SupportsSHA && Loader.RequiresSHA()) ||
    (!HostFeatures.SupportsCLZERO && Loader.RequiresCLZERO()) ||
    (!HostFeatures.SupportsBMI1 && Loader.RequiresBMI1()) ||
    (!HostFeatures.SupportsBMI2 && Loader.RequiresBMI2()) ||
    (!HostFeatures.SupportsCLWB && Loader.RequiresCLWB());

  if (TestUnsupported) {
    FEXCore::Context::Context::DestroyContext(CTX);
    return 0;
  }

  if (Core != FEXCore::Config::CONFIG_CUSTOM) {
    jmp_buf LongJump{};
    int LongJumpVal{};

    SignalDelegation->RegisterFrontendHostSignalHandler(SIGSEGV, [&DidFault, &LongJump](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) {
      constexpr uint8_t HLT = 0xF4;
      if (reinterpret_cast<uint8_t*>(Thread->CurrentFrame->State.rip)[0] != HLT) {
        DidFault = false;
        return false;
      }

      longjmp(LongJump, 1);
      return false;
    }, true);

    // Run through FEX
    auto SyscallHandler = Loader.Is64BitMode() ? FEX::HLE::x64::CreateHandler(CTX, SignalDelegation.get())
                                               : FEX::HLE::x32::CreateHandler(CTX, SignalDelegation.get(), std::move(Allocator));

    auto Mapper = std::bind_front(&FEX::HLE::SyscallHandler::GuestMmap, SyscallHandler.get());
    auto Unmapper = std::bind_front(&FEX::HLE::SyscallHandler::GuestMunmap, SyscallHandler.get());

    if (!Loader.MapMemory(Mapper, Unmapper)) {
      // failed to map
      LogMan::Msg::EFmt("Failed to map %d-bit elf file.", Loader.Is64BitMode() ? 64 : 32);
      return -ENOEXEC;
    }

    CTX->SetSignalDelegator(SignalDelegation.get());
    CTX->SetSyscallHandler(SyscallHandler.get());

    bool Result1 = CTX->InitCore(Loader.DefaultRIP(), Loader.GetStackPointer());

    if (!Result1) {
      return 1;
    }

    LongJumpVal = setjmp(LongJump);
    if (!LongJumpVal) {
      CTX->RunUntilExit();
    }

    // Just re-use compare state. It also checks against the expected values in config.
    CTX->GetCPUState(&State);

    SyscallHandler.reset();
  } else {
    // Run as host
    SupportsAVX = true;
    SignalDelegation->RegisterTLSState((FEXCore::Core::InternalThreadState*)UINTPTR_MAX);
    if (!Loader.MapMemory(mmap, munmap)) {
      // failed to map
      LogMan::Msg::EFmt("Failed to map %d-bit elf file.", Loader.Is64BitMode() ? 64 : 32);
      return -ENOEXEC;
    }

    RunAsHost(SignalDelegation, Loader.DefaultRIP(), Loader.GetStackPointer(), &State);
  }

  FEXCore::Context::Context::DestroyContext(CTX);
  FEXCore::Context::ShutdownStaticTables();

  bool Passed = !DidFault && Loader.CompareStates(&State, nullptr, SupportsAVX);

  LogMan::Msg::IFmt("Faulted? {}", DidFault ? "Yes" : "No");
  LogMan::Msg::IFmt("Passed? {}", Passed ? "Yes" : "No");


  SignalDelegation.reset();

  FEXCore::Config::Shutdown();

  LogMan::Throw::UnInstallHandlers();
  LogMan::Msg::UnInstallHandlers();

  FEXCore::Allocator::ClearHooks();

  return Passed ? 0 : -1;
}

