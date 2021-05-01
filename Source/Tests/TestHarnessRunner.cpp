/*
$info$
tags: Bin|TestHarnessRunner
desc: Used to run Assembly tests
$end_info$
*/

#include "Common/ArgumentLoader.h"
#include "Common/EnvironmentLoader.h"
#include "CommonCore/HostFactory.h"
#include "HarnessHelpers.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include "Tests/LinuxSyscalls/SignalDelegator.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CodeLoader.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/LogManager.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

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
  printf("[%s] %s\n", CharLevel, Message);
}

void AssertHandler(char const *Message) {
  printf("[ASSERT] %s\n", Message);
}

int main(int argc, char **argv, char **const envp) {
  LogMan::Throw::InstallHandler(AssertHandler);
  LogMan::Msg::InstallHandler(MsgHandler);
  FEXCore::Config::Initialize();
  FEXCore::Config::AddLayer(std::make_unique<FEX::ArgLoader::ArgLoader>(argc, argv));
  FEXCore::Config::AddLayer(std::make_unique<FEX::Config::EnvLoader>(envp));
  FEXCore::Config::Load();

  auto Args = FEX::ArgLoader::Get();

  LOGMAN_THROW_A(Args.size() > 1, "Not enough arguments");

  FEX::HarnessHelper::HarnessCodeLoader Loader{Args[0], Args[1].c_str()};
  FEXCore::Config::Set(FEXCore::Config::CONFIG_IS64BIT_MODE, Loader.Is64BitMode() ? "1" : "0");

  FEXCore::Context::InitializeStaticTables(Loader.Is64BitMode() ? FEXCore::Context::MODE_64BIT : FEXCore::Context::MODE_32BIT);
  auto CTX = FEXCore::Context::CreateNewContext();

  FEXCore::Context::SetCustomCPUBackendFactory(CTX, HostFactory::CPUCreationFactory);

  FEXCore::Context::InitializeContext(CTX);

  FEX::HLE::x32::MemAllocator *Allocator = nullptr;

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

    if (!Loader.MapMemory([Allocator](void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
      return Allocator->mmap(addr, length, prot, flags, fd, offset);
    }, [Allocator](void *addr, size_t length) {
      return Allocator->munmap(addr, length);
    })) {
      // failed to map
      LogMan::Msg::E("Failed to map 32-bit elf file.");
      return -ENOEXEC;
    }
  }

  std::unique_ptr<FEX::HLE::SignalDelegator> SignalDelegation = std::make_unique<FEX::HLE::SignalDelegator>();
  std::unique_ptr<FEX::HLE::SyscallHandler> SyscallHandler{
    Loader.Is64BitMode() ?
      FEX::HLE::x64::CreateHandler(CTX, SignalDelegation.get()) :
      FEX::HLE::x32::CreateHandler(CTX, SignalDelegation.get(), Allocator)
  };

  bool DidFault = false;
  SignalDelegation->RegisterFrontendHostSignalHandler(SIGSEGV, [&DidFault](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) {
      DidFault = true;
    return false;
  });

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

  LogMan::Msg::I("Faulted? %s", DidFault ? "Yes" : "No");
  LogMan::Msg::I("Passed? %s", Passed ? "Yes" : "No");

  FEXCore::Context::DestroyContext(CTX);

  return Passed ? 0 : -1;
}

