#include "Common/MathUtils.h"
#include "Interface/Core/Interpreter/InterpreterClass.h"
#include <FEXCore/Core/X86Enums.h>

#include <cmath>

#include "aarch64/assembler-aarch64.h"
#include "aarch64/cpu-aarch64.h"
#include "aarch64/disasm-aarch64.h"
#include "aarch64/assembler-aarch64.h"

namespace FEXCore::CPU {
static void SleepThread(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread) {
  --ctx->IdleWaitRefCount;
  ctx->IdleWaitCV.notify_all();

  // Go to sleep
  Thread->StartRunning.Wait();

  Thread->State.RunningEvents.Running = true;
  ++ctx->IdleWaitRefCount;
  ctx->IdleWaitCV.notify_all();
}

using namespace vixl;
using namespace vixl::aarch64;

#define STATE x28
static constexpr size_t MAX_DISPATCHER_CODE_SIZE = 4096;

class DispatchGenerator : public vixl::aarch64::Assembler {
  public:
    DispatchGenerator(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread);
    bool HandleGuestSignal(int Signal, void *info, void *ucontext, SignalDelegator::GuestSigAction *GuestAction, stack_t *GuestStack);
    bool HandleSignalPause(int Signal, void *info, void *ucontext);

  CPUBackend::AsmDispatch DispatchPtr;
  CPUBackend::JITCallback CallbackPtr;

