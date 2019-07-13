#include "Common/MathUtils.h"
#include "Core/CPU/CPUBackend.h"
#include "Core/CPU/CPUCore.h"
#include "Core/CPU/DebugData.h"
#include "Core/CPU/IR.h"
#include "Core/CPU/IntrusiveIRList.h"
#include "Core/CPU/HostCore/HostCore.h"
#include "LogManager.h"
#include <ucontext.h>

#include <cassert>
#include <iostream>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <errno.h>
#include <sys/user.h>
#include <stdint.h>

namespace FEX::CPU {

class HostCore final : public CPUBackend {
public:
  explicit HostCore(FEX::ThreadState *Thread);
  ~HostCore() override = default;
  std::string GetName() override { return "Host Stepper"; }
  void* CompileCode(FEX::IR::IntrusiveIRList const *ir, FEX::CPU::DebugData *DebugData) override;

  void *MapRegion(void *HostPtr, uint64_t GuestPtr, uint64_t Size) override {
    // Map locally to unprotected
    printf("Mapping Guest Ptr: 0x%lx\n", GuestPtr);
    void *Mem = LocalMemoryMapper.MapRegionFlags(GuestPtr, Size, PROT_READ | PROT_WRITE | PROT_EXEC);
    memset(Mem, 0xCC, Size);
    return HostPtr;
  }

  void ExecuteCode(FEX::ThreadState *Thread);

  void SignalHandler(int sig, siginfo_t *info, void *RawContext);

  bool NeedsOpDispatch() override { return false; }
private:
  void HandleSyscall();
  void ExecutionThreadFunction();

  void InstallSignalHandler();

  FEX::ThreadState *ThreadState;
  FEX::Config::Value<bool> ConfigMultiblock{"Multiblock", false};
  std::thread ExecutionThread;
  Memmap LocalMemoryMapper{};
  pid_t ChildPid;
  struct sigaction OldSigAction_SEGV;
  struct sigaction OldSigAction_TRAP;

