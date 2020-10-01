#include "Interface/Context/Context.h"

#include "Interface/Core/JIT/x86_64/JITClass.h"
#include "Interface/Core/InternalThreadState.h"

#include "Interface/IR/Passes/RegisterAllocationPass.h"

#include <FEXCore/Core/X86Enums.h>

#include <cmath>

// #define DEBUG_RA 1
// #define DEBUG_CYCLES

namespace FEXCore::CPU {

CodeBuffer AllocateNewCodeBuffer(size_t Size) {
  CodeBuffer Buffer;
  Buffer.Size = Size;
  Buffer.Ptr = static_cast<uint8_t*>(
               mmap(nullptr,
                    Buffer.Size,
                    PROT_READ | PROT_WRITE | PROT_EXEC,
                    MAP_PRIVATE | MAP_ANONYMOUS,
                    -1, 0));
  LogMan::Throw::A(!!Buffer.Ptr, "Couldn't allocate code buffer");
  return Buffer;
}

void FreeCodeBuffer(CodeBuffer Buffer) {
  munmap(Buffer.Ptr, Buffer.Size);
}

}

namespace FEXCore::CPU {
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

void JITCore::StoreThreadState(int Signal, void *ucontext) {
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
  memcpy(&Context->GuestState, &ThreadState->State, sizeof(FEXCore::Core::CPUState));

  // Set the new SP
  _mcontext->gregs[REG_RSP] = NewSP;

  SignalFrames.push(NewSP);
}

void JITCore::RestoreThreadState(void *ucontext) {
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
  memcpy(&ThreadState->State, &Context->GuestState, sizeof(FEXCore::Core::CPUState));

  // Now restore host state

  // Copy the GPRs
  memcpy(&_mcontext->gregs[0], &Context->GPRs[0], NGREG * sizeof(_mcontext->gregs[0]));
  // Copy the FPRState
  memcpy(_mcontext->fpregs, &Context->FPRState, sizeof(_libc_fpstate));

  // Restore the previous signal state
  // This allows recursive signals to properly handle signal masking as we are walking back up the list of signals
  CTX->SignalDelegation.SetCurrentSignal(Context->Signal);
}

bool JITCore::HandleGuestSignal(int Signal, void *info, void *ucontext, SignalDelegator::GuestSigAction *GuestAction, stack_t *GuestStack) {
  ucontext_t* _context = (ucontext_t*)ucontext;
  mcontext_t* _mcontext = &_context->uc_mcontext;

  StoreThreadState(Signal, ucontext);

  // Set the new PC
  _mcontext->gregs[REG_RIP] = AbsoluteLoopTopAddress;
  // Set our state register to point to our guest thread data
  _mcontext->gregs[REG_R14] = reinterpret_cast<uint64_t>(ThreadState);

  // Ref count our faults
  // We use this to track if it is safe to clear cache
  ++SignalHandlerRefCounter;

  uint64_t OldGuestSP = ThreadState->State.State.gregs[X86State::REG_RSP];
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

  ThreadState->State.State.gregs[X86State::REG_RDI] = Signal;

  if (GuestAction->sa_flags & SA_SIGINFO) {
    // XXX: siginfo_t(RSI), ucontext (RDX)
    ThreadState->State.State.gregs[X86State::REG_RSI] = 0;
    ThreadState->State.State.gregs[X86State::REG_RDX] = 0;
    ThreadState->State.State.rip = reinterpret_cast<uint64_t>(GuestAction->sigaction_handler.sigaction);
  }
  else {
    ThreadState->State.State.rip = reinterpret_cast<uint64_t>(GuestAction->sigaction_handler.handler);
  }

  // Set up the new SP for stack handling
  NewGuestSP -= 8;
  *(uint64_t*)NewGuestSP = CTX->X86CodeGen.SignalReturn;
  ThreadState->State.State.gregs[X86State::REG_RSP] = NewGuestSP;

  return true;
}

bool JITCore::HandleSIGILL(int Signal, void *info, void *ucontext) {
  ucontext_t* _context = (ucontext_t*)ucontext;
  mcontext_t* _mcontext = &_context->uc_mcontext;

  if (_mcontext->gregs[REG_RIP] == SignalHandlerReturnAddress) {
    RestoreThreadState(ucontext);

    // Ref count our faults
    // We use this to track if it is safe to clear cache
    --SignalHandlerRefCounter;
    return true;
  }

  if (_mcontext->gregs[REG_RIP] == PauseReturnInstruction) {
    RestoreThreadState(ucontext);

    // Ref count our faults
    // We use this to track if it is safe to clear cache
    --SignalHandlerRefCounter;
    return true;
  }

  return false;
}

bool JITCore::HandleSignalPause(int Signal, void *info, void *ucontext) {
  FEXCore::Core::SignalEvent SignalReason = ThreadState->SignalReason.load();

  if (SignalReason == FEXCore::Core::SignalEvent::SIGNALEVENT_PAUSE) {
    ucontext_t* _context = (ucontext_t*)ucontext;
    mcontext_t* _mcontext = &_context->uc_mcontext;

    // Store our thread state so we can come back to this
    StoreThreadState(Signal, ucontext);

    // Set the new PC
    _mcontext->gregs[REG_RIP] = ThreadPauseHandlerAddress;

    // Set our state register to point to our guest thread data
    _mcontext->gregs[REG_R14] = reinterpret_cast<uint64_t>(ThreadState);

    // Ref count our faults
    // We use this to track if it is safe to clear cache
    ++SignalHandlerRefCounter;

    ThreadState->SignalReason.store(FEXCore::Core::SIGNALEVENT_NONE);
    return true;
  }

  if (SignalReason == FEXCore::Core::SignalEvent::SIGNALEVENT_RETURN) {
    RestoreThreadState(ucontext);

    // Ref count our faults
    // We use this to track if it is safe to clear cache
    --SignalHandlerRefCounter;

    ThreadState->SignalReason.store(FEXCore::Core::SIGNALEVENT_NONE);
    return true;
  }

  if (SignalReason == FEXCore::Core::SignalEvent::SIGNALEVENT_STOP) {
    ucontext_t* _context = (ucontext_t*)ucontext;
    mcontext_t* _mcontext = &_context->uc_mcontext;

    // Our thread is stopping
    // We don't care about anything at this point
    // Set the stack to our starting location when we entered the JIT and get out safely
    _mcontext->gregs[REG_RSP] = ThreadState->State.ReturningStackLocation;

    // Our ref counting doesn't matter anymore
    SignalHandlerRefCounter = 0;

    // Set the new PC
    _mcontext->gregs[REG_RIP] = ThreadStopHandlerAddress;

    ThreadState->SignalReason.store(FEXCore::Core::SIGNALEVENT_NONE);
    return true;
  }

  return false;
}

void JITCore::Op_Unhandled(FEXCore::IR::IROp_Header *IROp, uint32_t Node) {
  auto Name = FEXCore::IR::GetName(IROp->Op);
  LogMan::Msg::A("Unhandled IR Op: %s", std::string(Name).c_str());
}

void JITCore::Op_NoOp(FEXCore::IR::IROp_Header *IROp, uint32_t Node) {
}

JITCore::JITCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread, CodeBuffer Buffer)
  : CodeGenerator(Buffer.Size, Buffer.Ptr, nullptr)
  , CTX {ctx}
  , ThreadState {Thread}
  , InitialCodeBuffer {Buffer}
{
  CurrentCodeBuffer = &InitialCodeBuffer;

  RAPass = Thread->PassManager->GetRAPass();

  RAPass->AllocateRegisterSet(RegisterCount, RegisterClasses);
  RAPass->AddRegisters(FEXCore::IR::GPRClass, NumGPRs);
  RAPass->AddRegisters(FEXCore::IR::FPRClass, NumXMMs);
  RAPass->AddRegisters(FEXCore::IR::GPRPairClass, NumGPRPairs);

  RAPass->AllocateRegisterConflicts(FEXCore::IR::GPRClass, NumGPRs);
  RAPass->AllocateRegisterConflicts(FEXCore::IR::GPRPairClass, NumGPRs);

  for (uint32_t i = 0; i < NumGPRPairs; ++i) {
    RAPass->AddRegisterConflict(FEXCore::IR::GPRClass, i * 2,     FEXCore::IR::GPRPairClass, i);
    RAPass->AddRegisterConflict(FEXCore::IR::GPRClass, i * 2 + 1, FEXCore::IR::GPRPairClass, i);
  }

  for (uint32_t i = 0; i < FEXCore::IR::IROps::OP_LAST + 1; ++i) {
    OpHandlers[i] = &JITCore::Op_Unhandled;
  }

  RegisterALUHandlers();
  RegisterAtomicHandlers();
  RegisterBranchHandlers();
  RegisterConversionHandlers();
  RegisterFlagHandlers();
  RegisterMemoryHandlers();
  RegisterMiscHandlers();
  RegisterMoveHandlers();
  RegisterVectorHandlers();
  RegisterEncryptionHandlers();

  CreateCustomDispatch(Thread);

  // This will register the host signal handler per thread, which is fine
  CTX->SignalDelegation.RegisterHostSignalHandler(SIGILL, [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) -> bool {
    JITCore *Core = reinterpret_cast<JITCore*>(Thread->CPUBackend.get());
    return Core->HandleSIGILL(Signal, info, ucontext);
  });

  CTX->SignalDelegation.RegisterHostSignalHandler(SignalDelegator::SIGNAL_FOR_PAUSE, [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) -> bool {
    JITCore *Core = reinterpret_cast<JITCore*>(Thread->CPUBackend.get());
    return Core->HandleSignalPause(Signal, info, ucontext);
  });

  auto GuestSignalHandler = [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext, SignalDelegator::GuestSigAction *GuestAction, stack_t *GuestStack) -> bool {
    JITCore *Core = reinterpret_cast<JITCore*>(Thread->CPUBackend.get());
    return Core->HandleGuestSignal(Signal, info, ucontext, GuestAction, GuestStack);
  };

  for (uint32_t Signal = 0; Signal < SignalDelegator::MAX_SIGNALS; ++Signal) {
    CTX->SignalDelegation.RegisterHostSignalHandlerForGuest(Signal, GuestSignalHandler);
  }
}

