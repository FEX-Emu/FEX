/*
$info$
tags: Bin|TestHarnessRunner
desc: Used to run Assembly tests
$end_info$
*/

#include "Common/ArgumentLoader.h"
#include "CommonCore/HostFactory.h"
#include "HarnessHelpers.h"
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
  FEXCore::Config::Set(FEXCore::Config::CONFIG_IS64BIT_MODE, Loader.Is64BitMode() ? "1" : "0");

  FEXCore::Context::InitializeStaticTables(Loader.Is64BitMode() ? FEXCore::Context::MODE_64BIT : FEXCore::Context::MODE_32BIT);
  auto CTX = FEXCore::Context::CreateNewContext();

  FEXCore::Context::SetCustomCPUBackendFactory(CTX, HostFactory::CPUCreationFactory);

  FEXCore::Context::InitializeContext(CTX);

  std::unique_ptr<FEX::HLE::x32::MemAllocator> Allocator;

  if (Loader.Is64BitMode()) {
    if (!Loader.MapMemory([](void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
      return FEXCore::Allocator::mmap(addr, length, prot, flags, fd, offset);
    }, [](void *addr, size_t length) {
      return FEXCore::Allocator::munmap(addr, length);
    })) {
      // failed to map
      return -ENOEXEC;
    }
  } else {
    // Setup our userspace allocator
    uint32_t KernelVersion = FEX::HLE::SyscallHandler::CalculateHostKernelVersion();
    if (KernelVersion >= FEX::HLE::SyscallHandler::KernelVersion(4, 17)) {
      FEXCore::Allocator::SetupHooks();
    }

    Allocator = FEX::HLE::x32::CreateAllocator(KernelVersion < FEX::HLE::SyscallHandler::KernelVersion(4, 17));

    if (!Loader.MapMemory([&Allocator](void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
      return Allocator->mmap(addr, length, prot, flags, fd, offset);
    }, [&Allocator](void *addr, size_t length) {
      return Allocator->munmap(addr, length);
    })) {
      // failed to map
      LogMan::Msg::EFmt("Failed to map 32-bit elf file.");
      return -ENOEXEC;
    }
  }

  auto SignalDelegation = std::make_unique<FEX::HLE::SignalDelegator>();
  auto SyscallHandler = Loader.Is64BitMode() ? FEX::HLE::x64::CreateHandler(CTX, SignalDelegation.get())
                                             : FEX::HLE::x32::CreateHandler(CTX, SignalDelegation.get(), std::move(Allocator));

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

