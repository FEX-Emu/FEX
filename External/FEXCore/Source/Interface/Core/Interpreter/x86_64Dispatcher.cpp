#include "Common/MathUtils.h"
#include "Interface/Core/ArchHelpers/MContext.h"
#include "Interface/Core/Interpreter/InterpreterClass.h"
#include "Interface/Context/Context.h"
#include <FEXCore/Core/X86Enums.h>

#include <cmath>
#include <xbyak/xbyak.h>

namespace FEXCore::CPU {
static constexpr size_t MAX_DISPATCHER_CODE_SIZE = 4096;
#define STATE r14

class X86DispatchGenerator : public Xbyak::CodeGenerator {
  public:
    X86DispatchGenerator(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread);
    bool HandleGuestSignal(int Signal, void *info, void *ucontext, GuestSigAction *GuestAction, stack_t *GuestStack);
    bool HandleSignalPause(int Signal, void *info, void *ucontext);

  CPUBackend::AsmDispatch DispatchPtr;
  CPUBackend::JITCallback CallbackPtr;
  FEXCore::Context::Context::IntCallbackReturn ReturnPtr;

  uint64_t ThreadStopHandlerAddress;
  uint64_t AbsoluteLoopTopAddress;
  uint64_t ThreadPauseHandlerAddress;
  FEXCore::Context::Context *CTX;
  FEXCore::Core::InternalThreadState *ThreadState;
  private:
    void StoreThreadState(int Signal, void *ucontext);
    void RestoreThreadState(void *ucontext);
    std::stack<uint64_t> SignalFrames;
};

static void SleepThread(FEXCore::Context::Context *ctx, FEXCore::Core::CpuStateFrame *Frame) {
  auto Thread = Frame->Thread;

  --ctx->IdleWaitRefCount;
  ctx->IdleWaitCV.notify_all();

  // Go to sleep
  Thread->StartRunning.Wait();

  Thread->RunningEvents.Running = true;
  ++ctx->IdleWaitRefCount;
  ctx->IdleWaitCV.notify_all();
}

X86DispatchGenerator::X86DispatchGenerator(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread)
  : Xbyak::CodeGenerator(MAX_DISPATCHER_CODE_SIZE)
  , CTX {ctx}
  , ThreadState {Thread} {
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
  mov(qword [rdi + offsetof(FEXCore::Core::CpuStateFrame, ReturningStackLocation)], rsp);

  Label LoopTop;
  Label NoBlock;
  Label ExitBlock;
  Label ThreadPauseHandler;

  L(LoopTop);
  AbsoluteLoopTopAddress = getCurr<uint64_t>();

  {
    mov(r13, Thread->LookupCache->GetPagePointer());

    // Load our RIP
    mov(rdx, qword [STATE + offsetof(FEXCore::Core::CPUState, rip)]);

    mov(rax, rdx);
    mov(rbx, Thread->LookupCache->GetVirtualMemorySize() - 1);
    and_(rax, rbx);
    shr(rax, 12);

    // Load page pointer
    mov(rdi, qword [r13 + rax * 8]);

    cmp(rdi, 0);
    je(NoBlock);

    mov (rax, rdx);
    and_(rax, 0x0FFF);

    shl(rax, (int)log2(sizeof(FEXCore::LookupCache::LookupCacheEntry)));

    // check for aliasing
    mov(rcx, qword [rdi + rax + 8]);
    cmp(rcx, rdx);
    jne(NoBlock);

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

    using ClassPtrType = uintptr_t (FEXCore::Context::Context::*)(FEXCore::Core::CpuStateFrame *, uint64_t);
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
    sub(qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, State.gregs[X86State::REG_RSP])], 16);
    mov(rbx, qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, State.gregs[X86State::REG_RSP])]);
    mov(qword [rbx], rax);

    // Store RIP to the context state
    mov(qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, State.rip)], rsi);

    // Back to the loop top now
    jmp(LoopTop);
  }


  {
    ReturnPtr = getCurr<FEXCore::Context::Context::IntCallbackReturn>();
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

void X86DispatchGenerator::StoreThreadState(int Signal, void *ucontext) {
  // We can end up getting a signal at any point in our host state
  // Jump to a handler that saves all state so we can safely return
  uint64_t OldSP = ArchHelpers::Context::GetSp(ucontext);
  uintptr_t NewSP = OldSP;

  size_t StackOffset = sizeof(X86ContextBackup);

  // We need to back up behind the host's red zone
  // We do this on the guest side as well
  NewSP -= 128;
  NewSP -= StackOffset;
  NewSP = AlignDown(NewSP, 16);

  X86ContextBackup *Context = reinterpret_cast<X86ContextBackup*>(NewSP);
  ArchHelpers::Context::BackupContext(ucontext, Context);

  // Retain the action pointer so we can see it when we return
  Context->Signal = Signal;

  // Save guest state
  // We can't guarantee if registers are in context or host GPRs
  // So we need to save everything
  memcpy(&Context->GuestState, ThreadState->CurrentFrame, sizeof(FEXCore::Core::CPUState));

  // Set the new SP
  ArchHelpers::Context::SetSp(ucontext, NewSP);

  SignalFrames.push(NewSP);
}

void X86DispatchGenerator::RestoreThreadState(void *ucontext) {
  uint64_t OldSP = SignalFrames.top();
  SignalFrames.pop();
  uintptr_t NewSP = OldSP;
  X86ContextBackup *Context = reinterpret_cast<X86ContextBackup*>(NewSP);

  // First thing, reset the guest state
  memcpy(ThreadState->CurrentFrame, &Context->GuestState, sizeof(FEXCore::Core::CPUState));

  // Now restore host state
  ArchHelpers::Context::RestoreContext(ucontext, Context);

  // Restore the previous signal state
  // This allows recursive signals to properly handle signal masking as we are walking back up the list of signals
  CTX->SignalDelegation->SetCurrentSignal(Context->Signal);
}

bool X86DispatchGenerator::HandleGuestSignal(int Signal, void *info, void *ucontext, GuestSigAction *GuestAction, stack_t *GuestStack) {
  StoreThreadState(Signal, ucontext);

  auto Frame = ThreadState->CurrentFrame;

  // Set the new PC
  ArchHelpers::Context::SetPc(ucontext, AbsoluteLoopTopAddress);
  // Set our state register to point to our guest thread data
  ArchHelpers::Context::SetState(ucontext, reinterpret_cast<uint64_t>(Frame));


  uint64_t OldGuestSP = Frame->State.gregs[X86State::REG_RSP];
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

  Frame->State.gregs[X86State::REG_RDI] = Signal;

  if (GuestAction->sa_flags & SA_SIGINFO) {
    // XXX: siginfo_t(RSI), ucontext (RDX)
    Frame->State.gregs[X86State::REG_RSI] = 0;
    Frame->State.gregs[X86State::REG_RDX] = 0;
    Frame->State.rip = reinterpret_cast<uint64_t>(GuestAction->sigaction_handler.sigaction);
  }
  else {
    Frame->State.rip = reinterpret_cast<uint64_t>(GuestAction->sigaction_handler.handler);
  }

  // Set up the new SP for stack handling
  NewGuestSP -= 8;
  *(uint64_t*)NewGuestSP = CTX->X86CodeGen.SignalReturn;
  Frame->State.gregs[X86State::REG_RSP] = NewGuestSP;

  return true;
}

bool X86DispatchGenerator::HandleSignalPause(int Signal, void *info, void *ucontext) {
  FEXCore::Core::SignalEvent SignalReason = ThreadState->SignalReason.load();

  auto Frame = ThreadState->CurrentFrame;

  if (SignalReason == FEXCore::Core::SignalEvent::SIGNALEVENT_PAUSE) {


    // Store our thread state so we can come back to this
    StoreThreadState(Signal, ucontext);

    // Set the new PC
    ArchHelpers::Context::SetPc(ucontext, ThreadPauseHandlerAddress);

    // Set our state register to point to our guest thread data
    ArchHelpers::Context::SetState(ucontext, reinterpret_cast<uint64_t>(Frame));


    ThreadState->SignalReason.store(FEXCore::Core::SIGNALEVENT_NONE);
    return true;
  }

  if (SignalReason == FEXCore::Core::SignalEvent::SIGNALEVENT_STOP) {
    // Our thread is stopping
    // We don't care about anything at this point
    // Set the stack to our starting location when we entered the core and get out safely
    ArchHelpers::Context::SetSp(ucontext, Frame->ReturningStackLocation);

    // Set the new PC
    ArchHelpers::Context::SetPc(ucontext, ThreadStopHandlerAddress);

    ThreadState->SignalReason.store(FEXCore::Core::SIGNALEVENT_NONE);
    return true;
  }

  if (SignalReason == FEXCore::Core::SignalEvent::SIGNALEVENT_RETURN) {
    RestoreThreadState(ucontext);

    ThreadState->SignalReason.store(FEXCore::Core::SIGNALEVENT_NONE);
    return true;
  }

  return false;
}

#ifdef _M_X86_64

void InterpreterCore::CreateAsmDispatch(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread) {
  Generator = new X86DispatchGenerator(ctx, Thread);
  DispatchPtr = Generator->DispatchPtr;
  CallbackPtr = Generator->CallbackPtr;

  // TODO: It feels wrong to initialize this way
  ctx->InterpreterCallbackReturn = Generator->ReturnPtr;
}

bool InterpreterCore::HandleGuestSignal(int Signal, void *info, void *ucontext, GuestSigAction *GuestAction, stack_t *GuestStack) {
  X86DispatchGenerator *Gen = Generator;
  return Gen->HandleGuestSignal(Signal, info, ucontext, GuestAction, GuestStack);
}

bool InterpreterCore::HandleSignalPause(int Signal, void *info, void *ucontext) {
  X86DispatchGenerator *Gen = Generator;
  return Gen->HandleSignalPause(Signal, info, ucontext);
}

void InterpreterCore::DeleteAsmDispatch() {
  delete Generator;
}

#endif

}