  uint64_t ThreadStopHandlerAddress;
  uint64_t AbsoluteLoopTopAddress;
  uint64_t ThreadPauseHandlerAddress;
  FEXCore::Context::Context *CTX;
  FEXCore::Core::InternalThreadState *State;
  private:
    void StoreThreadState(int Signal, void *ucontext);
    void RestoreThreadState(void *ucontext);
    void PushCalleeSavedRegisters();
    void PopCalleeSavedRegisters();
    void LoadConstant(vixl::aarch64::Register Reg, uint64_t Constant);
    std::stack<uint64_t> SignalFrames;
};

void DispatchGenerator::PushCalleeSavedRegisters() {
  // We need to save pairs of registers
  // We save r19-r30
  MemOperand PairOffset(sp, -16, PreIndex);
  const std::array<std::pair<vixl::aarch64::XRegister, vixl::aarch64::XRegister>, 6> CalleeSaved = {{
    {x19, x20},
    {x21, x22},
    {x23, x24},
    {x25, x26},
    {x27, x28},
    {x29, x30},
  }};

  for (auto &RegPair : CalleeSaved) {
    stp(RegPair.first, RegPair.second, PairOffset);
  }

  // Additionally we need to store the lower 64bits of v8-v15
  // Here's a fun thing, we can use two ST4 instructions to store everything
  // We just need a single sub to sp before that
  const std::array<
    std::tuple<vixl::aarch64::VRegister,
               vixl::aarch64::VRegister,
               vixl::aarch64::VRegister,
               vixl::aarch64::VRegister>, 2> FPRs = {{
    {v8, v9, v10, v11},
    {v12, v13, v14, v15},
  }};

  uint32_t VectorSaveSize = sizeof(uint64_t) * 8;
  sub(sp, sp, VectorSaveSize);
  // SP supporting move
  // We just saved x19 so it is safe
  add(x19, sp, 0);

  MemOperand QuadOffset(x19, 32, PostIndex);
  for (auto &RegQuad : FPRs) {
    st4(std::get<0>(RegQuad).D(),
        std::get<1>(RegQuad).D(),
        std::get<2>(RegQuad).D(),
        std::get<3>(RegQuad).D(),
        0,
        QuadOffset);
  }
}

void DispatchGenerator::PopCalleeSavedRegisters() {
  const std::array<
    std::tuple<vixl::aarch64::VRegister,
               vixl::aarch64::VRegister,
               vixl::aarch64::VRegister,
               vixl::aarch64::VRegister>, 2> FPRs = {{
    {v12, v13, v14, v15},
    {v8, v9, v10, v11},
  }};

  MemOperand QuadOffset(sp, 32, PostIndex);
  for (auto &RegQuad : FPRs) {
    ld4(std::get<0>(RegQuad).D(),
        std::get<1>(RegQuad).D(),
        std::get<2>(RegQuad).D(),
        std::get<3>(RegQuad).D(),
        0,
        QuadOffset);
  }

  MemOperand PairOffset(sp, 16, PostIndex);
  const std::array<std::pair<vixl::aarch64::XRegister, vixl::aarch64::XRegister>, 6> CalleeSaved = {{
    {x29, x30},
    {x27, x28},
    {x25, x26},
    {x23, x24},
    {x21, x22},
    {x19, x20},
  }};

  for (auto &RegPair : CalleeSaved) {
    ldp(RegPair.first, RegPair.second, PairOffset);
  }
}

void DispatchGenerator::LoadConstant(vixl::aarch64::Register Reg, uint64_t Constant) {
  bool Is64Bit = Reg.IsX();
  int Segments = Is64Bit ? 4 : 2;

  movz(Reg, (Constant) & 0xFFFF, 0);
  for (int i = 1; i < Segments; ++i) {
    uint16_t Part = (Constant >> (i * 16)) & 0xFFFF;
    if (Part) {
      movk(Reg, Part, i * 16);
    }
  }
}

DispatchGenerator::DispatchGenerator(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread)
  : vixl::aarch64::Assembler(MAX_DISPATCHER_CODE_SIZE, vixl::aarch64::PositionDependentCode)
  , CTX {ctx}
  , State {Thread} {

  SetAllowAssembler(true);
  auto Buffer = GetBuffer();

  DispatchPtr = Buffer->GetOffsetAddress<CPUBackend::AsmDispatch>(GetCursorOffset());

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

  // Push all the register we need to save
  PushCalleeSavedRegisters();

  // Push our memory base to the correct register
  // Move our thread pointer to the correct register
  // This is passed in to parameter 0 (x0)
  mov(STATE, x0);

  // Save this stack pointer so we can cleanly shutdown the emulation with a long jump
  // regardless of where we were in the stack
  add(x0, sp, 0);
  str(x0, MemOperand(STATE, offsetof(FEXCore::Core::ThreadState, ReturningStackLocation)));

  Label Exit;
  Label LoopTop;
  Label NoBlock;
  Label ThreadPauseHandler;
  bind(&LoopTop);
  AbsoluteLoopTopAddress = GetLabelAddress<uint64_t>(&LoopTop);

  // Load in our RIP
  // Don't modify x2 since it contains our RIP once the block doesn't exist
  ldr(x2, MemOperand(STATE, offsetof(FEXCore::Core::ThreadState, State.rip)));
  auto RipReg = x2;

  // Mask the address by the virtual address size so we can check for aliases
  LoadConstant(x3, Thread->BlockCache->GetVirtualMemorySize() - 1);
  and_(x3, RipReg, x3);

  {
    // This is the block cache lookup routine
    // It matches what is going on it BlockCache.h::FindBlock
    LoadConstant(x0, Thread->BlockCache->GetPagePointer());

    // Offset the address and add to our page pointer
    lsr(x1, x3, 12);

    // Load the pointer from the offset
    ldr(x0, MemOperand(x0, x1, Shift::LSL, 3));

    // If page pointer is zero then we have no block
    cbz(x0, &NoBlock);

    // Steal the page offset
    and_(x1, x3, 0x0FFF);

    // Shift the offset by the size of the block cache entry
    add(x0, x0, Operand(x1, Shift::LSL, (int)log2(sizeof(FEXCore::BlockCache::BlockCacheEntry))));

    // Load the guest address first to ensure it maps to the address we are currently at
    // This fixes aliasing problems
    ldr(x1, MemOperand(x0, offsetof(FEXCore::BlockCache::BlockCacheEntry, GuestCode)));
    cmp(x1, RipReg);
    b(&NoBlock, Condition::ne);

    // Now load the actual host block to execute if we can
    ldr(x1, MemOperand(x0, offsetof(FEXCore::BlockCache::BlockCacheEntry, HostCode)));
    cbz(x1, &NoBlock);

    // If we've made it here then we have a real compiled block
    {
      mov(x0, STATE);
      blr(x1);
    }

    if (CTX->GetGdbServerStatus()) {
      // If we have a gdb server running then run in a less efficient mode that checks if we need to exit
      // This happens when single stepping

      static_assert(sizeof(CTX->Config.RunningMode) == 4, "This is expected to be size of 4");
      ldr(x0, MemOperand(STATE, offsetof(FEXCore::Core::InternalThreadState, CTX)));
      ldr(w0, MemOperand(x0, offsetof(FEXCore::Context::Context, Config.RunningMode)));
      // If the value == 0 then branch to the top
      cbz(x0, &LoopTop);
      // Else we need to pause now
      b(&ThreadPauseHandler);
    }
    else {
      // Unconditionally loop to the top
      // We will only stop on error when compiling a block or signal
      b(&LoopTop);
    }
  }

  {
    bind(&Exit);
    ThreadStopHandlerAddress = Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset());
    PopCalleeSavedRegisters();

    // Return from the function
    // LR is set to the correct return location now
    ret();
  }

