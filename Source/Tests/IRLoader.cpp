/*
$info$
tags: Bin|IRLoader
desc: Used to run IR Tests
$end_info$
*/

#include "Common/ArgumentLoader.h"
#include "IRLoader/Loader.h"
#include "Tests/LinuxSyscalls/SignalDelegator.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CodeLoader.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/HLE/SyscallHandler.h>

#include <functional>
#include <memory>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <vector>

#include <fmt/format.h>

namespace FEXCore::Context {
  struct Context;
}

void MsgHandler(LogMan::DebugLevels Level, char const *Message)
{
  const char *CharLevel{nullptr};

  switch (Level)
  {
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
  case LogMan::STDOUT:
    CharLevel = "STDOUT";
    break;
  case LogMan::STDERR:
    CharLevel = "STDERR";
    break;
  default:
    CharLevel = "???";
    break;
  }

  fmt::print("[{}] {}\n", CharLevel, Message);
  fflush(stdout);
}

void AssertHandler(char const *Message)
{
  fmt::print("[ASSERT] {}\n", Message);
  fflush(stdout);
}

class IRCodeLoader final : public FEXCore::CodeLoader
{
public:
  IRCodeLoader(FEX::IRLoader::Loader *Loader)
      : IR{Loader}
  {
  }

  uint64_t StackSize() const override
  {
    return STACK_SIZE;
  }

  uint64_t GetStackPointer() override
  {
    return reinterpret_cast<uint64_t>(FEXCore::Allocator::mmap(nullptr, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
  }

  uint64_t DefaultRIP() const override
  {
    return IR->GetEntryRIP();
  }

  bool MapMemory(const MapperFn& Mapper, const UnmapperFn& Unmapper) override
  {
    // Map the memory regions the test file asks for
    IR->MapRegions();

    LoadMemory();

    return true;
  }

  void LoadMemory()
  {
    IR->LoadMemory();
  }

  virtual void AddIR(IRHandler Handler) override
  {
    Handler(IR->GetEntryRIP(), IR->GetIREmitter());
  }

private:
  FEX::IRLoader::Loader *IR;
  constexpr static uint64_t STACK_SIZE = 8 * 1024 * 1024;
};

class DummySyscallHandler: public FEXCore::HLE::SyscallHandler {
  public:

  uint64_t HandleSyscall(FEXCore::Core::CpuStateFrame *Frame, FEXCore::HLE::SyscallArguments *Args) override {
    LOGMAN_MSG_A_FMT("Syscalls not implemented");
    return 0;
  }

  FEXCore::HLE::SyscallABI GetSyscallABI(uint64_t Syscall) override {
    LOGMAN_MSG_A_FMT("Syscalls not implemented");
    return {0, false, 0 };
  }

  // These are no-ops implementations of the SyscallHandler API
  std::shared_mutex StubMutex;
  FEXCore::HLE::AOTIRCacheEntryLookupResult LookupAOTIRCacheEntry(uint64_t GuestAddr) override {
    return {0, 0, FHU::ScopedSignalMaskWithSharedLock {StubMutex}};
  }

  std::shared_mutex StubMutex2;
  std::shared_lock<std::shared_mutex> CompileCodeLock(uint64_t Start) {
    return std::shared_lock(StubMutex2);
  }
};

int main(int argc, char **argv, char **const envp)
{
  LogMan::Throw::InstallHandler(AssertHandler);
  LogMan::Msg::InstallHandler(MsgHandler);

  FEXCore::Config::Initialize();
  FEXCore::Config::AddLayer(std::make_unique<FEX::ArgLoader::ArgLoader>(argc, argv));
  FEXCore::Config::AddLayer(FEXCore::Config::CreateEnvironmentLayer(envp));
  FEXCore::Config::Load();

  auto Args = FEX::ArgLoader::Get();
  auto ParsedArgs = FEX::ArgLoader::GetParsedArgs();

  LOGMAN_THROW_A_FMT(Args.size() > 1, "Not enough arguments");

  FEXCore::Context::InitializeStaticTables();
  auto CTX = FEXCore::Context::CreateNewContext();
  FEXCore::Context::InitializeContext(CTX);

  std::unique_ptr<FEX::HLE::SignalDelegator> SignalDelegation = std::make_unique<FEX::HLE::SignalDelegator>();

  FEXCore::Context::SetSignalDelegator(CTX, SignalDelegation.get());
  FEXCore::Context::SetSyscallHandler(CTX, new DummySyscallHandler());

  FEX::IRLoader::Loader Loader(Args[0], Args[1]);

  int Return{};

  if (Loader.IsValid())
  {
    IRCodeLoader CodeLoader{&Loader};
    CodeLoader.MapMemory(nullptr, nullptr);
    FEXCore::Context::InitCore(CTX, &CodeLoader);
    auto ShutdownReason = FEXCore::Context::ExitReason::EXIT_SHUTDOWN;

    // There might already be an exit handler, leave it installed
    if (!FEXCore::Context::GetExitHandler(CTX))
    {
      FEXCore::Context::SetExitHandler(CTX, [&](uint64_t thread, FEXCore::Context::ExitReason reason) {
        if (reason != FEXCore::Context::ExitReason::EXIT_DEBUG)
        {
          ShutdownReason = reason;
          FEXCore::Context::Stop(CTX);
        }
      });
    }

    FEXCore::Context::RunUntilExit(CTX);

    LogMan::Msg::DFmt("Reason we left VM: {}", ShutdownReason);

    // Just re-use compare state. It also checks against the expected values in config.
    FEXCore::Core::CPUState State;
    FEXCore::Context::GetCPUState(CTX, &State);
    const bool Passed = Loader.CompareStates(&State);

    LogMan::Msg::IFmt("Passed? {}\n", Passed ? "Yes" : "No");

    Return = Passed ? 0 : -1;
  }
  else
  {
    LogMan::Msg::EFmt("Couldn't load IR");
    Return = -1;
  }

  FEXCore::Context::DestroyContext(CTX);

  return Return;
}