  int PipeFDs[2];
  std::atomic_bool ShouldStart{false};
};

static HostCore *GlobalCore;
static void SigAction_SEGV(int sig, siginfo_t* info, void* RawContext) {
  GlobalCore->SignalHandler(sig, info, RawContext);
}

HostCore::HostCore(FEX::ThreadState *Thread)
  : ThreadState {Thread} {

  GlobalCore = this;
  LocalMemoryMapper.AllocateSHMRegion(1ULL << 36);

  ExecutionThread = std::thread(&HostCore::ExecutionThreadFunction, this);
}
static void HostExecution(FEX::ThreadState *Thread) {
  HostCore *Core = reinterpret_cast<HostCore*>(Thread->CPUBackend.get());
  Core->ExecuteCode(Thread);
}

void HostCore::SignalHandler(int sig, siginfo_t *info, void *RawContext) {
  ucontext_t* Context = (ucontext_t*)RawContext;
  uint64_t HostToEmuRIP = (uint64_t)Context->uc_mcontext.gregs[REG_RIP] - LocalMemoryMapper.GetBaseOffset<uint64_t>(0);
  printf("We hecked up and faulted! %d Emu:0x%lx Host:0x%lx\n", sig, HostToEmuRIP, (uint64_t)Context->uc_mcontext.gregs[REG_RIP]);
  static uint64_t LastEMURip = ~0ULL;
  static uint64_t LastInstSize = 0;
  if (sig == SIGSEGV) {
    // RIP == Base instruction
  }
  else if (sig == SIGTRAP) {
    HostToEmuRIP -= 1; // 0xCC moves us ahead by one
  }

  uint8_t *LocalData = LocalMemoryMapper.GetPointer<uint8_t*>(HostToEmuRIP);
  uint8_t *ActualData = ThreadState->CPUCore->MemoryMapper->GetPointer<uint8_t*>(HostToEmuRIP);
  uint64_t TotalInstructionsLength {0};

  ThreadState->CPUCore->FrontendDecoder.DecodeInstructionsInBlock(ActualData, HostToEmuRIP);
  auto DecodedOps = ThreadState->CPUCore->FrontendDecoder.GetDecodedInsts();
  if (sig == SIGSEGV) {
    for (size_t i = 0; i < DecodedOps.second; ++i) {
      FEX::X86Tables::DecodedInst const* DecodedInfo {nullptr};
      DecodedInfo = &DecodedOps.first->at(i);
      auto CheckOp = [&](char const* Name, FEX::X86Tables::DecodedOperand const &Operand) {
        if (Operand.TypeNone.Type != FEX::X86Tables::DecodedOperand::TYPE_NONE &&
            Operand.TypeNone.Type != FEX::X86Tables::DecodedOperand::TYPE_LITERAL &&
            Operand.TypeNone.Type != FEX::X86Tables::DecodedOperand::TYPE_GPR) {
          printf("Operand type is %d\n", Operand.TypeNone.Type);
          const std::vector<unsigned> EmulatorToSystemContext = {
            offsetof(mcontext_t, gregs[REG_RAX]),
            offsetof(mcontext_t, gregs[REG_RBX]),
            offsetof(mcontext_t, gregs[REG_RCX]),
            offsetof(mcontext_t, gregs[REG_RDX]),
            offsetof(mcontext_t, gregs[REG_RSI]),
            offsetof(mcontext_t, gregs[REG_RDI]),
            offsetof(mcontext_t, gregs[REG_RBP]),
            offsetof(mcontext_t, gregs[REG_RSP]),
            offsetof(mcontext_t, gregs[REG_R8]),
            offsetof(mcontext_t, gregs[REG_R9]),
            offsetof(mcontext_t, gregs[REG_R10]),
            offsetof(mcontext_t, gregs[REG_R11]),
            offsetof(mcontext_t, gregs[REG_R12]),
            offsetof(mcontext_t, gregs[REG_R13]),
            offsetof(mcontext_t, gregs[REG_R14]),
            offsetof(mcontext_t, gregs[REG_R15]),
          };

          // Modify the registers to match the memory operands necessary.
          // This will propagate some addresses
          if (Operand.TypeNone.Type == FEX::X86Tables::DecodedOperand::TYPE_GPR_DIRECT) {
            uint64_t *GPR = (uint64_t*)((uintptr_t)&Context->uc_mcontext + EmulatorToSystemContext[Operand.TypeGPR.GPR]);
            uint64_t HostPointer = LocalMemoryMapper.GetPointer<uint64_t>(*GPR);
            printf("Changing host pointer from 0x%lx to %lx\n", *GPR, HostPointer);
            *GPR = HostPointer;
          }
          else {
            OldSigAction_SEGV.sa_sigaction(sig, info, RawContext);
            return;
          }
        }
      };
      printf("Insts: %ld\n", DecodedOps.second);
      CheckOp("Dest", DecodedInfo->Dest);
      CheckOp("Src1", DecodedInfo->Src1);
      CheckOp("Src2", DecodedInfo->Src2);

      // Reset RIP to the start of the instruction
      Context->uc_mcontext.gregs[REG_RIP] = (uint64_t)LocalData;
      return;
    }

    OldSigAction_SEGV.sa_sigaction(sig, info, RawContext);
    return;
  }
  else if (sig == SIGTRAP) {
    for (size_t i = 0; i < DecodedOps.second; ++i) {
      FEX::X86Tables::DecodedInst const* DecodedInfo {nullptr};
      DecodedInfo = &DecodedOps.first->at(i);
      TotalInstructionsLength += DecodedInfo->InstSize;
    }

    if (LastEMURip != ~0ULL) {
      uint8_t *PreviousLocalData = LocalMemoryMapper.GetPointer<uint8_t*>(LastEMURip);
      memset(PreviousLocalData, 0xCC, LastInstSize);
    }
    LastInstSize = TotalInstructionsLength;
    printf("\tHit an instruction of length %ld 0x%lx 0x%lx 0x%lx\n", TotalInstructionsLength, (uint64_t)Context->uc_mcontext.gregs[REG_RIP], (uint64_t)LocalData, (uint64_t)ActualData);
    memcpy(LocalData, ActualData, TotalInstructionsLength);
    printf("\tData Source:");
    for (uint64_t i = 0; i < TotalInstructionsLength; ++i) {
      printf("%02x ", ActualData[i]);
    }
    printf("\n");

    // Reset RIP to the start of the instruction
    Context->uc_mcontext.gregs[REG_RIP] = (uint64_t)LocalData;
  }
}

void HostCore::InstallSignalHandler() {
  struct sigaction sa;
  sa.sa_handler = nullptr;
  sa.sa_sigaction = &SigAction_SEGV;
  sa.sa_flags = SA_SIGINFO;
  sigemptyset(&sa.sa_mask);
  // We use sigsegv to capture invalid memory accesses
  // We then patch the GPR state of that instruction to point to the correct location
  sigaction(SIGSEGV, &sa, &OldSigAction_SEGV);
  // We use trapping to determine when we've stepped to a new instruction
  sigaction(SIGTRAP, &sa, &OldSigAction_TRAP);
}

void HostCore::ExecutionThreadFunction() {
  printf("Host Core created\n");

  if (pipe(PipeFDs) == -1) {
    LogMan::Msg::A("Couldn't pipe");
    return;
  }
  ChildPid = fork();
  if (ChildPid == 0) {
    // Child
    // Set that we want to be traced
    if (ptrace(PTRACE_TRACEME, 0, 0, 0)) {
      LogMan::Msg::A("Couldn't start trace");
    }
    raise(SIGSTOP);
    InstallSignalHandler();

    close(PipeFDs[1]);
    using Ptr = void (*)();
    read(PipeFDs[0], &ThreadState->CPUState, sizeof(FEX::X86State::State));

    printf("Child is running! 0x%lx\n", ThreadState->CPUState.rip);
    Ptr Loc = LocalMemoryMapper.GetPointer<Ptr>(ThreadState->CPUState.rip);
    Loc();
    printf("Oh Snap. We returned in the child\n");
  }
  else {
    // Parent
    // Parent will be the ptrace control thread
    int status;
    close(PipeFDs[0]);

    waitpid(ChildPid, &status, 0);

    if (WIFSTOPPED(status) && WSTOPSIG(status) == 19) {
      // Attach to child
      ptrace(PTRACE_ATTACH, ChildPid, 0, 0);
      ptrace(PTRACE_CONT, ChildPid, 0, 0);
    }

    while (!ShouldStart.load());
    printf("Telling child to go!\n");
    write(PipeFDs[1], &ThreadState->CPUState, sizeof(FEX::X86State::State));

    while (1) {

      ShouldStart.store(false);

      waitpid(ChildPid, &status, 0);
      if (WIFEXITED(status)) {
        return;
      }
      if (WIFSTOPPED(status) && WSTOPSIG(status) == 5) {
        ptrace(PTRACE_CONT, ChildPid, 0, 5);
      }
      if (WIFSTOPPED(status) && WSTOPSIG(status) == 11) {
        ptrace(PTRACE_DETACH, ChildPid, 0, 11);
        break;
      }
    }
  }
}
void* HostCore::CompileCode([[maybe_unused]] FEX::IR::IntrusiveIRList const* ir, FEX::CPU::DebugData *DebugData) {
  printf("Attempting to compile: 0x%lx\n", ThreadState->CPUState.rip);
  return reinterpret_cast<void*>(HostExecution);
}

void HostCore::ExecuteCode(FEX::ThreadState *Thread) {
  ShouldStart = true;
  while(1);
}

FEX::CPU::CPUBackend *CreateHostCore(FEX::ThreadState *Thread) {
  return new HostCore(Thread);
}

}
