// SPDX-License-Identifier: MIT
/*
$info$
tags: Bin|IRLoader
desc: Used to run IR Tests
$end_info$
*/

#include "Common/ArgumentLoader.h"
#include "LinuxSyscalls/SignalDelegator.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CodeLoader.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/HostFeatures.h>
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/EnumUtils.h>
#include <FEXCore/Utils/FileLoading.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/IR/IREmitter.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/sstream.h>

#include <csetjmp>
#include <functional>
#include <memory>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <vector>

#include "HarnessHelpers.h"

namespace FEXCore::Context {
  class Context;
}

void MsgHandler(LogMan::DebugLevels Level, char const *Message)
{
  fextl::fmt::print("[{}] {}\n", LogMan::DebugLevelStr(Level), Message);
  fflush(stdout);
}

void AssertHandler(char const *Message)
{
  fextl::fmt::print("[ASSERT] {}\n", Message);
  fflush(stdout);
}

using namespace FEXCore::IR;
class IRCodeLoader final {
public:
  IRCodeLoader(fextl::string const &Filename, fextl::string const &ConfigFilename) {
    Config.Init(ConfigFilename);

    fextl::string IRFile;
    if (!FEXCore::FileLoading::LoadFile(IRFile, Filename)) {
      LogMan::Msg::EFmt("Couldn't open IR file '{}'", Filename);
      return;
    }
    fextl::stringstream IRStream(IRFile);
    ParsedCode = FEXCore::IR::Parse(Allocator, IRStream);

    if (ParsedCode) {
      EntryRIP = 0x40000;

      fextl::stringstream out;
      auto IR = ParsedCode->ViewIR();
      FEXCore::IR::Dump(&out, &IR, nullptr);
      fextl::fmt::print("IR:\n{}\n@@@@@\n", out.str());

      for (auto &[region, size] : Config.GetMemoryRegions()) {
        FEXCore::Allocator::mmap(reinterpret_cast<void *>(region), size, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      }

      Config.LoadMemory();
    }
  }

  bool LoadIR(FEXCore::Context::Context *CTX) const {
    if (!ParsedCode) {
      return false;
    }

    return !!CTX->AddCustomIREntrypoint(EntryRIP, [ParsedCodePtr = ParsedCode.get()](uintptr_t Entrypoint, FEXCore::IR::IREmitter *emit) {
      emit->CopyData(*ParsedCodePtr);
    });
  }

  bool CompareStates(FEXCore::Core::CPUState const *State, bool SupportsAVX) {
    return Config.CompareStates(State, nullptr, SupportsAVX);
  }

  uint64_t GetStackPointer()
  {
    return reinterpret_cast<uint64_t>(FEXCore::Allocator::mmap(nullptr, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
  }

  uint64_t DefaultRIP() const
  {
    return EntryRIP;
  }

  bool RequiresAVX() const {
    return Config.RequiresAVX();
  }

private:
  uint64_t EntryRIP{};
  fextl::unique_ptr<IREmitter> ParsedCode;

  FEXCore::Utils::PooledAllocatorMalloc Allocator;
  FEX::HarnessHelper::ConfigLoader Config;
  constexpr static uint64_t STACK_SIZE = 8 * 1024 * 1024;
};

class DummySyscallHandler: public FEXCore::HLE::SyscallHandler, public FEXCore::Allocator::FEXAllocOperators {
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
  FEXCore::HLE::AOTIRCacheEntryLookupResult LookupAOTIRCacheEntry(FEXCore::Core::InternalThreadState *Thread, uint64_t GuestAddr) override {
    return {0, 0};
  }
};

int main(int argc, char **argv, char **const envp)
{
  FEXCore::Allocator::GLIBCScopedFault GLIBFaultScope;
  LogMan::Throw::InstallHandler(AssertHandler);
  LogMan::Msg::InstallHandler(MsgHandler);

  FEXCore::Config::Initialize();
  FEXCore::Config::AddLayer(fextl::make_unique<FEX::ArgLoader::ArgLoader>(argc, argv));
  FEXCore::Config::AddLayer(FEX::Config::CreateEnvironmentLayer(envp));
  FEXCore::Config::Load();
  // Ensure the IRLoader runs in 64-bit mode.
  // This is to ensure that static register allocation in the JIT
  // is configured correctly for accesses to the top 8 GPRs and 8 XMM registers.
  FEXCore::Config::EraseSet(FEXCore::Config::CONFIG_IS64BIT_MODE, "1");
#ifdef VIXL_SIMULATOR
  // If running under the vixl simulator, ensure that indirect runtime calls are enabled.
  FEXCore::Config::EraseSet(FEXCore::Config::CONFIG_DISABLE_VIXL_INDIRECT_RUNTIME_CALLS, "0");
#endif

  auto Args = FEX::ArgLoader::Get();
  auto ParsedArgs = FEX::ArgLoader::GetParsedArgs();

  LOGMAN_THROW_A_FMT(Args.size() > 1, "Not enough arguments");

  FEXCore::Context::InitializeStaticTables();
  auto CTX = FEXCore::Context::Context::CreateNewContext();
  CTX->InitializeContext();

  auto SignalDelegation = FEX::HLE::CreateSignalDelegator(CTX.get(), {});

  CTX->SetSignalDelegator(SignalDelegation.get());
  CTX->SetSyscallHandler(new DummySyscallHandler());

  IRCodeLoader Loader(Args[0], Args[1]);

  // Skip tests that require AVX on hosts that don't support it.
  const bool SupportsAVX = CTX->GetHostFeatures().SupportsAVX;
  if (!SupportsAVX && Loader.RequiresAVX()) {
    return 0;
  }

  int Return{};

  if (Loader.LoadIR(CTX.get()))
  {
    CTX->InitCore(Loader.DefaultRIP(), Loader.GetStackPointer());

    auto ShutdownReason = FEXCore::Context::ExitReason::EXIT_SHUTDOWN;

    // There might already be an exit handler, leave it installed
    if (!CTX->GetExitHandler())
    {
      CTX->SetExitHandler([&](uint64_t thread, FEXCore::Context::ExitReason reason) {
        if (reason != FEXCore::Context::ExitReason::EXIT_DEBUG)
        {
          ShutdownReason = reason;
          CTX->Stop();
        }
      });
    }

    jmp_buf LongJump{};
    int LongJumpVal{};

    SignalDelegation->RegisterFrontendHostSignalHandler(SIGSEGV, [&LongJump](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) {
      longjmp(LongJump, 1);
      return false;
    }, true);

    LongJumpVal = setjmp(LongJump);
    if (!LongJumpVal) {
      CTX->RunUntilExit();
    }

    LogMan::Msg::DFmt("Reason we left VM: {}", FEXCore::ToUnderlying(ShutdownReason));

    // Just re-use compare state. It also checks against the expected values in config.
    FEXCore::Core::CPUState State;
    CTX->GetCPUState(&State);

    const bool Passed = Loader.CompareStates(&State, SupportsAVX);

    LogMan::Msg::IFmt("Passed? {}\n", Passed ? "Yes" : "No");

    Return = Passed ? 0 : -1;
  }
  else
  {
    LogMan::Msg::EFmt("Couldn't load IR");
    Return = -1;
  }

  return Return;
}