JITCore::~JITCore() {
  LogMan::Msg::D("Used %ld bytes for compiling", getCurr<uintptr_t>() - getCode<uintptr_t>());
  FreeCodeBuffer(DispatcherCodeBuffer);
  FreeCodeBuffer(InitialCodeBuffer);
}

void JITCore::ClearCache() {
  if (SignalHandlerRefCounter == 0) {
    if (!CodeBuffers.empty()) {
      // If we have more than one code buffer we are tracking then walk them and delete
      // This is a cleanup step
      for (auto CodeBuffer : CodeBuffers) {
        FreeCodeBuffer(CodeBuffer);
      }
      CodeBuffers.clear();

      // Set the current code buffer to the initial
      setNewBuffer(InitialCodeBuffer.Ptr, InitialCodeBuffer.Size);
      CurrentCodeBuffer = &InitialCodeBuffer;
    }

    if (CurrentCodeBuffer->Size == MAX_CODE_SIZE) {
      // Rewind to the start of the code cache start
      reset();
    }
    else {
      // Resize the code buffer and reallocate our code size
      CurrentCodeBuffer->Size *= 1.5;
      CurrentCodeBuffer->Size = std::min(CurrentCodeBuffer->Size, MAX_CODE_SIZE);

      FreeCodeBuffer(InitialCodeBuffer);
      InitialCodeBuffer = AllocateNewCodeBuffer(CurrentCodeBuffer->Size);
      setNewBuffer(InitialCodeBuffer.Ptr, InitialCodeBuffer.Size);
    }
  }
  else {
    // We have signal handlers that have generated code
    // This means that we can not safely clear the code at this point in time
    // Allocate some new code buffers that we can switch over to instead
    auto NewCodeBuffer = AllocateNewCodeBuffer(JITCore::INITIAL_CODE_SIZE);
    CurrentCodeBuffer->Size = JITCore::INITIAL_CODE_SIZE;
    EmplaceNewCodeBuffer(NewCodeBuffer);
    setNewBuffer(NewCodeBuffer.Ptr, NewCodeBuffer.Size);
  }
}