  // Need to create the block
  {
    bind(&NoBlock);

    ldr(x0, MemOperand(STATE, offsetof(FEXCore::Core::InternalThreadState, CTX)));
    mov(x1, STATE);

    using ClassPtrType = uintptr_t (FEXCore::Context::Context::*)(FEXCore::Core::InternalThreadState *, uint64_t);
    union PtrCast {
      ClassPtrType ClassPtr;
      uintptr_t Data;
    };

    PtrCast Ptr;
    Ptr.ClassPtr = &FEXCore::Context::Context::CompileBlock;
    LoadConstant(x3, Ptr.Data);

    // X2 contains our guest RIP
    blr(x3); // { CTX, ThreadState, RIP}

    b(&LoopTop);
  }

  {
    bind(&ThreadPauseHandler);
    ThreadPauseHandlerAddress = Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset());
    // We are pausing, this means the frontend should be waiting for this thread to idle
    // We will have faulted and jumped to this location at this point

    // Call our sleep handler
    LoadConstant(x0, reinterpret_cast<uintptr_t>(CTX));
    mov(x1, STATE);
    LoadConstant(x2, reinterpret_cast<uint64_t>(SleepThread));
    blr(x2);

    // XXX: Unsupported atm
    //PauseReturnInstruction = Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset());
    //// Fault to start running again
    //hlt(0);
  }

  {
    CallbackPtr = Buffer->GetOffsetAddress<CPUBackend::JITCallback>(GetCursorOffset());

    // We expect the thunk to have previously pushed the registers it was using
    PushCalleeSavedRegisters();

    // First thing we need to move the thread state pointer back in to our register
    mov(STATE, x0);

    // Now push the callback return trampoline to the guest stack
    // Guest will be misaligned because calling a thunk won't correct the guest's stack once we call the callback from the host
    LoadConstant(x0, CTX->X86CodeGen.CallbackReturn);

    ldr(x2, MemOperand(STATE, offsetof(FEXCore::Core::InternalThreadState, State.State.gregs[X86State::REG_RSP])));
    sub(x2, x2, 16);
    str(x2, MemOperand(STATE, offsetof(FEXCore::Core::InternalThreadState, State.State.gregs[X86State::REG_RSP])));

    // Store the trampoline to the guest stack
    // Guest stack is now correctly misaligned after a regular call instruction
    str(x0, MemOperand(x2));

    // Store RIP to the context state
    str(x1, MemOperand(STATE, offsetof(FEXCore::Core::InternalThreadState, State.State.rip)));

    // Now go back to the regular dispatcher loop
    b(&LoopTop);
  }

  FinalizeCode();
  uint64_t CodeEnd = Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset());
  vixl::aarch64::CPU::EnsureIAndDCacheCoherency(reinterpret_cast<void*>(DispatchPtr), CodeEnd - reinterpret_cast<uint64_t>(DispatchPtr));
  GetBuffer()->SetExecutable();
}


struct HostCTXHeader {
  uint32_t Magic;
  uint32_t Size;
};

constexpr uint32_t FPR_MAGIC = 0x46508001U;

struct HostFPRState {
  HostCTXHeader Head;
  uint32_t FPSR;
  uint32_t FPCR;
  __uint128_t FPRs[32];
};

struct ContextBackup {
  // Host State
  uint64_t GPRs[31];
  uint64_t PrevSP;
  uint64_t PrevPC;
  uint64_t PState;
  uint32_t FPSR;
  uint32_t FPCR;
  __uint128_t FPRs[32];

  // Guest state
  int Signal;
  FEXCore::Core::CPUState GuestState;
};

