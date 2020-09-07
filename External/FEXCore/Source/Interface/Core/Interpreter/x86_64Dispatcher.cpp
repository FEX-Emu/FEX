#include "Common/MathUtils.h"
#include "Interface/Core/Interpreter/InterpreterClass.h"
#include <FEXCore/Core/X86Enums.h>

#include <cmath>
#include <xbyak/xbyak.h>

namespace FEXCore::CPU {
static constexpr size_t MAX_DISPATCHER_CODE_SIZE = 4096;
#define STATE r14

class DispatchGenerator : public Xbyak::CodeGenerator {
  public:
    DispatchGenerator(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread);
    bool HandleGuestSignal(int Signal, void *info, void *ucontext, SignalDelegator::GuestSigAction *GuestAction, stack_t *GuestStack);
    bool HandleSignalPause(int Signal, void *info, void *ucontext);

  CPUBackend::AsmDispatch DispatchPtr;
  CPUBackend::JITCallback CallbackPtr;
  InterpreterCore::CallbackReturn ReturnPtr;

  uint64_t ThreadStopHandlerAddress;
  uint64_t AbsoluteLoopTopAddress;
  uint64_t ThreadPauseHandlerAddress;
  FEXCore::Context::Context *CTX;
  FEXCore::Core::InternalThreadState *State;
  private:
    void StoreThreadState(int Signal, void *ucontext);
    void RestoreThreadState(void *ucontext);
    std::stack<uint64_t> SignalFrames;
};

static void SleepThread(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread) {
  --ctx->IdleWaitRefCount;
  ctx->IdleWaitCV.notify_all();

  // Go to sleep
  Thread->StartRunning.Wait();

  Thread->State.RunningEvents.Running = true;
  ++ctx->IdleWaitRefCount;
  ctx->IdleWaitCV.notify_all();
}

DispatchGenerator::DispatchGenerator(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread)
  : Xbyak::CodeGenerator(MAX_DISPATCHER_CODE_SIZE)
  , CTX {ctx}
  , State {Thread} {
  using namespace Xbyak;
  using namespace Xbyak::util;
  DispatchPtr = getCurr<CPUBackend::AsmDispatch>();

  // while (!Thread->State.RunningEvents.ShouldStop.load()) {
  //    Ptr = FindBlock(RIP)
  //    if (!Ptr)
  //      Ptr = CTX->CompileBlock(RIP);
  //
  //    if (Ptr)
  //      Ptr();
  //    else
  //    {
  //      Ptr = FallbackCore->CompileBlock()
  //      if (Ptr)
  //        Ptr()
  //      else {
  //        ShouldStop = true;
  //      }
  //    }
  // }
  // Bunch of exit state stuff

  // x86-64 ABI has the stack aligned when /call/ happens
  // Which means the destination has a misaligned stack at that point
  push(rbx);
  push(rbp);
  push(r12);
  push(r13);
  push(r14);
  push(r15);
  sub(rsp, 8);

  mov(STATE, rdi);

  // Save this stack pointer so we can cleanly shutdown the emulation with a long jump
  // regardless of where we were in the stack
  mov(qword [rdi + offsetof(FEXCore::Core::ThreadState, ReturningStackLocation)], rsp);

  Label LoopTop;
  Label NoBlock;
  Label ExitBlock;
  Label ThreadPauseHandler;

  L(LoopTop);
  AbsoluteLoopTopAddress = getCurr<uint64_t>();

  {
    mov(r13, Thread->BlockCache->GetPagePointer());

    // Load our RIP
    mov(rdx, qword [STATE + offsetof(FEXCore::Core::CPUState, rip)]);

    mov(rax, rdx);
    mov(rbx, Thread->BlockCache->GetVirtualMemorySize() - 1);
    and_(rax, rbx);
    shr(rax, 12);

    // Load page pointer
    mov(rdi, qword [r13 + rax * 8]);

    cmp(rdi, 0);
    je(NoBlock);

    mov (rax, rdx);
    and_(rax, 0x0FFF);

    shl(rax, (int)log2(sizeof(FEXCore::BlockCache::BlockCacheEntry)));

    // Load the block pointer
    mov(rax, qword [rdi + rax]);

    cmp(rax, 0);
    je(NoBlock);

    // Real block if we made it here
    mov(rdi, STATE);
    call(rax);

    if (CTX->GetGdbServerStatus()) {
      // If we have a gdb server running then run in a less efficient mode that checks if we need to exit
      // This happens when single stepping
      static_assert(sizeof(CTX->Config.RunningMode) == 4, "This is expected to be size of 4");
      mov(rax, qword [STATE + (offsetof(FEXCore::Core::InternalThreadState, CTX))]);

      // If the value == 0 then branch to the top
      cmp(dword [rax + (offsetof(FEXCore::Context::Context, Config.RunningMode))], 0);
      je(LoopTop);
      // Else we need to pause now
      jmp(ThreadPauseHandler);
      ud2();
    }
    else {
      jmp(LoopTop);
    }
  }

  {
    L(ExitBlock);
    ThreadStopHandlerAddress = getCurr<uint64_t>();

    add(rsp, 8);

    pop(r15);
    pop(r14);
    pop(r13);
    pop(r12);
    pop(rbp);
    pop(rbx);

    ret();
  }

  // Block creation
  {
    L(NoBlock);

    using ClassPtrType = uintptr_t (FEXCore::Context::Context::*)(FEXCore::Core::InternalThreadState *, uint64_t);
    union PtrCast {
      ClassPtrType ClassPtr;
      uintptr_t Data;
    };

    PtrCast Ptr;
    Ptr.ClassPtr = &FEXCore::Context::Context::CompileBlock;

    // {rdi, rsi, rdx}
    mov(rdi, reinterpret_cast<uint64_t>(CTX));
    mov(rsi, STATE);
    mov(rax, Ptr.Data);

    call(rax);

    // rdx already contains RIP here
    jmp(LoopTop);
  }

  {
    // Pause handler
    ThreadPauseHandlerAddress = getCurr<uint64_t>();
    L(ThreadPauseHandler);

    mov(rdi, reinterpret_cast<uintptr_t>(CTX));
    mov(rsi, STATE);
    mov(rax, reinterpret_cast<uint64_t>(SleepThread));

    call(rax);

    // XXX: Unsupported atm
    // uint64_t PauseReturnInstruction = getCurr<uint64_t>();
    // ud2();
  }

  {
    CallbackPtr = getCurr<CPUBackend::JITCallback>();

    push(rbx);
    push(rbp);
    push(r12);
    push(r13);
    push(r14);
    push(r15);
    sub(rsp, 8);

    // First thing we need to move the thread state pointer back in to our register
    mov(STATE, rdi);
    // XXX: XMM?

    // Now push the callback return trampoline to the guest stack
    // Guest will be misaligned because calling a thunk won't correct the guest's stack once we call the callback from the host
    mov(rax, CTX->X86CodeGen.CallbackReturn);

    // Store the trampoline to the guest stack
    // Guest stack is now correctly misaligned after a regular call instruction
    sub(qword [STATE + offsetof(FEXCore::Core::InternalThreadState, State.State.gregs[X86State::REG_RSP])], 16);
    mov(rbx, qword [STATE + offsetof(FEXCore::Core::InternalThreadState, State.State.gregs[X86State::REG_RSP])]);
    mov(qword [rbx], rax);

    // Store RIP to the context state
    mov(qword [STATE + offsetof(FEXCore::Core::InternalThreadState, State.State.rip)], rsi);

    // Back to the loop top now
    jmp(LoopTop);
  }


  {
    ReturnPtr = getCurr<InterpreterCore::CallbackReturn>();
//  using CallbackReturn =  __attribute__((naked)) void(*)(FEXCore::Core::InternalThreadState *Thread, volatile void *Host_RSP);

    // rdi = thread
    // rsi = rsp

    mov(rsp, rsi);

    // Now jump back to the thunk
    // XXX: XMM?
    add(rsp, 8);

    pop(r15);
    pop(r14);
    pop(r13);
    pop(r12);
    pop(rbp);
    pop(rbx);

    ret();
  }
  ready();
}

struct ContextBackup {
  uint64_t StoredCookie;
  // Host State
  // RIP and RSP is stored in GPRs here
  uint64_t GPRs[NGREG];
  _libc_fpstate FPRState;