uint32_t JITCore::GetPhys(uint32_t Node) {
  uint64_t Reg = RAPass->GetNodeRegister(Node);

  if ((uint32_t)Reg != ~0U)
    return Reg;
  else
    LogMan::Msg::A("Couldn't Allocate register for node: ssa%d. Class: %d", Node, Reg >> 32);

  return ~0U;
}

template<uint8_t RAType>
Xbyak::Reg JITCore::GetSrc(uint32_t Node) {
  // rax, rcx, rdx, rsi, r8, r9,
  // r10
  // Callee Saved
  // rbx, rbp, r12, r13, r14, r15
  uint32_t Reg = GetPhys(Node);
  if (RAType == RA_64)
    return RA64[Reg].cvt64();
  else if (RAType == RA_XMM)
    return RAXMM[Reg];
  else if (RAType == RA_32)
    return RA64[Reg].cvt32();
  else if (RAType == RA_16)
    return RA64[Reg].cvt16();
  else if (RAType == RA_8)
    return RA64[Reg].cvt8();
}

template
Xbyak::Reg JITCore::GetSrc<JITCore::RA_64>(uint32_t Node);

template
Xbyak::Reg JITCore::GetSrc<JITCore::RA_32>(uint32_t Node);

template
Xbyak::Reg JITCore::GetSrc<JITCore::RA_16>(uint32_t Node);

