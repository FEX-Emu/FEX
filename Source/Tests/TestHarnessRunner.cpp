/*
$info$
tags: Bin|TestHarnessRunner
desc: Used to run Assembly tests
$end_info$
*/

#include "Common/ArgumentLoader.h"
#include "CommonCore/HostFactory.h"
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
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/LogManager.h>

#include <cstdint>
#include <errno.h>
#include <memory>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <vector>
#include <utility>

namespace FEXCore::Core {
  struct InternalThreadState;
}

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
static const std::vector<std::pair<const char*, FEXCore::Config::ConfigOption>> EnvConfigLookup = {{
#define OPT_BASE(type, group, enum, json, default) {"FEX_" #enum, FEXCore::Config::ConfigOption::CONFIG_##enum},
#include <FEXCore/Config/ConfigValues.inl>
}};

// Claims to be a local application config layer
class TestEnvLoader final : public FEXCore::Config::Layer {
public:
  explicit TestEnvLoader(std::vector<std::pair<std::string_view, std::string_view>> _Env)
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
  std::vector<std::pair<std::string_view, std::string_view>> Env;
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

  FEXCore::Context::InitializeStaticTables(Loader.Is64BitMode() ? FEXCore::Context::MODE_64BIT : FEXCore::Context::MODE_32BIT);
  auto CTX = FEXCore::Context::CreateNewContext();

  FEXCore::Context::SetCustomCPUBackendFactory(CTX, HostFactory::CPUCreationFactory);

  FEXCore::Context::InitializeContext(CTX);

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
  auto SyscallHandler = Loader.Is64BitMode() ? FEX::HLE::x64::CreateHandler(CTX, SignalDelegation.get())
                                             : FEX::HLE::x32::CreateHandler(CTX, SignalDelegation.get(), std::move(Allocator));

  auto Mapper = [&SyscallHandler](auto && ...args){ return SyscallHandler->GuestMmap(args...); };
  auto Unmapper = [&SyscallHandler](auto && ...args){ return SyscallHandler->GuestMunmap(args...); };

  if (!Loader.MapMemory(Mapper, Unmapper)) {
    // failed to map
    LogMan::Msg::EFmt("Failed to map %d-bit elf file.", Loader.Is64BitMode() ? 64 : 32);
    return -ENOEXEC;
  }

  bool DidFault = false;
  SignalDelegation->RegisterFrontendHostSignalHandler(SIGSEGV, [&DidFault](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) {
      DidFault = true;
    return false;
  }, true);

  FEXCore::Context::SetSignalDelegator(CTX, SignalDelegation.get());
  FEXCore::Context::SetSyscallHandler(CTX, SyscallHandler.get());
  bool Result1 = FEXCore::Context::InitCore(CTX, &Loader);

  if (!Result1)
    return 1;

  FEXCore::Context::RunUntilExit(CTX);

  // Just re-use compare state. It also checks against the expected values in config.
  FEXCore::Core::CPUState State;
  FEXCore::Context::GetCPUState(CTX, &State);
  bool Passed = !DidFault && Loader.CompareStates(&State, nullptr);

  LogMan::Msg::IFmt("Faulted? {}", DidFault ? "Yes" : "No");
  LogMan::Msg::IFmt("Passed? {}", Passed ? "Yes" : "No");

  SyscallHandler.reset();
  SignalDelegation.reset();

  FEXCore::Context::DestroyContext(CTX);
  FEXCore::Context::ShutdownStaticTables();

  FEXCore::Config::Shutdown();

  LogMan::Throw::UnInstallHandlers();
  LogMan::Msg::UnInstallHandlers();

  FEXCore::Allocator::ClearHooks();

  return Passed ? 0 : -1;
}