  // Guest state
  int Signal;
  FEXCore::Core::CPUState GuestState;
};

void DispatchGenerator::StoreThreadState(int Signal, void *ucontext) {
  ucontext_t* _context = (ucontext_t*)ucontext;
  mcontext_t* _mcontext = &_context->uc_mcontext;

  // We can end up getting a signal at any point in our host state
  // Jump to a handler that saves all state so we can safely return
  uint64_t OldSP = _mcontext->gregs[REG_RSP];
  uintptr_t NewSP = OldSP;

  size_t StackOffset = sizeof(ContextBackup);

  // We need to back up behind the host's red zone
  // We do this on the guest side as well
  NewSP -= 128;
  NewSP -= StackOffset;
  NewSP = AlignDown(NewSP, 16);

  ContextBackup *Context = reinterpret_cast<ContextBackup*>(NewSP);

  Context->StoredCookie = 0x4142434445464748ULL;

  // Copy the GPRs
  memcpy(&Context->GPRs[0], &_mcontext->gregs[0], NGREG * sizeof(_mcontext->gregs[0]));
  // Copy the FPRState
  memcpy(&Context->FPRState, _mcontext->fpregs, sizeof(_libc_fpstate));

  // XXX: Save 256bit and 512bit AVX register state

  // Retain the action pointer so we can see it when we return
  Context->Signal = Signal;

  // Save guest state
  // We can't guarantee if registers are in context or host GPRs
  // So we need to save everything
  memcpy(&Context->GuestState, &State->State, sizeof(FEXCore::Core::CPUState));

  // Set the new SP
  _mcontext->gregs[REG_RSP] = NewSP;

  SignalFrames.push(NewSP);
}

void DispatchGenerator::RestoreThreadState(void *ucontext) {
  ucontext_t* _context = (ucontext_t*)ucontext;
  mcontext_t* _mcontext = &_context->uc_mcontext;

  uint64_t OldSP = SignalFrames.top();
  SignalFrames.pop();
  uintptr_t NewSP = OldSP;
  ContextBackup *Context = reinterpret_cast<ContextBackup*>(NewSP);

  if (Context->StoredCookie != 0x4142434445464748ULL) {
    LogMan::Msg::D("COOKIE WAS NOT CORRECT!\n");
    exit(-1);
  }

  // First thing, reset the guest state
  memcpy(&State->State, &Context->GuestState, sizeof(FEXCore::Core::CPUState));

  // Now restore host state

  // Copy the GPRs
  memcpy(&_mcontext->gregs[0], &Context->GPRs[0], NGREG * sizeof(_mcontext->gregs[0]));
  // Copy the FPRState
  memcpy(_mcontext->fpregs, &Context->FPRState, sizeof(_libc_fpstate));

  // Restore the previous signal state
  // This allows recursive signals to properly handle signal masking as we are walking back up the list of signals
  CTX->SignalDelegation.SetCurrentSignal(Context->Signal);
}

bool DispatchGenerator::HandleGuestSignal(int Signal, void *info, void *ucontext, SignalDelegator::GuestSigAction *GuestAction, stack_t *GuestStack) {
  ucontext_t* _context = (ucontext_t*)ucontext;
  mcontext_t* _mcontext = &_context->uc_mcontext;

  StoreThreadState(Signal, ucontext);

  // Set the new PC
  _mcontext->gregs[REG_RIP] = AbsoluteLoopTopAddress;
  // Set our state register to point to our guest thread data
  _mcontext->gregs[REG_R14] = reinterpret_cast<uint64_t>(State);

  uint64_t OldGuestSP = State->State.State.gregs[X86State::REG_RSP];
  uint64_t NewGuestSP = OldGuestSP;

  if (!(GuestStack->ss_flags & SS_DISABLE)) {
    // If our guest is already inside of the alternative stack
    // Then that means we are hitting recursive signals and we need to walk back the stack correctly
    uint64_t AltStackBase = reinterpret_cast<uint64_t>(GuestStack->ss_sp);
    uint64_t AltStackEnd = AltStackBase + GuestStack->ss_size;
    if (OldGuestSP >= AltStackBase &&
        OldGuestSP <= AltStackEnd) {
      // We are already in the alt stack, the rest of the code will handle adjusting this
    }
    else {
      NewGuestSP = AltStackEnd;
    }
  }

  // Back up past the redzone, which is 128bytes
  // Don't need this offset if we aren't going to be putting siginfo in to it
  NewGuestSP -= 128;

  State->State.State.gregs[X86State::REG_RDI] = Signal;

  if (GuestAction->sa_flags & SA_SIGINFO) {
    // XXX: siginfo_t(RSI), ucontext (RDX)
    State->State.State.gregs[X86State::REG_RSI] = 0;
    State->State.State.gregs[X86State::REG_RDX] = 0;
    State->State.State.rip = reinterpret_cast<uint64_t>(GuestAction->sigaction_handler.sigaction);
  }
  else {
    State->State.State.rip = reinterpret_cast<uint64_t>(GuestAction->sigaction_handler.handler);
  }

  // Set up the new SP for stack handling
  NewGuestSP -= 8;
  *(uint64_t*)NewGuestSP = CTX->X86CodeGen.SignalReturn;
  State->State.State.gregs[X86State::REG_RSP] = NewGuestSP;

  return true;
}

bool DispatchGenerator::HandleSignalPause(int Signal, void *info, void *ucontext) {
  FEXCore::Core::SignalEvent SignalReason = State->SignalReason.load();

  if (SignalReason == FEXCore::Core::SignalEvent::SIGNALEVENT_PAUSE) {
    ucontext_t* _context = (ucontext_t*)ucontext;
    mcontext_t* _mcontext = &_context->uc_mcontext;

    // Store our thread state so we can come back to this
    StoreThreadState(Signal, ucontext);

    // Set the new PC
    _mcontext->gregs[REG_RIP] = ThreadPauseHandlerAddress;

    // Set our state register to point to our guest thread data
    _mcontext->gregs[REG_R14] = reinterpret_cast<uint64_t>(State);

    State->SignalReason.store(FEXCore::Core::SIGNALEVENT_NONE);
    return true;
  }

  if (SignalReason == FEXCore::Core::SignalEvent::SIGNALEVENT_STOP) {
    ucontext_t* _context = (ucontext_t*)ucontext;
    mcontext_t* _mcontext = &_context->uc_mcontext;

    // Our thread is stopping
    // We don't care about anything at this point
    // Set the stack to our starting location when we entered the core and get out safely
    _mcontext->gregs[REG_RSP] = State->State.ReturningStackLocation;

    // Set the new PC
    _mcontext->gregs[REG_RIP] = ThreadStopHandlerAddress;

    State->SignalReason.store(FEXCore::Core::SIGNALEVENT_NONE);
    return true;
  }

  if (SignalReason == FEXCore::Core::SignalEvent::SIGNALEVENT_RETURN) {
    RestoreThreadState(ucontext);

    State->SignalReason.store(FEXCore::Core::SIGNALEVENT_NONE);
    return true;
  }

  return false;
}

void InterpreterCore::CreateAsmDispatch(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread) {
  Generator = new DispatchGenerator(ctx, Thread);
  DispatchPtr = Generator->DispatchPtr;
  CallbackPtr = Generator->CallbackPtr;
  ReturnPtr = Generator->ReturnPtr;
}

bool InterpreterCore::HandleGuestSignal(int Signal, void *info, void *ucontext, SignalDelegator::GuestSigAction *GuestAction, stack_t *GuestStack) {
  DispatchGenerator *Gen = Generator;
  return Gen->HandleGuestSignal(Signal, info, ucontext, GuestAction, GuestStack);
}

bool InterpreterCore::HandleSignalPause(int Signal, void *info, void *ucontext) {
  DispatchGenerator *Gen = Generator;
  return Gen->HandleSignalPause(Signal, info, ucontext);
}

void InterpreterCore::DeleteAsmDispatch() {
  delete Generator;
}

}