template
Xbyak::Reg JITCore::GetSrc<JITCore::RA_8>(uint32_t Node);

Xbyak::Xmm JITCore::GetSrc(uint32_t Node) {
  uint32_t Reg = GetPhys(Node);
  return RAXMM_x[Reg];
}

template<uint8_t RAType>
Xbyak::Reg JITCore::GetDst(uint32_t Node) {
  uint32_t Reg = GetPhys(Node);
  if (RAType == RA_64)
    return RA64[Reg].cvt64();
  else if (RAType == RA_XMM)
    return RAXMM[Reg];
  else if (RAType == RA_32)
    return RA64[Reg].cvt32();
  else if (RAType == RA_16)
    return RA64[Reg].cvt16();
  else if (RAType == RA_8)
    return RA64[Reg].cvt8();
}

template
Xbyak::Reg JITCore::GetDst<JITCore::RA_64>(uint32_t Node);

template
Xbyak::Reg JITCore::GetDst<JITCore::RA_32>(uint32_t Node);

template
Xbyak::Reg JITCore::GetDst<JITCore::RA_16>(uint32_t Node);

template
Xbyak::Reg JITCore::GetDst<JITCore::RA_8>(uint32_t Node);

template<uint8_t RAType>
std::pair<Xbyak::Reg, Xbyak::Reg> JITCore::GetSrcPair(uint32_t Node) {
  uint32_t Reg = GetPhys(Node);
  if (RAType == RA_64)
    return RA64Pair[Reg];
  else if (RAType == RA_32)
    return {RA64Pair[Reg].first.cvt32(), RA64Pair[Reg].second.cvt32()};
}

template
std::pair<Xbyak::Reg, Xbyak::Reg> JITCore::GetSrcPair<JITCore::RA_64>(uint32_t Node);

template
std::pair<Xbyak::Reg, Xbyak::Reg> JITCore::GetSrcPair<JITCore::RA_32>(uint32_t Node);

Xbyak::Xmm JITCore::GetDst(uint32_t Node) {
  uint32_t Reg = GetPhys(Node);
  return RAXMM_x[Reg];
}

