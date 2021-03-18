#include "Interface/Core/ArchHelpers/StateReg.h"
#include "Interface/Core/Dispatcher/X86Dispatcher.h"
#include "Interface/Core/Dispatcher/Dispatcher_asm.h"

#include "Interface/Core/Interpreter/InterpreterClass.h"
#include "Interface/Context/Context.h"

#include <FEXCore/Core/X86Enums.h>
#include <cmath>


namespace FEXCore::CPU {
static constexpr size_t MAX_DISPATCHER_CODE_SIZE = 4096;
static Xbyak::Reg64 STATE(STATE_x86);

X86Dispatcher::X86Dispatcher(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread, DispatcherConfig &config)
  : Dispatcher(ctx, Thread)
  , X86Emitter(MAX_DISPATCHER_CODE_SIZE) {

  using namespace Xbyak;
  using namespace Xbyak::util;
  DispatchPtr = getCurr<CPUBackend::AsmDispatch>();

  // Temp registers
  // rax, rcx, rdx, rsi, r8, r9,
  // r10, r11
  //
  // Callee Saved
  // rbx, rbp, r12, r13, r14, r15
  //
  // 1St Argument: rdi <ThreadState>
  // XMM:
  // All temp

  // while (true) {
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

  // Create a fake stack frame so we can exit by returning
  mov(rax, reinterpret_cast<uint64_t>(DispatcherExitReturn));
  push(rax);

  // Save this stack pointer so we can cleanly shutdown the emulation with a long jump
  // regardless of where we were in the stack
  mov(qword [rdi + offsetof(FEXCore::Core::CpuStateFrame, ReturningStackLocation)], rsp);

  // Fixup alignment again
  sub(rsp, 8);

  Label LoopTop;
  Label FullLookup;
  Label NoBlock;
  Label ExitBlock;
  Label ThreadPauseHandler;

  L(LoopTop);
  AbsoluteLoopTopAddressFillSRA = AbsoluteLoopTopAddress = getCurr<uint64_t>();

  {
    // Load our RIP
    mov(rdx, qword [STATE + offsetof(FEXCore::Core::CPUState, rip)]);

    if (!config.ExecuteBlocksWithCall)
    {
      // L1 Cache
      mov(r13, Thread->LookupCache->GetL1Pointer());
      mov(rax, rdx);

      and_(rax, LookupCache::L1_ENTRIES_MASK);
      shl(rax, 4);
      cmp(qword[r13 + rax + 8], rdx);
      jne(FullLookup);
      jmp(qword[r13 + rax + 0]);
    }

    L(FullLookup);
    mov(r13, Thread->LookupCache->GetPagePointer());

    // Full lookup
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

    // Update L1
    if (!config.ExecuteBlocksWithCall) {
      mov(r13, Thread->LookupCache->GetL1Pointer());
      mov(rcx, rdx);
      and_(rcx, LookupCache::L1_ENTRIES_MASK);
      shl(rcx, 1);
      mov(qword[r13 + rcx*8 + 8], rdx);
      mov(qword[r13 + rcx*8 + 0], rax);
    }

    // Real block if we made it here
    if (!config.ExecuteBlocksWithCall) {
      jmp(rax);
    } else {
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
  }

  {
    L(ExitBlock);
    ThreadStopHandlerAddress = getCurr<uint64_t>();

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
    ExitFunctionLinkerAddress = getCurr<uint64_t>();
    // {rdi, rsi, rdx}
    mov(rdi, config.ExitFunctionLinkThis);
    mov(rsi, STATE);
    mov(rdx, rax); // rax is set at the block end

    mov(rax, config.ExitFunctionLink);
    call(rax);
    jmp(rax);
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
    PauseReturnInstruction = getCurr<uint64_t>();
    ud2();
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

    // Make sure to adjust the refcounter so we don't clear the cache now
    mov(rax, reinterpret_cast<uint64_t>(&SignalHandlerRefCounter));
    add(dword [rax], 1);

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

  Start = reinterpret_cast<uint64_t>(getCode());
  End = Start + getSize();

  #if ENABLE_JITSYMBOLS
    std::string Name = "Dispatch_" + std::to_string(::gettid());
    CTX->Symbols.Register(Start, End-Start, Name);
  #endif
}

X86Dispatcher::~X86Dispatcher() {

}

#ifdef _M_X86_64

void InterpreterCore::CreateAsmDispatch(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread) {
  DispatcherConfig config;
  config.ExecuteBlocksWithCall = true;

  Dispatcher = new X86Dispatcher(ctx, Thread, config);
  DispatchPtr = Dispatcher->DispatchPtr;
  CallbackPtr = Dispatcher->CallbackPtr;

  // TODO: It feels wrong to initialize this way
  ctx->InterpreterCallbackReturn = Dispatcher->ReturnPtr;
}

#endif

}