void DispatchGenerator::StoreThreadState(int Signal, void *ucontext) {
  ucontext_t* _context = (ucontext_t*)ucontext;
  mcontext_t* _mcontext = &_context->uc_mcontext;

  // We can end up getting a signal at any point in our host state
  // Jump to a handler that saves all state so we can safely return
  uint64_t OldSP = _mcontext->sp;
  uintptr_t NewSP = OldSP;

  size_t StackOffset = sizeof(ContextBackup);
  NewSP -= StackOffset;
  NewSP = AlignDown(NewSP, 16);

  ContextBackup *Context = reinterpret_cast<ContextBackup*>(NewSP);
  memcpy(&Context->GPRs[0], &_mcontext->regs[0], 31 * sizeof(uint64_t));
  Context->PrevSP = _mcontext->sp;
  Context->PrevPC = _mcontext->pc;
  Context->PState = _mcontext->pstate;

  // Host FPR state starts at _mcontext->reserved[0];
  HostFPRState *HostState = reinterpret_cast<HostFPRState*>(&_mcontext->__reserved[0]);
  LogMan::Throw::A(HostState->Head.Magic == FPR_MAGIC, "Wrong FPR Magic: 0x%08x", HostState->Head.Magic);
  Context->FPSR = HostState->FPSR;
  Context->FPCR = HostState->FPCR;
  memcpy(&Context->FPRs[0], &HostState->FPRs[0], 32 * sizeof(__uint128_t));

  // Retain the action pointer so we can see it when we return
  Context->Signal = Signal;

  // Save guest state
  // We can't guarantee if registers are in context or host GPRs
  // So we need to save everything
  memcpy(&Context->GuestState, &State->State, sizeof(FEXCore::Core::CPUState));

  // Set the new SP
  _mcontext->sp = NewSP;
}

void DispatchGenerator::RestoreThreadState(void *ucontext) {
  ucontext_t* _context = (ucontext_t*)ucontext;
  mcontext_t* _mcontext = &_context->uc_mcontext;

  uint64_t OldSP = _mcontext->sp;
  uintptr_t NewSP = OldSP;
  ContextBackup *Context = reinterpret_cast<ContextBackup*>(NewSP);

  // First thing, reset the guest state
  memcpy(&State->State, &Context->GuestState, sizeof(FEXCore::Core::CPUState));

  // Now restore host state
  HostFPRState *HostState = reinterpret_cast<HostFPRState*>(&_mcontext->__reserved[0]);
  LogMan::Throw::A(HostState->Head.Magic == FPR_MAGIC, "Wrong FPR Magic: 0x%08x", HostState->Head.Magic);
  memcpy(&HostState->FPRs[0], &Context->FPRs[0], 32 * sizeof(__uint128_t));
  Context->FPCR = HostState->FPCR;
  Context->FPSR = HostState->FPSR;

  // Restore GPRs and other state
  _mcontext->pstate = Context->PState;
  _mcontext->pc = Context->PrevPC;
  _mcontext->sp = Context->PrevSP;
  memcpy(&_mcontext->regs[0], &Context->GPRs[0], 31 * sizeof(uint64_t));

  // Restore the previous signal state
  // This allows recursive signals to properly handle signal masking as we are walking back up the list of signals
  CTX->SignalDelegation.SetCurrentSignal(Context->Signal);
}

bool DispatchGenerator::HandleGuestSignal(int Signal, void *info, void *ucontext, SignalDelegator::GuestSigAction *GuestAction, stack_t *GuestStack) {
  ucontext_t* _context = (ucontext_t*)ucontext;
  mcontext_t* _mcontext = &_context->uc_mcontext;

  StoreThreadState(Signal, ucontext);

  // Set the new PC
  _mcontext->pc = AbsoluteLoopTopAddress;
  // Set x28 (which is our state register) to point to our guest thread data
  _mcontext->regs[28 /* STATE */] = reinterpret_cast<uint64_t>(State);

  State->State.State.gregs[X86State::REG_RDI] = Signal;
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
    _mcontext->pc = ThreadPauseHandlerAddress;

    // Set our state register to point to our guest thread data
    _mcontext->regs[28 /* STATE */] = reinterpret_cast<uint64_t>(State);

    State->SignalReason.store(FEXCore::Core::SIGNALEVENT_NONE);
    return true;
  }

  if (SignalReason == FEXCore::Core::SignalEvent::SIGNALEVENT_RETURN) {
    RestoreThreadState(ucontext);

    State->SignalReason.store(FEXCore::Core::SIGNALEVENT_NONE);
    return true;
  }

  if (SignalReason == FEXCore::Core::SignalEvent::SIGNALEVENT_STOP) {
    ucontext_t* _context = (ucontext_t*)ucontext;
    mcontext_t* _mcontext = &_context->uc_mcontext;

    // Our thread is stopping
    // We don't care about anything at this point
    // Set the stack to our starting location when we entered the JIT and get out safely
    _mcontext->sp = State->State.ReturningStackLocation;

    // Set the new PC
    _mcontext->pc = ThreadStopHandlerAddress;

    State->SignalReason.store(FEXCore::Core::SIGNALEVENT_NONE);
    return true;
  }

  return false;
}

void InterpreterCore::CreateAsmDispatch(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread) {
  Generator = new DispatchGenerator(ctx, Thread);
  DispatchPtr = Generator->DispatchPtr;
  CallbackPtr = Generator->CallbackPtr;
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