void *JITCore::CompileCode([[maybe_unused]] FEXCore::IR::IRListView<true> const *IR, [[maybe_unused]] FEXCore::Core::DebugData *DebugData) {
  JumpTargets.clear();
  uint32_t SSACount = IR->GetSSACount();

  auto HeaderOp = IR->GetHeader();
  if (HeaderOp->ShouldInterpret) {
    return InterpreterFallbackHelperAddress;
  }

  // Fairly excessive buffer range to make sure we don't overflow
  uint32_t BufferRange = SSACount * 16;
  if ((getSize() + BufferRange) > CurrentCodeBuffer->Size) {
    LogMan::Msg::D("Gotta clear code cache: 0x%lx is too close to 0x%lx", getSize(), CurrentCodeBuffer->Size);
    ThreadState->CTX->ClearCodeCache(ThreadState, HeaderOp->Entry);
  }

	void *Entry = getCurr<void*>();

  LogMan::Throw::A(RAPass->HasFullRA(), "Needs RA");

  SpillSlots = RAPass->SpillSlots();

  if (SpillSlots) {
    sub(rsp, SpillSlots * 16 + 8);
  }
  else {
    sub(rsp, 8);
  }

#ifdef BLOCKSTATS
  BlockSamplingData::BlockData *SamplingData = CTX->BlockData->GetBlockData(HeaderOp->Entry);
  if (GetSamplingData) {
    mov(rcx, reinterpret_cast<uintptr_t>(SamplingData));
    rdtsc();
    shl(rdx, 32);
    or(rax, rdx);
    mov(qword [rcx + offsetof(BlockSamplingData::BlockData, Start)], rax);
  }

  auto ExitBlock = [&]() {
    if (GetSamplingData) {
      mov(rcx, reinterpret_cast<uintptr_t>(SamplingData));
      // Get time
      rdtsc();
      shl(rdx, 32);
      or(rax, rdx);

      // Calculate time spent in block
      mov(rdx, qword [rcx + offsetof(BlockSamplingData::BlockData, Start)]);
      sub(rax, rdx);

      // Add time to total time
      add(qword [rcx + offsetof(BlockSamplingData::BlockData, TotalTime)], rax);

      // Increment call count
      inc(qword [rcx + offsetof(BlockSamplingData::BlockData, TotalCalls)]);

      // Calculate min
      mov(rdx, qword [rcx + offsetof(BlockSamplingData::BlockData, Min)]);
      cmp(rdx, rax);
      cmova(rdx, rax);
      mov(qword [rcx + offsetof(BlockSamplingData::BlockData, Min)], rdx);

      // Calculate max
      mov(rdx, qword [rcx + offsetof(BlockSamplingData::BlockData, Max)]);
      cmp(rdx, rax);
      cmovb(rdx, rax);
      mov(qword [rcx + offsetof(BlockSamplingData::BlockData, Max)], rdx);
    }
  };
#endif

  for (auto [BlockNode, BlockHeader] : IR->GetBlocks()) {
    using namespace FEXCore::IR;
    {
      auto BlockIROp = BlockHeader->CW<IROp_CodeBlock>();
      LogMan::Throw::A(BlockIROp->Header.Op == IR::OP_CODEBLOCK, "IR type failed to be a code block");

      uint32_t Node = IR->GetID(BlockNode);
      auto IsTarget = JumpTargets.find(Node);
      if (IsTarget == JumpTargets.end()) {
        IsTarget = JumpTargets.try_emplace(Node).first;
      }

      L(IsTarget->second);
    }

    for (auto [CodeNode, IROp] : IR->GetCode(BlockNode)) {
      #ifdef DEBUG_RA
      if (IROp->Op != IR::OP_BEGINBLOCK &&
          IROp->Op != IR::OP_CONDJUMP &&
          IROp->Op != IR::OP_JUMP) {
        std::stringstream Inst;
        auto Name = FEXCore::IR::GetName(IROp->Op);

        if (IROp->HasDest) {
          uint64_t PhysReg = RAPass->GetNodeRegister(Node);
          if (PhysReg >= GPRPairBase)
            Inst << "\tPair" << GetPhys(Node) << " = " << Name << " ";
          else if (PhysReg >= XMMBase)
            Inst << "\tXMM" << GetPhys(Node) << " = " << Name << " ";
          else
            Inst << "\tReg" << GetPhys(Node) << " = " << Name << " ";
        }
        else {
          Inst << "\t" << Name << " ";
        }

        uint8_t NumArgs = IR::GetArgs(IROp->Op);
        for (uint8_t i = 0; i < NumArgs; ++i) {
          uint32_t ArgNode = IROp->Args[i].ID();
          uint64_t PhysReg = RAPass->GetNodeRegister(ArgNode);
          if (PhysReg >= GPRPairBase)
            Inst << "Pair" << GetPhys(ArgNode) << (i + 1 == NumArgs ? "" : ", ");
          else if (PhysReg >= XMMBase)
            Inst << "XMM" << GetPhys(ArgNode) << (i + 1 == NumArgs ? "" : ", ");
          else
            Inst << "Reg" << GetPhys(ArgNode) << (i + 1 == NumArgs ? "" : ", ");
        }

        LogMan::Msg::D("%s", Inst.str().c_str());
      }
      #endif
      uint32_t ID = IR->GetID(CodeNode);

      // Execute handler
      OpHandler Handler = OpHandlers[IROp->Op];
      (this->*Handler)(IROp, ID);
    }
  }

  void *Exit = getCurr<void*>();

  ready();

  if (DebugData) {
    DebugData->HostCodeSize = reinterpret_cast<uintptr_t>(Exit) - reinterpret_cast<uintptr_t>(Entry);
  }
  // LogMan::Msg::D("RIP: 0x%lx ; disas %p,%p", HeaderOp->Entry, Entry, Exit);
  return Entry;
}

static void SleepThread(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread) {
  --ctx->IdleWaitRefCount;
  ctx->IdleWaitCV.notify_all();

  // Go to sleep
  Thread->StartRunning.Wait();

  Thread->State.RunningEvents.Running = true;
  ++ctx->IdleWaitRefCount;
  ctx->IdleWaitCV.notify_all();
}

void JITCore::CreateCustomDispatch(FEXCore::Core::InternalThreadState *Thread) {
  DispatcherCodeBuffer = AllocateNewCodeBuffer(MAX_DISPATCHER_CODE_SIZE);
  setNewBuffer(DispatcherCodeBuffer.Ptr, DispatcherCodeBuffer.Size);

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
  mov(qword [STATE + offsetof(FEXCore::Core::ThreadState, ReturningStackLocation)], rsp);

  Label LoopTop;
  Label NoBlock;

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
    and(rax, 0x0FFF);

    shl(rax, (int)log2(sizeof(FEXCore::BlockCache::BlockCacheEntry)));

    // check for aliasing
    mov(rcx, qword [rdi + rax + 8]);
    cmp(rcx, rdx);
    jne(NoBlock);

    // Load the block pointer
    mov(rax, qword [rdi + rax]);

    cmp(rax, 0);
    je(NoBlock);

    // Real block if we made it here
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

  Label FallbackCore;
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

    // RAX contains nulptr or block ptr here
    cmp(rax, 0);
    je(FallbackCore);
    // rdx already contains RIP here
    jmp(LoopTop);
  }

  {
    L(FallbackCore);
    ud2();
  }

  {
    // Interpreter fallback helper code
    InterpreterFallbackHelperAddress = getCurr<void*>();
    // This will get called so our stack is now misaligned
    sub(rsp, 8);
    mov(rdi, STATE);
    mov(rax, reinterpret_cast<uint64_t>(ThreadState->IntBackend->CompileCode(nullptr, nullptr)));

    call(rax);

    // Adjust the stack to remove the alignment and also the return address
    // We will have been called from the ASM dispatcher, so we know where we came from
    add(rsp, 16);

    jmp(LoopTop);
  }

  {
    // Signal return handler
    SignalHandlerReturnAddress = getCurr<uint64_t>();

    ud2();
  }

  {
    // Pause handler
    ThreadPauseHandlerAddress = getCurr<uint64_t>();
    L(ThreadPauseHandler);

    mov(rdi, reinterpret_cast<uintptr_t>(CTX));
    mov(rsi, STATE);
    mov(rax, reinterpret_cast<uint64_t>(SleepThread));

    call(rax);

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
    sub(qword [STATE + offsetof(FEXCore::Core::InternalThreadState, State.State.gregs[X86State::REG_RSP])], 16);
    mov(rbx, qword [STATE + offsetof(FEXCore::Core::InternalThreadState, State.State.gregs[X86State::REG_RSP])]);
    mov(qword [rbx], rax);

    // Store RIP to the context state
    mov(qword [STATE + offsetof(FEXCore::Core::InternalThreadState, State.State.rip)], rsi);

    // Back to the loop top now
    jmp(LoopTop);
  }

  ready();

  void *Exit = getCurr<void*>();
  LogMan::Msg::D("Dispatcher : disas %p,%p", DispatchPtr, Exit);

  setNewBuffer(InitialCodeBuffer.Ptr, InitialCodeBuffer.Size);
}

FEXCore::CPU::CPUBackend *CreateJITCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread) {
  return new JITCore(ctx, Thread, AllocateNewCodeBuffer(JITCore::INITIAL_CODE_SIZE));
}
}
