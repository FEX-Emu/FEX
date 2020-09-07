#include "Interface/Context/Context.h"

#include "Interface/Core/JIT/x86_64/JITClass.h"
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
static void PrintValue(uint64_t Value) {
  LogMan::Msg::D("Value: 0x%lx", Value);
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


JITCore::JITCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread, CodeBuffer Buffer)
  : CodeGenerator(Buffer.Size, Buffer.Ptr, nullptr)
  , CTX {ctx}
  , ThreadState {Thread}
  , InitialCodeBuffer {Buffer}
{
  CurrentCodeBuffer = &InitialCodeBuffer;
  Stack.resize(9000 * 16 * 64);

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

template<uint8_t RAType>
std::pair<Xbyak::Reg, Xbyak::Reg> JITCore::GetSrcPair(uint32_t Node) {
  uint32_t Reg = GetPhys(Node);
  if (RAType == RA_64)
    return RA64Pair[Reg];
  else if (RAType == RA_32)
    return {RA64Pair[Reg].first.cvt32(), RA64Pair[Reg].second.cvt32()};
}

Xbyak::Xmm JITCore::GetDst(uint32_t Node) {
  uint32_t Reg = GetPhys(Node);
  return RAXMM_x[Reg];
}

void *JITCore::CompileCode([[maybe_unused]] FEXCore::IR::IRListView<true> const *IR, [[maybe_unused]] FEXCore::Core::DebugData *DebugData) {
  JumpTargets.clear();
  CurrentIR = IR;
  uint32_t SSACount = CurrentIR->GetSSACount();
  uintptr_t ListBegin = CurrentIR->GetListData();
  uintptr_t DataBegin = CurrentIR->GetData();

  auto HeaderIterator = CurrentIR->begin();
  IR::OrderedNodeWrapper *HeaderNodeWrapper = HeaderIterator();
  IR::OrderedNode *HeaderNode = HeaderNodeWrapper->GetNode(ListBegin);
  auto HeaderOp = HeaderNode->Op(DataBegin)->CW<FEXCore::IR::IROp_IRHeader>();
  LogMan::Throw::A(HeaderOp->Header.Op == IR::OP_IRHEADER, "First op wasn't IRHeader");

  if (HeaderOp->ShouldInterpret) {
    return InterpreterFallbackHelperAddress;
  }

  // Fairly excessive buffer range to make sure we don't overflow
  uint32_t BufferRange = SSACount * 16;
  if ((getSize() + BufferRange) > CurrentCodeBuffer->Size) {
    LogMan::Msg::D("Gotta clear code cache: 0x%lx is too close to 0x%lx", getSize(), CurrentCodeBuffer->Size);
    ThreadState->CTX->ClearCodeCache(ThreadState, HeaderOp->Entry);
  }

  uint64_t ListStackSize = SSACount * 16;
  if (ListStackSize > Stack.size()) {
    Stack.resize(ListStackSize);
  }

	void *Entry = getCurr<void*>();

  LogMan::Throw::A(RAPass->HasFullRA(), "Needs RA");

  uint32_t SpillSlots = RAPass->SpillSlots();

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

  auto RegularExit = [&]() {
    if (SpillSlots) {
      add(rsp, SpillSlots * 16 + 8);
    }
    else {
      add(rsp, 8);
    }

#ifdef BLOCKSTATS
    ExitBlock();
#endif
    ret();
  };

  IR::OrderedNode *BlockNode = HeaderOp->Blocks.GetNode(ListBegin);
  while (1) {
    using namespace FEXCore::IR;
    auto BlockIROp = BlockNode->Op(DataBegin)->CW<FEXCore::IR::IROp_CodeBlock>();
    LogMan::Throw::A(BlockIROp->Header.Op == IR::OP_CODEBLOCK, "IR type failed to be a code block");

    // We grab these nodes this way so we can iterate easily
    auto CodeBegin = CurrentIR->at(BlockIROp->Begin);
    auto CodeLast = CurrentIR->at(BlockIROp->Last);

    {
      uint32_t Node = BlockNode->Wrapped(ListBegin).ID();
      auto IsTarget = JumpTargets.find(Node);
      if (IsTarget == JumpTargets.end()) {
        IsTarget = JumpTargets.try_emplace(Node).first;
      }

      L(IsTarget->second);
    }

    while (1) {
      OrderedNodeWrapper *WrapperOp = CodeBegin();
      OrderedNode *RealNode = WrapperOp->GetNode(ListBegin);
      FEXCore::IR::IROp_Header *IROp = RealNode->Op(DataBegin);
      uint8_t OpSize = IROp->Size;
      uint32_t Node = WrapperOp->ID();

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

      switch (IROp->Op) {
        case IR::OP_BEGINBLOCK: {
          auto IsTarget = JumpTargets.find(Node);
          if (IsTarget == JumpTargets.end()) {
            IsTarget = JumpTargets.try_emplace(Node).first;
          }
          else {
          }

          L(IsTarget->second);
          break;
        }
        case IR::OP_ENDBLOCK: {
          auto Op = IROp->C<IR::IROp_EndBlock>();
          if (Op->RIPIncrement) {
            add(qword [STATE + offsetof(FEXCore::Core::CPUState, rip)], Op->RIPIncrement);
          }
          break;
        }
        case IR::OP_EXITFUNCTION: {
          RegularExit();
          break;
        }
        case IR::OP_SIGNALRETURN: {
          // Adjust the stack first for a regular return
          if (SpillSlots) {
            add(rsp, SpillSlots * 16 + 8 + 8); // + 8 to consume return address
          }
          else {
            add(rsp, 8 + 8); // + 8 to consume return address
          }

          mov(TMP1, SignalHandlerReturnAddress);
          jmp(TMP1);
          break;
        }
        case IR::OP_CALLBACKRETURN: {
          // Adjust the stack first for a regular return
          if (SpillSlots) {
            add(rsp, SpillSlots * 16 + 8 + 8); // + 8 to consume return address
          }
          else {
            add(rsp, 8 + 8); // + 8 to consume return address
          }

          // Make sure to adjust the refcounter so we don't clear the cache now
          mov(rax, reinterpret_cast<uint64_t>(&SignalHandlerRefCounter));
          sub(dword [rax], 1);

          // We need to adjust an additional 8 bytes to get back to the original "misaligned" RSP state
          add(qword [STATE + offsetof(FEXCore::Core::InternalThreadState, State.State.gregs[X86State::REG_RSP])], 8);

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
          break;
        }
        case IR::OP_BREAK: {
          auto Op = IROp->C<IR::IROp_Break>();
          switch (Op->Reason) {
            case 0: // Hard fault
            case 5: // Guest ud2
              ud2();
            break;
            case 4: { // HLT
              // Time to quit
              // Set our stack to the starting stack location
              mov(rsp, qword [STATE + offsetof(FEXCore::Core::ThreadState, ReturningStackLocation)]);

              // Now we need to jump to the thread stop handler
              mov(TMP1, ThreadStopHandlerAddress);
              jmp(TMP1);
              break;
            }
            case 6: // INT3
            {
              if (CTX->GetGdbServerStatus()) {
                // Adjust the stack first for a regular return
                if (SpillSlots) {
                  add(rsp, SpillSlots * 16 + 8);
                }
                else {
                  add(rsp, 8);
                }

                // This jump target needs to be a constant offset here
                mov(TMP1, ThreadPauseHandlerAddress);
                jmp(TMP1);
              }
              else {
                // If we don't have a gdb server attached then....crash?
                // Treat this case like HLT
                mov(rsp, qword [STATE + offsetof(FEXCore::Core::ThreadState, ReturningStackLocation)]);

                // Now we need to jump to the thread stop handler
                mov(TMP1, ThreadStopHandlerAddress);
                jmp(TMP1);
              }
            break;
            }
            default: LogMan::Msg::A("Unknown Break reason: %d", Op->Reason);
          }
          break;
        }
        case IR::OP_JUMP: {
          auto Op = IROp->C<IR::IROp_Jump>();

          Label *TargetLabel;
          auto IsTarget = JumpTargets.find(Op->Header.Args[0].ID());
          if (IsTarget == JumpTargets.end()) {
            TargetLabel = &JumpTargets.try_emplace(Op->Header.Args[0].ID()).first->second;
          }
          else {
            TargetLabel = &IsTarget->second;
          }

          jmp(*TargetLabel, T_NEAR);

          break;
        }
        case IR::OP_CONDJUMP: {
          auto Op = IROp->C<IR::IROp_CondJump>();

          Label *TrueTargetLabel;
          Label *FalseTargetLabel;

          auto TrueIter = JumpTargets.find(Op->Header.Args[1].ID());
          auto FalseIter = JumpTargets.find(Op->Header.Args[2].ID());

          if (TrueIter == JumpTargets.end()) {
            TrueTargetLabel = &JumpTargets.try_emplace(Op->Header.Args[1].ID()).first->second;
          }
          else {
            TrueTargetLabel = &TrueIter->second;
          }

          if (FalseIter == JumpTargets.end()) {
            FalseTargetLabel = &JumpTargets.try_emplace(Op->Header.Args[2].ID()).first->second;
          }
          else {
            FalseTargetLabel = &FalseIter->second;
          }

          // Take branch if (src != 0)
          cmp(GetSrc<RA_64>(Op->Header.Args[0].ID()), 0);
          jne(*TrueTargetLabel, T_NEAR);
          jmp(*FalseTargetLabel, T_NEAR);
          break;
        }
        case IR::OP_LOADCONTEXT: {
          auto Op = IROp->C<IR::IROp_LoadContext>();
          if (Op->Class.Val == 0) {
            switch (OpSize) {
            case 1: {
              movzx(GetDst<RA_32>(Node), byte [STATE + Op->Offset]);
            }
            break;
            case 2: {
              movzx(GetDst<RA_32>(Node), word [STATE + Op->Offset]);
            }
            break;
            case 4: {
              mov(GetDst<RA_32>(Node), dword [STATE + Op->Offset]);
            }
            break;
            case 8: {
              mov(GetDst<RA_64>(Node), qword [STATE + Op->Offset]);
            }
            break;
            case 16: {
              LogMan::Msg::A("Invalid GPR load of size 16");
            }
            break;
            default:  LogMan::Msg::A("Unhandled LoadContext size: %d", OpSize);
            }
          }
          else {
            switch (OpSize) {
            case 1: {
              movzx(rax, byte [STATE + Op->Offset]);
              vmovq(GetDst(Node), rax);
            }
            break;
            case 2: {
              movzx(rax, word [STATE + Op->Offset]);
              vmovq(GetDst(Node), rax);
            }
            break;
            case 4: {
              vmovd(GetDst(Node), dword [STATE + Op->Offset]);
            }
            break;
            case 8: {
              vmovq(GetDst(Node), qword [STATE + Op->Offset]);
            }
            break;
            case 16: {
              if (Op->Offset % 16 == 0)
                movaps(GetDst(Node), xword [STATE + Op->Offset]);
              else
                movups(GetDst(Node), xword [STATE + Op->Offset]);
            }
            break;
            default:  LogMan::Msg::A("Unhandled LoadContext size: %d", OpSize);
            }
          }
          break;
        }
        case IR::OP_LOADCONTEXTINDEXED: {
          auto Op = IROp->C<IR::IROp_LoadContextIndexed>();
          size_t size = Op->Size;
          Reg index = GetSrc<RA_64>(Op->Header.Args[0].ID());

          if (Op->Class.Val == 0) {
            switch (Op->Stride) {
            case 1:
            case 2:
            case 4:
            case 8: {
              lea(rax, dword [STATE + Op->BaseOffset]);
              switch (size) {
              case 1:
                movzx(GetDst<RA_32>(Node), byte [rax + index * Op->Stride]);
                break;
              case 2:
                movzx(GetDst<RA_32>(Node), word [rax + index * Op->Stride]);
                break;
              case 4:
                mov(GetDst<RA_32>(Node),  dword [rax + index * Op->Stride]);
                break;
              case 8:
                mov(GetDst<RA_64>(Node),  qword [rax + index * Op->Stride]);
                break;
              default:
                LogMan::Msg::A("Unhandled LoadContextIndexed size: %d", Op->Size);
              }
              break;
            }
            case 16:
              LogMan::Msg::A("Invalid Class load of size 16");
              break;
            default:
              LogMan::Msg::A("Unhandled LoadContextIndexed stride: %d", Op->Stride);
            }

          }
          else {
            switch (Op->Stride) {
            case 1:
            case 2:
            case 4:
            case 8: {
              lea(rax, dword [STATE + Op->BaseOffset]);
              switch (size) {
              case 1:
                movzx(eax, byte [rax + index * Op->Stride]);
                vmovd(GetDst(Node), eax);
                break;
              case 2:
                movzx(eax, word [rax + index * Op->Stride]);
                vmovd(GetDst(Node), eax);
                break;
              case 4:
                vmovd(GetDst(Node),  dword [rax + index * Op->Stride]);
                break;
              case 8:
                vmovq(GetDst(Node),  qword [rax + index * Op->Stride]);
                break;
              default:
                LogMan::Msg::A("Unhandled LoadContextIndexed size: %d", Op->Size);
              }
              break;
            }
            case 16: {
              mov(rax, index);
              shl(rax, 4);
              lea(rax, dword [rax + Op->BaseOffset]);
              switch (size) {
              case 1:
                pinsrb(GetDst(Node), byte [STATE + rax], 0);
                break;
              case 2:
                pinsrw(GetDst(Node), word [STATE + rax], 0);
                break;
              case 4:
                vmovd(GetDst(Node), dword [STATE + rax]);
                break;
              case 8:
                vmovq(GetDst(Node), qword [STATE + rax]);
                break;
              case 16:
                if (Op->BaseOffset % 16 == 0)
                  movaps(GetDst(Node), xword [STATE + rax]);
                else
                  movups(GetDst(Node), xword [STATE + rax]);
                break;
              default:
                LogMan::Msg::A("Unhandled LoadContextIndexed size: %d", Op->Size);
              }
              break;
            }
            default:
              LogMan::Msg::A("Unhandled LoadContextIndexed stride: %d", Op->Stride);
            }
          }
          break;
        }
        case IR::OP_STORECONTEXT: {
          auto Op = IROp->C<IR::IROp_StoreContext>();

          if (Op->Class.Val == 0) {
            switch (OpSize) {
            case 1: {
              mov(byte [STATE + Op->Offset], GetSrc<RA_8>(Op->Header.Args[0].ID()));
            }
            break;

            case 2: {
              mov(word [STATE + Op->Offset], GetSrc<RA_16>(Op->Header.Args[0].ID()));
            }
            break;
            case 4: {
              mov(dword [STATE + Op->Offset], GetSrc<RA_32>(Op->Header.Args[0].ID()));
            }
            break;
            case 8: {
              mov(qword [STATE + Op->Offset], GetSrc<RA_64>(Op->Header.Args[0].ID()));
            }
            break;
            case 16:
              LogMan::Msg::D("Invalid store size of 16");
            break;
            default:  LogMan::Msg::A("Unhandled StoreContext size: %d", OpSize);
            }
          }
          else {
            switch (OpSize) {
            case 1: {
              pextrb(byte [STATE + Op->Offset], GetSrc(Op->Header.Args[0].ID()), 0);
            }
            break;

            case 2: {
              pextrw(word [STATE + Op->Offset], GetSrc(Op->Header.Args[0].ID()), 0);
            }
            break;
            case 4: {
              vmovd(dword [STATE + Op->Offset], GetSrc(Op->Header.Args[0].ID()));
            }
            break;
            case 8: {
              vmovq(qword [STATE + Op->Offset], GetSrc(Op->Header.Args[0].ID()));
            }
            break;
            case 16: {
              if (Op->Offset % 16 == 0)
                movaps(xword [STATE + Op->Offset], GetSrc(Op->Header.Args[0].ID()));
              else
                movups(xword [STATE + Op->Offset], GetSrc(Op->Header.Args[0].ID()));
            }
            break;
            default:  LogMan::Msg::A("Unhandled StoreContext size: %d", OpSize);
            }
          }
          break;
        }
        case IR::OP_STORECONTEXTINDEXED: {
          auto Op = IROp->C<IR::IROp_StoreContextIndexed>();
          Reg index = GetSrc<RA_64>(Op->Header.Args[1].ID());
          size_t size = Op->Size;

          if (Op->Class.Val == 0) {
            auto value = GetSrc<RA_64>(Op->Header.Args[0].ID());
            lea(rax, dword [STATE + Op->BaseOffset]);

            switch (Op->Stride) {
            case 1:
            case 2:
            case 4:
            case 8: {
              if (!(size == 1 || size == 2 || size == 4 || size == 8)) {
                LogMan::Msg::A("Unhandled StoreContextIndexed size: %d", Op->Size);
              }
              mov(AddressFrame(Op->Size * 8) [rax + index * Op->Stride], value);
              break;
            }
            default:
              LogMan::Msg::A("Unhandled StoreContextIndexed stride: %d", Op->Stride);
            }
          }
          else {
            auto value = GetSrc(Op->Header.Args[0].ID());
            switch (Op->Stride) {
            case 1:
            case 2:
            case 4:
            case 8: {
              lea(rax, dword [STATE + Op->BaseOffset]);
              switch (size) {
              case 1:
                pextrb(AddressFrame(Op->Size * 8) [rax + index * Op->Stride], value, 0);
                break;
              case 2:
                pextrw(AddressFrame(Op->Size * 8) [rax + index * Op->Stride], value, 0);
                break;
              case 4:
                vmovd(AddressFrame(Op->Size * 8) [rax + index * Op->Stride], value);
                break;
              case 8:
                vmovq(AddressFrame(Op->Size * 8) [rax + index * Op->Stride], value);
                break;
              default:
                LogMan::Msg::A("Unhandled StoreContextIndexed size: %d", size);
              }
              break;
            }
            case 16: {
              mov(rax, index);
              shl(rax, 4);
              lea(rax, dword [rax + Op->BaseOffset]);
              switch (size) {
              case 1:
                pextrb(AddressFrame(Op->Size * 8) [STATE + rax], value, 0);
                break;
              case 2:
                pextrw(AddressFrame(Op->Size * 8) [STATE + rax], value, 0);
                break;
              case 4:
                vmovd(AddressFrame(Op->Size * 8) [STATE + rax], value);
                break;
              case 8:
                vmovq(AddressFrame(Op->Size * 8) [STATE + rax], value);
                break;
              case 16:
                if (Op->BaseOffset % 16 == 0)
                  movaps(xword [STATE + rax], value);
                else
                  movups(xword [STATE + rax], value);
                break;
              default:
                LogMan::Msg::A("Unhandled StoreContextIndexed size: %d", size);
              }
              break;
            }
            default:
              LogMan::Msg::A("Unhandled StoreContextIndexed stride: %d", Op->Stride);
            }
          }
          break;
        }
        case IR::OP_LOADCONTEXTPAIR: {
          auto Op = IROp->C<IR::IROp_LoadContextPair>();
          switch (Op->Size) {
            case 4: {
              auto Dst = GetSrcPair<RA_32>(Node);
              mov(Dst.first,  dword [STATE + Op->Offset]);
              mov(Dst.second, dword [STATE + Op->Offset + Op->Size]);
              break;
            }
            case 8: {
              auto Dst = GetSrcPair<RA_64>(Node);
              mov(Dst.first,  qword [STATE + Op->Offset]);
              mov(Dst.second, qword [STATE + Op->Offset + Op->Size]);
              break;
            }
            default: LogMan::Msg::A("Unknown Size"); break;
          }
          break;
        }
        case IR::OP_STORECONTEXTPAIR: {
          auto Op = IROp->C<IR::IROp_StoreContextPair>();
          switch (Op->Size) {
            case 4: {
              auto Src = GetSrcPair<RA_32>(Op->Header.Args[0].ID());
              mov(dword [STATE + Op->Offset], Src.first);
              mov(dword [STATE + Op->Offset + Op->Size], Src.first);
              break;
            }
            case 8: {
              auto Src = GetSrcPair<RA_64>(Op->Header.Args[0].ID());
              mov(qword [STATE + Op->Offset], Src.first);
              mov(qword [STATE + Op->Offset + Op->Size], Src.first);
              break;
            }
          }
          break;
        }
        case IR::OP_CREATEELEMENTPAIR: {
          auto Op = IROp->C<IR::IROp_CreateElementPair>();
          switch (Op->Header.Size) {
            case 4: {
              auto Dst = GetSrcPair<RA_32>(Node);
              mov(Dst.first, GetSrc<RA_32>(Op->Header.Args[0].ID()));
              mov(Dst.second, GetSrc<RA_32>(Op->Header.Args[1].ID()));
              break;
            }
            case 8: {
              auto Dst = GetSrcPair<RA_64>(Node);
              mov(Dst.first, GetSrc<RA_64>(Op->Header.Args[0].ID()));
              mov(Dst.second, GetSrc<RA_64>(Op->Header.Args[1].ID()));
              break;
            }
            default: LogMan::Msg::A("Unknown Size"); break;
          }
          break;
        }
        case IR::OP_EXTRACTELEMENTPAIR: {
          auto Op = IROp->C<IR::IROp_ExtractElementPair>();
          switch (Op->Header.Size) {
            case 4: {
              auto Src = GetSrcPair<RA_32>(Op->Header.Args[0].ID());
              std::array<Xbyak::Reg, 2> Regs = {Src.first, Src.second};
              mov (GetDst<RA_32>(Node), Regs[Op->Element]);
              break;
            }
            case 8: {
              auto Src = GetSrcPair<RA_64>(Op->Header.Args[0].ID());
              std::array<Xbyak::Reg, 2> Regs = {Src.first, Src.second};
              mov (GetDst<RA_64>(Node), Regs[Op->Element]);
              break;
            }
            default: LogMan::Msg::A("Unknown Size"); break;
          }
          break;
        }
        case IR::OP_TRUNCELEMENTPAIR: {
          auto Op = IROp->C<IR::IROp_TruncElementPair>();

          switch (Op->Size) {
            case 4: {
              auto Dst = GetSrcPair<RA_32>(Node);
              auto Src = GetSrcPair<RA_32>(Op->Header.Args[0].ID());
              mov(Dst.first, Src.first);
              mov(Dst.second, Src.second);
              break;
            }
            default: LogMan::Msg::A("Unhandled Truncation size: %d", Op->Size); break;
          }
          break;
        }
        case IR::OP_CASPAIR: {
          auto Op = IROp->C<IR::IROp_CAS>();
          // Args[0]: Desired
          // Args[1]: Expected
          // Args[2]: Pointer
          // DataSrc = *Src1
          // if (DataSrc == Src3) { *Src1 == Src2; } Src2 = DataSrc
          // This will write to memory! Careful!
          // Third operand must be a calculated guest memory address
          //OrderedNode *CASResult = _CAS(Src3, Src2, Src1);
          uint64_t Memory = CTX->MemoryMapper.GetBaseOffset<uint64_t>(0);
          auto Dst = GetSrcPair<RA_64>(Node);
          auto Expected = GetSrcPair<RA_64>(Op->Header.Args[0].ID());
          auto Desired = GetSrcPair<RA_64>(Op->Header.Args[1].ID());
          auto MemSrc = GetSrc<RA_64>(Op->Header.Args[2].ID());

          Xbyak::Reg MemReg = rdi;
          if (CTX->Config.UnifiedMemory) {
            MemReg = MemSrc;
          }
          else {
            mov(MemReg, Memory);
            add(MemReg, MemSrc);
          }

          mov(rax, Expected.first);
          mov(rdx, Expected.second);

          mov(rbx, Desired.first);
          mov(rcx, Desired.second);

          // RDI(Or Source) now contains pointer
          // RDX:RAX contains our expected value
          // RCX:RBX contains our desired

          lock();

          switch (OpSize) {
            case 4: {
              cmpxchg8b(dword [MemReg]);
              // EDX:EAX now contains the result
              mov(Dst.first.cvt32(), eax);
              mov(Dst.second.cvt32(), edx);
            break;
            }
            case 8: {
              cmpxchg16b(qword [MemReg]);
              // RDX:RAX now contains the result
              mov(Dst.first, rax);
              mov(Dst.second, rdx);
            break;
            }
            default: LogMan::Msg::A("Unsupported: %d", OpSize);
          }
          break;
        }
        case IR::OP_FILLREGISTER: {
          auto Op = IROp->C<IR::IROp_FillRegister>();
          uint32_t SlotOffset = Op->Slot * 16;
          switch (OpSize) {
          case 1: {
            movzx(GetDst<RA_32>(Node), byte [rsp + SlotOffset]);
          }
          break;
          case 2: {
            movzx(GetDst<RA_32>(Node), word [rsp + SlotOffset]);
          }
          break;
          case 4: {
            mov(GetDst<RA_32>(Node), dword [rsp + SlotOffset]);
          }
          break;
          case 8: {
            mov(GetDst<RA_64>(Node), qword [rsp + SlotOffset]);
          }
          break;
          case 16: {
            movaps(GetDst(Node), xword [rsp + SlotOffset]);
          }
          break;
          default:  LogMan::Msg::A("Unhandled FillRegister size: %d", OpSize);
          }
          break;
        }
        case IR::OP_SPILLREGISTER: {
          auto Op = IROp->C<IR::IROp_SpillRegister>();
          uint32_t SlotOffset = Op->Slot * 16;
          switch (OpSize) {
          case 1: {
            mov(byte [rsp + SlotOffset], GetSrc<RA_8>(Op->Header.Args[0].ID()));
          }
          break;
          case 2: {
            mov(word [rsp + SlotOffset], GetSrc<RA_16>(Op->Header.Args[0].ID()));
          }
          break;
          case 4: {
            mov(dword [rsp + SlotOffset], GetSrc<RA_32>(Op->Header.Args[0].ID()));
          }
          break;
          case 8: {
            mov(qword [rsp + SlotOffset], GetSrc<RA_64>(Op->Header.Args[0].ID()));
          }
          break;
          case 16: {
            movaps(xword [rsp + SlotOffset], GetSrc(Op->Header.Args[0].ID()));
          }
          break;
          default:  LogMan::Msg::A("Unhandled SpillRegister size: %d", OpSize);
          }
          break;
        }
        case IR::OP_ADD: {
          auto Op = IROp->C<IR::IROp_Add>();
          mov(rax, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          switch (OpSize) {
          case 4:
            add(eax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
            break;
          case 8:
            add(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
            break;
          default:  LogMan::Msg::A("Unhandled Add size: %d", OpSize);
            continue;
          }
          mov(GetDst<RA_64>(Node), rax);
          break;
        }
        case IR::OP_NEG: {
          auto Op = IROp->C<IR::IROp_Neg>();
          Xbyak::Reg Src;
          Xbyak::Reg Dst;
          switch (OpSize) {
          case 4:
            Src = GetSrc<RA_32>(Op->Header.Args[0].ID());
            Dst = GetDst<RA_32>(Node);
            break;
          case 8:
            Src = GetSrc<RA_64>(Op->Header.Args[0].ID());
            Dst = GetDst<RA_64>(Node);
            break;
          default:  LogMan::Msg::A("Unhandled Neg size: %d", OpSize);
            continue;
          }
          mov(Dst, Src);
          neg(Dst);
          break;
        }
        case IR::OP_SUB: {
          auto Op = IROp->C<IR::IROp_Sub>();
          mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          switch (OpSize) {
          case 4:
            sub(eax, GetSrc<RA_32>(Op->Header.Args[1].ID()));
            break;
          case 8:
            sub(rax, GetSrc<RA_64>(Op->Header.Args[1].ID()));
            break;
          default:  LogMan::Msg::A("Unhandled Sub size: %d", OpSize);
            continue;
          }
          mov(GetDst<RA_64>(Node), rax);
          break;
        }
        case IR::OP_XOR: {
          auto Op = IROp->C<IR::IROp_Xor>();
          auto Dst = GetDst<RA_64>(Node);
          mov(rax, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          xor(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          mov(Dst, rax);
          break;
        }
        case IR::OP_AND: {
          auto Op = IROp->C<IR::IROp_And>();
          auto Dst = GetDst<RA_64>(Node);
          mov(rax, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          and(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          mov(Dst, rax);
          break;
        }
        case IR::OP_OR: {
          auto Op = IROp->C<IR::IROp_Or>();
          auto Dst = GetDst<RA_64>(Node);
          mov(rax, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          or (rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          mov(Dst, rax);
          break;
        }
        case IR::OP_MOV: {
          auto Op = IROp->C<IR::IROp_Mov>();
          mov (GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()));
          break;
        }
        case IR::OP_CONSTANT: {
          auto Op = IROp->C<IR::IROp_Constant>();
          mov(GetDst<RA_64>(Node), Op->Constant);
          break;
        }
        case IR::OP_POPCOUNT: {
          auto Op = IROp->C<IR::IROp_Popcount>();
          auto Dst64 = GetDst<RA_64>(Node);

          switch (OpSize) {
          case 1:
            movzx(GetDst<RA_32>(Node), GetSrc<RA_8>(Op->Header.Args[0].ID()));
            popcnt(Dst64, Dst64);
          break;
          case 2: {
            movzx(GetDst<RA_32>(Node), GetSrc<RA_16>(Op->Header.Args[0].ID()));
            popcnt(Dst64, Dst64);
            break;
          }
          case 4:
            popcnt(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()));
          break;
          case 8:
            popcnt(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()));
          break;
          }
          break;
        }
        case IR::OP_NOT: {
          auto Op = IROp->C<IR::IROp_Not>();
          auto Dst = GetDst<RA_64>(Node);
          mov(Dst, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          not_(Dst);
          break;
        }
        case IR::OP_SEXT: {
          auto Op = IROp->C<IR::IROp_Sext>();
          LogMan::Throw::A(Op->SrcSize <= 64, "Can't support Zext of size: %ld", Op->SrcSize);
          auto Dst = GetDst<RA_64>(Node);

          switch (Op->SrcSize / 8) {
          case 1:
            movsx(Dst, GetSrc<RA_8>(Op->Header.Args[0].ID()));
          break;
          case 2:
            movsx(Dst, GetSrc<RA_16>(Op->Header.Args[0].ID()));
          break;
          case 4:
            movsxd(Dst.cvt64(), GetSrc<RA_32>(Op->Header.Args[0].ID()));
          break;
          case 8:
            mov(Dst, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          break;
          default: LogMan::Msg::A("Unknown Sext size: %d", Op->SrcSize / 8);
          }
          break;
        }
        case IR::OP_BFE: {
          auto Op = IROp->C<IR::IROp_Bfe>();
          LogMan::Throw::A(OpSize <= 8, "OpSize is too large for BFE: %d", OpSize);

          auto Dst = GetDst<RA_64>(Node);

          // Special cases for fast extends
          if (Op->lsb == 0) {
            switch (Op->Width / 8) {
            case 1:
              movzx(Dst, GetSrc<RA_8>(Op->Header.Args[0].ID()));
              goto Bfe_done;
            case 2:
              movzx(Dst, GetSrc<RA_16>(Op->Header.Args[0].ID()));
              goto Bfe_done;
            case 4:
              mov(Dst.cvt32(), GetSrc<RA_32>(Op->Header.Args[0].ID()));
              goto Bfe_done;
            case 8:
              mov(Dst, GetSrc<RA_64>(Op->Header.Args[0].ID()));
              goto Bfe_done;
            default:
              // Need to use slower general case
              break;
            }
          }

          mov(Dst, GetSrc<RA_64>(Op->Header.Args[0].ID()));

          if (Op->lsb != 0)
            shr(Dst, Op->lsb);

          if (Op->Width != 64) {
            mov(rcx, uint64_t((1ULL << Op->Width) - 1));
            and(Dst, rcx);
          }

          Bfe_done:
          break;
        }
        case IR::OP_SBFE: {
          auto Op = IROp->C<IR::IROp_Sbfe>();

          auto Dst = GetDst<RA_64>(Node);

          // Special cases for fast signed extends
          if (Op->lsb == 0) {
            switch (Op->Width / 8) {
            case 1:
              movsx(Dst, GetSrc<RA_8>(Op->Header.Args[0].ID()));
              goto Sbfe_done;
            case 2:
              movsx(Dst, GetSrc<RA_16>(Op->Header.Args[0].ID()));
              goto Sbfe_done;
            case 4:
              movsxd(Dst.cvt64(), GetSrc<RA_32>(Op->Header.Args[0].ID()));
              goto Sbfe_done;
            case 8:
              mov(Dst, GetSrc<RA_64>(Op->Header.Args[0].ID()));
              goto Sbfe_done;
            default:
              // Need to use slower general case
              break;
            }
          }

          // Slightly slower general case
          {
            uint64_t ShiftLeftAmount = (64 - (Op->Width + Op->lsb));
            uint64_t ShiftRightAmount = ShiftLeftAmount + Op->lsb;
            mov(Dst, GetSrc<RA_64>(Op->Header.Args[0].ID()));
            shl(Dst, ShiftLeftAmount);
            sar(Dst, ShiftRightAmount);
          }

          Sbfe_done:
          break;
        }
        case IR::OP_LSHR: {
          auto Op = IROp->C<IR::IROp_Lshr>();
          uint8_t Mask = OpSize * 8 - 1;

          mov (rcx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          and(rcx, Mask);

          switch (OpSize) {
            case 1:
              movzx(GetDst<RA_32>(Node), GetSrc<RA_8>(Op->Header.Args[0].ID()));
              shr(GetDst<RA_32>(Node).cvt8(), cl);
              break;
            case 2:
              movzx(GetDst<RA_32>(Node), GetSrc<RA_16>(Op->Header.Args[0].ID()));
              shr(GetDst<RA_32>(Node).cvt16(), cl);
              break;
            case 4:
              mov(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()));
              shr(GetDst<RA_32>(Node), cl);
              break;
            case 8:
              mov(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()));
              shr(GetDst<RA_64>(Node), cl);
              break;
            default: LogMan::Msg::A("Unknown Size: %d\n", OpSize); break;
          };

          break;
        }
        case IR::OP_LSHL: {
          auto Op = IROp->C<IR::IROp_Lshl>();
          uint8_t Mask = OpSize * 8 - 1;

          mov (rcx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          and(rcx, Mask);

          switch (OpSize) {
            case 4:
              mov(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()));
              shl(GetDst<RA_32>(Node), cl);
              break;
            case 8:
              mov(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()));
              shl(GetDst<RA_64>(Node), cl);
              break;
            default: LogMan::Msg::A("Unknown LSHL Size: %d\n", OpSize); break;
          };
          break;
        }
        case IR::OP_ASHR: {
          auto Op = IROp->C<IR::IROp_Ashr>();
          uint8_t Mask = OpSize * 8 - 1;

          mov (rcx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          and(rcx, Mask);
          switch (OpSize) {
          case 1:
            movsx(rax, GetSrc<RA_8>(Op->Header.Args[0].ID()));
            sar(al, cl);
            movsx(GetDst<RA_64>(Node), al);
          break;
          case 2:
            movsx(rax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
            sar(ax, cl);
            movsx(GetDst<RA_64>(Node), ax);
          break;
          case 4:
            mov(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()));
            sar(GetDst<RA_32>(Node), cl);
          break;
          case 8:
            mov(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()));
            sar(GetDst<RA_64>(Node), cl);
          break;
          default: LogMan::Msg::A("Unknown ASHR Size: %d\n", OpSize); break;
          };
          break;
        }
        case IR::OP_ROL: {
          auto Op = IROp->C<IR::IROp_Rol>();
          uint8_t Mask = OpSize * 8 - 1;

          mov (rcx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          and(rcx, Mask);
          switch (OpSize) {
          case 1: {
            movzx(rax, GetSrc<RA_8>(Op->Header.Args[0].ID()));
            rol(al, cl);
          break;
          }
          case 2: {
            movzx(rax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
            rol(ax, cl);
          break;
          }
          case 4: {
            mov(eax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
            rol(eax, cl);
          break;
          }
          case 8: {
            mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
            rol(rax, cl);
          break;
          }
          }
          mov(GetDst<RA_64>(Node), rax);
          break;
        }
        case IR::OP_ROR: {
          auto Op = IROp->C<IR::IROp_Ror>();
          uint8_t Mask = OpSize * 8 - 1;

          mov (rcx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          and(rcx, Mask);
          switch (OpSize) {
          case 1: {
            movzx(rax, GetSrc<RA_8>(Op->Header.Args[0].ID()));
            ror(al, cl);
          break;
          }
          case 2: {
            movzx(rax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
            ror(ax, cl);
          break;
          }
          case 4: {
            mov(eax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
            ror(eax, cl);
          break;
          }
          case 8: {
            mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
            ror(rax, cl);
          break;
          }
          }
          mov(GetDst<RA_64>(Node), rax);
          break;
        }
        case IR::OP_MUL: {
          auto Op = IROp->C<IR::IROp_Mul>();
          auto Dst = GetDst<RA_64>(Node);

          switch (OpSize) {
          case 1:
            movsx(rax, GetSrc<RA_8>(Op->Header.Args[0].ID()));
            movsx(rcx, GetSrc<RA_8>(Op->Header.Args[1].ID()));
            imul(cl);
            movsx(Dst, al);
          break;
          case 2:
            movsx(rax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
            movsx(rcx, GetSrc<RA_16>(Op->Header.Args[1].ID()));
            imul(cx);
            movsx(Dst, ax);
          break;
          case 4:
            movsxd(rax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
            imul(eax, GetSrc<RA_32>(Op->Header.Args[1].ID()));
            movsxd(Dst.cvt64(), eax);
          break;
          case 8:
            mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
            imul(rax, GetSrc<RA_64>(Op->Header.Args[1].ID()));
            mov(Dst, rax);
          break;
          default: LogMan::Msg::A("Unknown Sext size: %d", OpSize);
          }
          break;
        }
        case IR::OP_MULH: {
          auto Op = IROp->C<IR::IROp_MulH>();
          switch (OpSize) {
          case 1:
            movsx(rax, GetSrc<RA_8>(Op->Header.Args[0].ID()));
            movsx(rcx, GetSrc<RA_8>(Op->Header.Args[1].ID()));
            imul(cl);
            movsx(rax, ax);
            mov(GetDst<RA_64>(Node), rax);
          break;
          case 2:
            movsx(rax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
            movsx(rcx, GetSrc<RA_16>(Op->Header.Args[1].ID()));
            imul(cx);
            movsx(rax, dx);
            mov(GetDst<RA_64>(Node), rax);
          break;
          case 4:
            movsxd(rax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
            imul(GetSrc<RA_32>(Op->Header.Args[1].ID()));
            movsxd(rax, edx);
            mov(GetDst<RA_64>(Node), rdx);
          break;
          case 8:
            mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
            imul(GetSrc<RA_64>(Op->Header.Args[1].ID()));
            mov(GetDst<RA_64>(Node), rdx);
          break;
          default: LogMan::Msg::A("Unknown Sext size: %d", OpSize);
          }
          break;
        }
        case IR::OP_UMUL: {
          auto Op = IROp->C<IR::IROp_UMul>();
          switch (OpSize) {
          case 1:
            movzx(rax, GetSrc<RA_8>(Op->Header.Args[0].ID()));
            movzx(rcx, GetSrc<RA_8>(Op->Header.Args[1].ID()));
            mul(cl);
            movzx(rax, al);
            mov(GetDst<RA_64>(Node), rax);
          break;
          case 2:
            movzx(rax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
            movzx(rcx, GetSrc<RA_16>(Op->Header.Args[1].ID()));
            mul(cx);
            movzx(rax, ax);
            mov(GetDst<RA_64>(Node), rax);
          break;
          case 4:
            mov(rax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
            mul(GetSrc<RA_32>(Op->Header.Args[1].ID()));
            mov(GetDst<RA_64>(Node), rax);
          break;
          case 8:
            mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
            mul(GetSrc<RA_64>(Op->Header.Args[1].ID()));
            mov(GetDst<RA_64>(Node), rax);
          break;
          default: LogMan::Msg::A("Unknown Sext size: %d", OpSize);
          }
          break;
        }
        case IR::OP_UMULH: {
          auto Op = IROp->C<IR::IROp_UMulH>();
          switch (OpSize) {
          case 1:
            movzx(rax, GetSrc<RA_8>(Op->Header.Args[0].ID()));
            movzx(rcx, GetSrc<RA_8>(Op->Header.Args[1].ID()));
            mul(cl);
            movzx(rax, ax);
            mov(GetDst<RA_64>(Node), rax);
          break;
          case 2:
            movzx(rax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
            movzx(rcx, GetSrc<RA_16>(Op->Header.Args[1].ID()));
            mul(cx);
            movzx(rax, dx);
            mov(GetDst<RA_64>(Node), rax);
          break;
          case 4:
            mov(rax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
            mul(GetSrc<RA_32>(Op->Header.Args[1].ID()));
            mov(GetDst<RA_64>(Node), rdx);
          break;
          case 8:
            mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
            mul(GetSrc<RA_64>(Op->Header.Args[1].ID()));
            mov(GetDst<RA_64>(Node), rdx);
          break;
          default: LogMan::Msg::A("Unknown Sext size: %d", OpSize);
          }
          break;
        }
        case IR::OP_LDIV: {
          auto Op = IROp->C<IR::IROp_LDiv>();
          // Each source is OpSize in size
          // So you can have up to a 128bit divide from x86-64
          auto Size = OpSize;
          switch (Size) {
            case 2: {
              mov(eax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
              mov(edx, GetSrc<RA_16>(Op->Header.Args[1].ID()));
              idiv(GetSrc<RA_16>(Op->Header.Args[2].ID()));
              movsx(GetDst<RA_64>(Node), ax);
              break;
            }
            case 4: {
              mov(eax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
              mov(edx, GetSrc<RA_32>(Op->Header.Args[1].ID()));
              idiv(GetSrc<RA_32>(Op->Header.Args[2].ID()));
              movsxd(GetDst<RA_64>(Node).cvt64(), eax);
              break;
            }
            case 8: {
              mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
              mov(rdx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
              idiv(GetSrc<RA_64>(Op->Header.Args[2].ID()));
              mov(GetDst<RA_64>(Node), rax);
              break;
            }
            default: LogMan::Msg::A("Unknown LDIV Size: %d", Size); break;
          }
          break;
        }
        case IR::OP_LREM: {
          auto Op = IROp->C<IR::IROp_LRem>();
          // Each source is OpSize in size
          // So you can have up to a 128bit divide from x86-64
          auto Size = OpSize;
          switch (Size) {
            case 2: {
              mov(ax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
              mov(dx, GetSrc<RA_16>(Op->Header.Args[1].ID()));
              idiv(GetSrc<RA_16>(Op->Header.Args[2].ID()));
              movsx(GetDst<RA_64>(Node), dx);
              break;
            }
            case 4: {
              mov(eax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
              mov(edx, GetSrc<RA_32>(Op->Header.Args[1].ID()));
              idiv(GetSrc<RA_32>(Op->Header.Args[2].ID()));
              mov(GetDst<RA_64>(Node), rdx);
              break;
            }
            case 8: {
              mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
              mov(rdx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
              idiv(GetSrc<RA_64>(Op->Header.Args[2].ID()));
              mov(GetDst<RA_64>(Node), rdx);
              break;
            }
            default: LogMan::Msg::A("Unknown LREM Size: %d", Size); break;
          }
          break;
        }
        case IR::OP_LUDIV: {
          auto Op = IROp->C<IR::IROp_LUDiv>();
          // Each source is OpSize in size
          // So you can have up to a 128bit divide from x86-64
          auto Size = OpSize;
          switch (Size) {
            case 2: {
              mov (ax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
              mov (dx, GetSrc<RA_16>(Op->Header.Args[1].ID()));
              div(GetSrc<RA_16>(Op->Header.Args[2].ID()));
              movzx(GetDst<RA_32>(Node), ax);
              break;
            }
            case 4: {
              mov (eax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
              mov (edx, GetSrc<RA_32>(Op->Header.Args[1].ID()));
              div(GetSrc<RA_32>(Op->Header.Args[2].ID()));
              mov(GetDst<RA_64>(Node), rax);
              break;
            }
            case 8: {
              mov (rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
              mov (rdx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
              div(GetSrc<RA_64>(Op->Header.Args[2].ID()));
              mov(GetDst<RA_64>(Node), rax);
              break;
            }
            default: LogMan::Msg::A("Unknown LUDIV Size: %d", Size); break;
          }
          break;
        }
        case IR::OP_LUREM: {
          auto Op = IROp->C<IR::IROp_LURem>();
          // Each source is OpSize in size
          // So you can have up to a 128bit divide from x86-64
          auto Size = OpSize;
          switch (Size) {
            case 2: {
              mov (ax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
              mov (dx, GetSrc<RA_16>(Op->Header.Args[1].ID()));
              div(GetSrc<RA_16>(Op->Header.Args[2].ID()));
              movzx(GetDst<RA_64>(Node), dx);
              break;
            }
            case 4: {
              mov (eax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
              mov (edx, GetSrc<RA_32>(Op->Header.Args[1].ID()));
              div(GetSrc<RA_32>(Op->Header.Args[2].ID()));
              mov(GetDst<RA_64>(Node), rdx);
              break;
            }
            case 8: {
              mov (rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
              mov (rdx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
              div(GetSrc<RA_64>(Op->Header.Args[2].ID()));
              mov(GetDst<RA_64>(Node), rdx);
              break;
            }
            default: LogMan::Msg::A("Unknown LUDIV Size: %d", Size); break;
          }
          break;
        }
        case IR::OP_UDIV: {
          auto Op = IROp->C<IR::IROp_UDiv>();
          // Each source is OpSize in size
          // So you can have up to a 128bit divide from x86-64
          auto Size = OpSize;
          switch (Size) {
          case 1: {
            mov (al, GetSrc<RA_8>(Op->Header.Args[0].ID()));
            mov (edx, 0);
            mov (cl, GetSrc<RA_8>(Op->Header.Args[1].ID()));
            div(cl);
            movzx(GetDst<RA_32>(Node), al);
          break;
          }
          case 2: {
            mov (ax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
            mov (edx, 0);
            mov (cx, GetSrc<RA_16>(Op->Header.Args[1].ID()));
            div(cx);
            movzx(GetDst<RA_32>(Node), ax);
          break;
          }
          case 4: {
            mov (eax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
            mov (edx, 0);
            mov (ecx, GetSrc<RA_32>(Op->Header.Args[1].ID()));
            div(ecx);
            mov(GetDst<RA_32>(Node), eax);
          break;
          }
          case 8: {
            mov (rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
            mov (rdx, 0);
            mov (rcx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
            div(rcx);
            mov(GetDst<RA_64>(Node), rax);
          break;
          }
          default: LogMan::Msg::A("Unknown UDIV Size: %d", Size); break;
          }
          break;
        }
        case IR::OP_UREM: {
          auto Op = IROp->C<IR::IROp_URem>();
          // Each source is OpSize in size
          // So you can have up to a 128bit divide from x86-64
          auto Size = OpSize;
          switch (Size) {
          case 1: {
            mov (al, GetSrc<RA_8>(Op->Header.Args[0].ID()));
            mov (edx, 0);
            mov (cl, GetSrc<RA_8>(Op->Header.Args[1].ID()));
            div(cl);
            movzx(GetDst<RA_32>(Node), ah);
          break;
          }
          case 2: {
            mov (ax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
            mov (edx, 0);
            mov (cx, GetSrc<RA_16>(Op->Header.Args[1].ID()));
            div(cx);
            movzx(GetDst<RA_32>(Node), dx);
          break;
          }
          case 4: {
            mov (eax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
            mov (edx, 0);
            mov (ecx, GetSrc<RA_32>(Op->Header.Args[1].ID()));
            div(ecx);
            mov(GetDst<RA_32>(Node), edx);
          break;
          }
          case 8: {
            mov (rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
            mov (rdx, 0);
            mov (rcx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
            div(rcx);
            mov(GetDst<RA_64>(Node), rdx);
          break;
          }
          default: LogMan::Msg::A("Unknown UDIV Size: %d", Size); break;
          }
          break;
        }
        case IR::OP_DIV: {
          auto Op = IROp->C<IR::IROp_Div>();
          // Each source is OpSize in size
          // So you can have up to a 128bit divide from x86-64
          auto Size = OpSize;
          switch (Size) {
          case 1: {
            movsx(ax, GetSrc<RA_8>(Op->Header.Args[0].ID()));
            idiv(GetSrc<RA_8>(Op->Header.Args[1].ID()));
            movsx(GetDst<RA_32>(Node), al);
          break;
          }
          case 2: {
            mov (ax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
            cwd();
            idiv(GetSrc<RA_16>(Op->Header.Args[1].ID()));
            movsx(GetDst<RA_32>(Node), ax);
          break;
          }
          case 4: {
            mov (eax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
            cdq();
            idiv(GetSrc<RA_32>(Op->Header.Args[1].ID()));
            movsxd(GetDst<RA_64>(Node).cvt64(), eax);
          break;
          }
          case 8: {
            mov (rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
            cqo();
            idiv(GetSrc<RA_64>(Op->Header.Args[1].ID()));
            mov(GetDst<RA_64>(Node), rax);
          break;
          }
          default: LogMan::Msg::A("Unknown UDIV Size: %d", Size); break;
          }
          break;
        }
        case IR::OP_REM: {
          auto Op = IROp->C<IR::IROp_Rem>();
          switch (OpSize) {
          case 1: {
            movsx(ax, GetSrc<RA_8>(Op->Header.Args[0].ID()));
            idiv(GetSrc<RA_8>(Op->Header.Args[1].ID()));
            mov(al, ah);
            movsx(GetDst<RA_32>(Node), al);
          break;
          }
          case 2: {
            mov (ax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
            cwd();
            idiv(GetSrc<RA_16>(Op->Header.Args[1].ID()));
            movsx(GetDst<RA_32>(Node), dx);
          break;
          }
          case 4: {
            mov (eax, GetSrc<RA_32>(Op->Header.Args[0].ID()));
            cdq();
            idiv(GetSrc<RA_32>(Op->Header.Args[1].ID()));
            movsxd(GetDst<RA_64>(Node).cvt64(), edx);
          break;
          }
          case 8: {
            mov (rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
            cqo();
            idiv(GetSrc<RA_64>(Op->Header.Args[1].ID()));
            mov(GetDst<RA_64>(Node), rdx);
          break;
          }
          default: LogMan::Msg::A("Unknown UDIV Size: %d", OpSize); break;
          }
          break;
        }

        case IR::OP_LOADFLAG: {
          auto Op = IROp->C<IR::IROp_LoadFlag>();

          auto Dst = GetDst<RA_64>(Node);
          movzx(Dst, byte [STATE + (offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag)]);
          and(Dst, 1);
          break;
        }
        case IR::OP_STOREFLAG: {
          auto Op = IROp->C<IR::IROp_StoreFlag>();

          mov (rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          and(rax, 1);
          mov(byte [STATE + (offsetof(FEXCore::Core::CPUState, flags[0]) + Op->Flag)], al);
          break;
        }
        case IR::OP_SELECT: {
          auto Op = IROp->C<IR::IROp_Select>();
          auto Dst = GetDst<RA_64>(Node);

          mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          cmp(rax, GetSrc<RA_64>(Op->Header.Args[1].ID()));

          switch (Op->Cond.Val) {
          case FEXCore::IR::COND_EQ:
            mov(rax, GetSrc<RA_64>(Op->Header.Args[3].ID()));
            cmove(rax, GetSrc<RA_64>(Op->Header.Args[2].ID()));
          break;
          case FEXCore::IR::COND_NEQ:
            mov(rax, GetSrc<RA_64>(Op->Header.Args[3].ID()));
            cmovne(rax, GetSrc<RA_64>(Op->Header.Args[2].ID()));
          break;
          case FEXCore::IR::COND_SGE:
            mov(rax, GetSrc<RA_64>(Op->Header.Args[3].ID()));
            cmovge(rax, GetSrc<RA_64>(Op->Header.Args[2].ID()));
          break;
          case FEXCore::IR::COND_SLT:
            mov(rax, GetSrc<RA_64>(Op->Header.Args[3].ID()));
            cmovl(rax, GetSrc<RA_64>(Op->Header.Args[2].ID()));
          break;
          case FEXCore::IR::COND_SGT:
            mov(rax, GetSrc<RA_64>(Op->Header.Args[3].ID()));
            cmovg(rax, GetSrc<RA_64>(Op->Header.Args[2].ID()));
          break;
          case FEXCore::IR::COND_SLE:
            mov(rax, GetSrc<RA_64>(Op->Header.Args[3].ID()));
            cmovle(rax, GetSrc<RA_64>(Op->Header.Args[2].ID()));
          break;
          case FEXCore::IR::COND_UGE:
            mov(rax, GetSrc<RA_64>(Op->Header.Args[3].ID()));
            cmovae(rax, GetSrc<RA_64>(Op->Header.Args[2].ID()));
          break;
          case FEXCore::IR::COND_ULT:
            mov(rax, GetSrc<RA_64>(Op->Header.Args[3].ID()));
            cmovb(rax, GetSrc<RA_64>(Op->Header.Args[2].ID()));
          break;
          case FEXCore::IR::COND_UGT:
            mov(rax, GetSrc<RA_64>(Op->Header.Args[3].ID()));
            cmova(rax, GetSrc<RA_64>(Op->Header.Args[2].ID()));
          break;
          case FEXCore::IR::COND_ULE:
            mov(rax, GetSrc<RA_64>(Op->Header.Args[3].ID()));
            cmovna(rax, GetSrc<RA_64>(Op->Header.Args[2].ID()));
            break;
          case FEXCore::IR::COND_MI:
          case FEXCore::IR::COND_PL:
          case FEXCore::IR::COND_VS:
          case FEXCore::IR::COND_VC:
          default:
          LogMan::Msg::A("Unsupported compare type");
          break;
          }
          mov (Dst, rax);
          break;
        }
        case IR::OP_LOADMEM:
        case IR::OP_LOADMEMTSO: {
          auto Op = IROp->C<IR::IROp_LoadMem>();
          uint64_t Memory = CTX->MemoryMapper.GetBaseOffset<uint64_t>(0);

          Xbyak::Reg MemReg = rax;
          if (CTX->Config.UnifiedMemory) {
            MemReg = GetSrc<RA_64>(Op->Header.Args[0].ID());
          }
          else {
            mov(MemReg, Memory);
            add(MemReg, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          }
          if (Op->Class.Val == 0) {
            auto Dst = GetDst<RA_64>(Node);

            switch (Op->Size) {
              case 1: {
                movzx (Dst, byte [MemReg]);
              }
              break;
              case 2: {
                movzx (Dst, word [MemReg]);
              }
              break;
              case 4: {
                mov(Dst.cvt32(), dword [MemReg]);
              }
              break;
              case 8: {
                mov(Dst, qword [MemReg]);
              }
              break;
              default:  LogMan::Msg::A("Unhandled LoadMem size: %d", Op->Size);
            }
          }
          else
          {
            auto Dst = GetDst(Node);

            switch (Op->Size) {
              case 1: {
                movzx(eax, byte [MemReg]);
                vmovd(Dst, eax);
              }
              break;
              case 2: {
                movzx(eax, byte [MemReg]);
                vmovd(Dst, eax);
              }
              break;
              case 4: {
                vmovd(Dst, dword [MemReg]);
              }
              break;
              case 8: {
                vmovq(Dst, qword [MemReg]);
              }
              break;
              case 16: {
                 if (Op->Size == Op->Align)
                   movups(GetDst(Node), xword [MemReg]);
                 else
                   movups(GetDst(Node), xword [MemReg]);
                 if (MemoryDebug) {
                   movq(rcx, GetDst(Node));
                 }
               }
               break;
              default:  LogMan::Msg::A("Unhandled LoadMem size: %d", Op->Size);
            }
          }
          break;
        }
        case IR::OP_STOREMEM:
        case IR::OP_STOREMEMTSO: {
          auto Op = IROp->C<IR::IROp_StoreMem>();
          uint64_t Memory = CTX->MemoryMapper.GetBaseOffset<uint64_t>(0);

          Xbyak::Reg MemReg = rax;
          if (CTX->Config.UnifiedMemory) {
            MemReg = GetSrc<RA_64>(Op->Header.Args[0].ID());
          }
          else {
            mov(MemReg, Memory);
            add(MemReg, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          }

          if (Op->Class.Val == 0) {
            switch (Op->Size) {
            case 1:
              mov(byte [MemReg], GetSrc<RA_8>(Op->Header.Args[1].ID()));
            break;
            case 2:
              mov(word [MemReg], GetSrc<RA_16>(Op->Header.Args[1].ID()));
            break;
            case 4:
              mov(dword [MemReg], GetSrc<RA_32>(Op->Header.Args[1].ID()));
            break;
            case 8:
              mov(qword [MemReg], GetSrc<RA_64>(Op->Header.Args[1].ID()));
            break;
            default:  LogMan::Msg::A("Unhandled StoreMem size: %d", Op->Size);
            }
          }
          else {
            switch (Op->Size) {
            case 1:
              pextrb(byte [MemReg], GetSrc(Op->Header.Args[1].ID()), 0);
            break;
            case 2:
              pextrw(word [MemReg], GetSrc(Op->Header.Args[1].ID()), 0);
            break;
            case 4:
              vmovd(dword [MemReg], GetSrc(Op->Header.Args[1].ID()));
            break;
            case 8:
              vmovq(qword [MemReg], GetSrc(Op->Header.Args[1].ID()));
            break;
            case 16:
              if (Op->Size == Op->Align)
                movups(xword [MemReg], GetSrc(Op->Header.Args[1].ID()));
              else
                movups(xword [MemReg], GetSrc(Op->Header.Args[1].ID()));
            break;
            default:  LogMan::Msg::A("Unhandled StoreMem size: %d", Op->Size);
            }
          }
          break;
        }
        case IR::OP_SYSCALL: {
          auto Op = IROp->C<IR::IROp_Syscall>();
          // XXX: This is very terrible, but I don't care for right now

          auto NumPush = RA64.size();

          for (auto &Reg : RA64)
            push(Reg);

          // Syscall ABI for x86-64
          // this: rdi
          // Thread: rsi
          // ArgPointer: rdx (Stack)
          //
          // Result: RAX

          // These are pushed in reverse order because stacks
          for (uint32_t i = FEXCore::HLE::SyscallArguments::MAX_ARGS; i > 0; --i) {
            if (Op->Header.Args[i - 1].IsInvalid()) continue;
            push(GetSrc<RA_64>(Op->Header.Args[i - 1].ID()));
            ++NumPush;
          }

          mov(rsi, STATE); // Move thread in to rsi
          mov(rdi, reinterpret_cast<uint64_t>(CTX->SyscallHandler.get()));
          mov(rdx, rsp);

          mov(rax, reinterpret_cast<uint64_t>(FEXCore::HandleSyscall));

          if (NumPush & 1)
            sub(rsp, 8); // Align
    // {rdi, rsi, rdx}
          call(rax);

          if (NumPush & 1)
            add(rsp, 8); // Align

          // Reload arguments just in case they are sill live after the fact
          for (uint32_t i = 0; i < FEXCore::HLE::SyscallArguments::MAX_ARGS; ++i) {
            if (Op->Header.Args[i].IsInvalid()) continue;
            pop(GetSrc<RA_64>(Op->Header.Args[i].ID()));
          }

          for (uint32_t i = RA64.size(); i > 0; --i)
            pop(RA64[i - 1]);

          mov (GetDst<RA_64>(Node), rax);
          break;
        }
        case IR::OP_THUNK: {
          auto Op = IROp->C<IR::IROp_Thunk>();

          auto NumPush = RA64.size();

          for (auto &Reg : RA64)
            push(Reg);

          if (NumPush & 1)
            sub(rsp, 8); // Align

          mov(rdi, reinterpret_cast<uintptr_t>(CTX));
          mov(rsi, GetSrc<RA_64>(Op->Header.Args[0].ID()));

          mov(rax, reinterpret_cast<uintptr_t>(Op->ThunkFnPtr));
          call(rax);

          if (NumPush & 1)
            add(rsp, 8); // Align

          for (uint32_t i = RA64.size(); i > 0; --i)
            pop(RA64[i - 1]);

          break;
        }
        case IR::OP_VEXTRACTTOGPR: {
          auto Op = IROp->C<IR::IROp_VExtractToGPR>();

          switch (Op->Header.ElementSize) {
          case 1: {
            pextrb(GetDst<RA_32>(Node), GetSrc(Op->Header.Args[0].ID()), Op->Idx);
          break;
          }
          case 2: {
            pextrw(GetDst<RA_32>(Node), GetSrc(Op->Header.Args[0].ID()), Op->Idx);
          break;
          }
          case 4: {
            pextrd(GetDst<RA_32>(Node), GetSrc(Op->Header.Args[0].ID()), Op->Idx);
          break;
          }
          case 8: {
            pextrq(GetDst<RA_64>(Node), GetSrc(Op->Header.Args[0].ID()), Op->Idx);
          break;
          }
          default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }

          break;
        }
        case IR::OP_VINSGPR: {
          auto Op = IROp->C<IR::IROp_VInsGPR>();
          movapd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));

          switch (Op->Header.ElementSize) {
          case 1: {
            pinsrb(GetDst(Node), GetSrc<RA_32>(Op->Header.Args[1].ID()), Op->Index);
          break;
          }
          case 2: {
            pinsrw(GetDst(Node), GetSrc<RA_32>(Op->Header.Args[1].ID()), Op->Index);
          break;
          }
          case 4: {
            pinsrd(GetDst(Node), GetSrc<RA_32>(Op->Header.Args[1].ID()), Op->Index);
          break;
          }
          case 8: {
            pinsrq(GetDst(Node), GetSrc<RA_64>(Op->Header.Args[1].ID()), Op->Index);
          break;
          }
          default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }

          break;
        }
        case IR::OP_PRINT: {
          auto Op = IROp->C<IR::IROp_Print>();

          for (auto &Reg : RA64)
            push(Reg);

          auto NumPush = RA64.size();
          if (NumPush & 1)
            sub(rsp, 8); // Align

          mov (rdi, GetSrc<RA_64>(Op->Header.Args[0].ID()));

          mov(rax, reinterpret_cast<uintptr_t>(PrintValue));

          call(rax);

          if (NumPush & 1)
            add(rsp, 8); // Align

          for (uint32_t i = RA64.size(); i > 0; --i)
            pop(RA64[i - 1]);

          break;
        }

        case IR::OP_CPUID: {
          auto Op = IROp->C<IR::IROp_CPUID>();

          using ClassPtrType = FEXCore::CPUIDEmu::FunctionResults (FEXCore::CPUIDEmu::*)(uint32_t Function);
          union {
            ClassPtrType ClassPtr;
            uint64_t Raw;
          } Ptr;
          Ptr.ClassPtr = &CPUIDEmu::RunFunction;

          for (auto &Reg : RA64)
            push(Reg);

          // CPUID ABI
          // this: rdi
          // Function: rsi
          //
          // Result: RAX, RDX. 4xi32

          mov (rsi, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          mov (rdi, reinterpret_cast<uint64_t>(&CTX->CPUID));

          auto NumPush = RA64.size();

          if (NumPush & 1)
            sub(rsp, 8); // Align

          mov(rax, Ptr.Raw);

          // {rdi, rsi, rdx}

          call(rax);

          if (NumPush & 1)
            add(rsp, 8); // Align

          for (uint32_t i = RA64.size(); i > 0; --i)
            pop(RA64[i - 1]);

          auto Dst = GetSrcPair<RA_64>(Node);
          mov(Dst.first, rax);
          mov(Dst.second, rdx);
          break;
        }
        case IR::OP_SPLATVECTOR2:
        case IR::OP_SPLATVECTOR4: {
          auto Op = IROp->C<IR::IROp_SplatVector2>();
          LogMan::Throw::A(OpSize <= 16, "Can't handle a vector of size: %d", OpSize);
          uint8_t Elements = 0;

          switch (Op->Header.Op) {
            case IR::OP_SPLATVECTOR4: Elements = 4; break;
            case IR::OP_SPLATVECTOR2: Elements = 2; break;
            default: LogMan::Msg::A("Uknown Splat size"); break;
          }

          uint8_t ElementSize = OpSize / Elements;

          switch (ElementSize) {
            case 4:
              movapd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
              shufps(GetDst(Node), GetDst(Node), 0);
            break;
            case 8:
              movddup(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
            break;
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.Size); break;
          }
          break;
        }
        case IR::OP_VINSELEMENT: {
          auto Op = IROp->C<IR::IROp_VInsElement>();
          movapd(xmm15, GetSrc(Op->Header.Args[0].ID()));

          // Dst_d[Op->DestIdx] = Src2_d[Op->SrcIdx];

          // pextrq reg64/mem64, xmm, imm
          // pinsrq xmm, reg64/mem64, imm8
          switch (Op->Header.ElementSize) {
          case 1: {
            pextrb(eax, GetSrc(Op->Header.Args[1].ID()), Op->SrcIdx);
            pinsrb(xmm15, eax, Op->DestIdx);
          break;
          }
          case 2: {
            pextrw(eax, GetSrc(Op->Header.Args[1].ID()), Op->SrcIdx);
            pinsrw(xmm15, eax, Op->DestIdx);
          break;
          }
          case 4: {
            pextrd(eax, GetSrc(Op->Header.Args[1].ID()), Op->SrcIdx);
            pinsrd(xmm15, eax, Op->DestIdx);
          break;
          }
          case 8: {
            pextrq(rax, GetSrc(Op->Header.Args[1].ID()), Op->SrcIdx);
            pinsrq(xmm15, rax, Op->DestIdx);
          break;
          }
          default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }

          movapd(GetDst(Node), xmm15);
          break;
        }
        case IR::OP_VINSSCALARELEMENT: {
          auto Op = IROp->C<IR::IROp_VInsScalarElement>();
          movapd(xmm15, GetSrc(Op->Header.Args[0].ID()));

          // Dst_d[Op->DestIdx] = Src2_d[Op->SrcIdx];

          // pextrq reg64/mem64, xmm, imm
          // pinsrq xmm, reg64/mem64, imm8
          switch (Op->Header.ElementSize) {
          case 1: {
            pextrb(eax, GetSrc(Op->Header.Args[1].ID()), 0);
            pinsrb(xmm15, eax, Op->DestIdx);
          break;
          }
          case 2: {
            pextrw(eax, GetSrc(Op->Header.Args[1].ID()), 0);
            pinsrw(xmm15, eax, Op->DestIdx);
          break;
          }
          case 4: {
            pextrd(eax, GetSrc(Op->Header.Args[1].ID()), 0);
            pinsrd(xmm15, eax, Op->DestIdx);
          break;
          }
          case 8: {
            pextrq(rax, GetSrc(Op->Header.Args[1].ID()), 0);
            pinsrq(xmm15, rax, Op->DestIdx);
          break;
          }
          default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }

          movapd(GetDst(Node), xmm15);
          break;
        }
        case IR::OP_VEXTRACTELEMENT: {
          auto Op = IROp->C<IR::IROp_VExtractElement>();

          switch (Op->Header.ElementSize) {
          case 1: {
            pextrb(eax, GetSrc(Op->Header.Args[0].ID()), Op->Index);
            pinsrb(GetDst(Node), eax, 0);
          break;
          }
          case 2: {
            pextrw(eax, GetSrc(Op->Header.Args[0].ID()), Op->Index);
            pinsrw(GetDst(Node), eax, 0);
          break;
          }
          case 4: {
            pextrd(eax, GetSrc(Op->Header.Args[0].ID()), Op->Index);
            pinsrd(GetDst(Node), eax, 0);
          break;
          }
          case 8: {
            pextrq(rax, GetSrc(Op->Header.Args[0].ID()), Op->Index);
            pinsrq(GetDst(Node), rax, 0);
          break;
          }
          default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }

          break;
        }
        case IR::OP_VADD: {
          auto Op = IROp->C<IR::IROp_VAdd>();
          switch (Op->Header.ElementSize) {
          case 1: {
            vpaddb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 2: {
            vpaddw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 4: {
            vpaddd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 8: {
            vpaddq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }
          break;
        }
        case IR::OP_VSQADD: {
          auto Op = IROp->C<IR::IROp_VSQAdd>();
          switch (Op->Header.ElementSize) {
          case 1: {
            vpaddsb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 2: {
            vpaddsw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }
          break;
        }
        case IR::OP_VSQSUB: {
          auto Op = IROp->C<IR::IROp_VSQSub>();
          switch (Op->Header.ElementSize) {
          case 1: {
            vpsubsb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 2: {
            vpsubsw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }
          break;
        }
        case IR::OP_VUQADD: {
          auto Op = IROp->C<IR::IROp_VUQAdd>();
          switch (Op->Header.ElementSize) {
          case 1: {
            vpaddusb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 2: {
            vpaddusw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }
          break;
        }
        case IR::OP_VUQSUB: {
          auto Op = IROp->C<IR::IROp_VUQSub>();
          switch (Op->Header.ElementSize) {
          case 1: {
            vpsubusb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 2: {
            vpsubusw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }
          break;
        }
        case IR::OP_VSUB: {
          auto Op = IROp->C<IR::IROp_VSub>();
          switch (Op->Header.ElementSize) {
          case 1: {
            vpsubb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 2: {
            vpsubw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 4: {
            vpsubd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 8: {
            vpsubq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }
          break;
        }
        case IR::OP_VADDP: {
          auto Op = IROp->C<IR::IROp_VAddP>();
          switch (Op->Header.ElementSize) {
          case 2:
            vphaddw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          case 4:
            vphaddd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }
          break;
        }
        case IR::OP_VADDV: {
          auto Op = IROp->C<IR::IROp_VAddV>();
          auto Src = GetSrc(Op->Header.Args[0].ID());
          auto Dest = GetDst(Node);
          vpxor(xmm15, xmm15, xmm15);
          switch (Op->Header.ElementSize) {
            case 2: {
              for (int i = 0; i < (Op->Header.Size / 4); ++i) {
                phaddw(Dest, Src);
                Src = Dest;
              }
              pextrw(eax, Dest, 0);
              pinsrw(xmm15, eax, 0);
            break;
            }
            case 4: {
              for (int i = 0; i < (Op->Header.Size / 8); ++i) {
                phaddd(Dest, Src);
                Src = Dest;
              }
              pextrd(eax, Dest, 0);
              pinsrd(xmm15, eax, 0);
            break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }

          movaps(Dest, xmm15);
          break;
        }
        case IR::OP_VURAVG: {
          auto Op = IROp->C<IR::IROp_VURAvg>();
          switch (Op->Header.ElementSize) {
          case 1: {
            vpavgb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 2: {
            vpavgw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }
          break;
        }
        case IR::OP_VABS: {
          auto Op = IROp->C<IR::IROp_VAbs>();
          switch (Op->Header.ElementSize) {
            case 1: {
              vpabsb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
            break;
            }
            case 2: {
              vpabsw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
            break;
            }
            case 4: {
              vpabsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
            break;
            }
            case 8: {
              vpabsq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
            break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }
          break;
        }
        case IR::OP_VUMUL:
        case IR::OP_VSMUL: {
          auto Op = IROp->C<IR::IROp_VUMul>();
          switch (Op->Header.ElementSize) {
          case 2: {
            vpmullw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 4: {
            vpmulld(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }
          break;
        }
        case IR::OP_VUMULL: {
          auto Op = IROp->C<IR::IROp_VUMull>();
          switch (Op->Header.ElementSize) {
            case 4: {
              // IR operation:
              // [31:00 ] = src1[15:00] * src2[15:00]
              // [63:32 ] = src1[31:16] * src2[31:16]
              // [95:64 ] = src1[47:32] * src2[47:32]
              // [127:96] = src1[63:48] * src2[63:48]
              //
              vpxor(xmm15, xmm15, xmm15);
              vpxor(xmm14, xmm14, xmm14);
              vpunpcklwd(xmm15, GetSrc(Op->Header.Args[0].ID()), xmm15);
              vpunpcklwd(xmm14, GetSrc(Op->Header.Args[1].ID()), xmm14);
              vpmulld(GetDst(Node), xmm14, xmm15);
              break;
            }
            case 8: {
              // We need to shuffle the data for this one
              // x86 PMULUDQ wants the 32bit values in [31:0] and [95:64]
              // Which then extends out to [63:0] and [127:64]
              vpshufd(xmm14, GetSrc(Op->Header.Args[0].ID()), 0b10'10'00'00);
              vpshufd(xmm15, GetSrc(Op->Header.Args[1].ID()), 0b10'10'00'00);

              vpmuludq(GetDst(Node), xmm14, xmm15);
            break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }
          break;
        }
        case IR::OP_VSMULL: {
          auto Op = IROp->C<IR::IROp_VSMull>();
          switch (Op->Header.ElementSize) {
            case 4: {
              // IR operation:
              // [31:00 ] = src1[15:00] * src2[15:00]
              // [63:32 ] = src1[31:16] * src2[31:16]
              // [95:64 ] = src1[47:32] * src2[47:32]
              // [127:96] = src1[63:48] * src2[63:48]
              //
              vpxor(xmm15, xmm15, xmm15);
              vpxor(xmm14, xmm14, xmm14);
              vpunpcklwd(xmm15, GetSrc(Op->Header.Args[0].ID()), xmm15);
              vpunpcklwd(xmm14, GetSrc(Op->Header.Args[1].ID()), xmm14);
              pslld(xmm15, 16);
              pslld(xmm14, 16);
              psrad(xmm15, 16);
              psrad(xmm14, 16);
              vpmulld(GetDst(Node), xmm14, xmm15);
              break;
            }
            case 8: {
              // We need to shuffle the data for this one
              // x86 PMULDQ wants the 32bit values in [31:0] and [95:64]
              // Which then extends out to [63:0] and [127:64]
              vpshufd(xmm14, GetSrc(Op->Header.Args[0].ID()), 0b10'10'00'00);
              vpshufd(xmm15, GetSrc(Op->Header.Args[1].ID()), 0b10'10'00'00);

              vpmuldq(GetDst(Node), xmm14, xmm15);
            break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }
          break;
        }
        case IR::OP_VUMULL2: {
          auto Op = IROp->C<IR::IROp_VUMull2>();
          switch (Op->Header.ElementSize) {
            case 4: {
              // IR operation:
              // [31:00 ] = src1[79:64  ] * src2[79:64  ]
              // [63:32 ] = src1[95:80  ] * src2[95:80  ]
              // [95:64 ] = src1[111:96 ] * src2[111:96 ]
              // [127:96] = src1[127:112] * src2[127:112]
              //
              vpxor(xmm15, xmm15, xmm15);
              vpxor(xmm14, xmm14, xmm14);
              vpunpckhwd(xmm15, GetSrc(Op->Header.Args[0].ID()), xmm15);
              vpunpckhwd(xmm14, GetSrc(Op->Header.Args[1].ID()), xmm14);
              vpmulld(GetDst(Node), xmm14, xmm15);
              break;
            }
            case 8: {
              // IR operation:
              // [63:00 ] = src1[95:64 ] * src2[95:64 ]
              // [127:64] = src1[127:96] * src2[127:96]
              //
              // x86 vpmuludq
              // [63:00 ] = src1[31:0 ] * src2[31:0 ]
              // [127:64] = src1[95:64] * src2[95:64]

              vpshufd(xmm14, GetSrc(Op->Header.Args[0].ID()), 0b11'11'10'10);
              vpshufd(xmm15, GetSrc(Op->Header.Args[1].ID()), 0b11'11'10'10);

              vpmuludq(GetDst(Node), xmm14, xmm15);
            break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }
          break;
        }
        case IR::OP_VSMULL2: {
          auto Op = IROp->C<IR::IROp_VSMull2>();
          switch (Op->Header.ElementSize) {
            case 4: {
              // IR operation:
              // [31:00 ] = src1[79:64  ] * src2[79:64  ]
              // [63:32 ] = src1[95:80  ] * src2[95:80  ]
              // [95:64 ] = src1[111:96 ] * src2[111:96 ]
              // [127:96] = src1[127:112] * src2[127:112]
              //
              vpxor(xmm15, xmm15, xmm15);
              vpxor(xmm14, xmm14, xmm14);
              vpunpckhwd(xmm15, GetSrc(Op->Header.Args[0].ID()), xmm15);
              vpunpckhwd(xmm14, GetSrc(Op->Header.Args[1].ID()), xmm14);
              pslld(xmm15, 16);
              pslld(xmm14, 16);
              psrad(xmm15, 16);
              psrad(xmm14, 16);
              vpmulld(GetDst(Node), xmm14, xmm15);
              break;
            }
            case 8: {
              // IR operation:
              // [63:00 ] = src1[95:64 ] * src2[95:64 ]
              // [127:64] = src1[127:96] * src2[127:96]
              //
              // x86 vpmuludq
              // [63:00 ] = src1[31:0 ] * src2[31:0 ]
              // [127:64] = src1[95:64] * src2[95:64]

              vpshufd(xmm14, GetSrc(Op->Header.Args[0].ID()), 0b11'11'10'10);
              vpshufd(xmm15, GetSrc(Op->Header.Args[1].ID()), 0b11'11'10'10);

              vpmuldq(GetDst(Node), xmm14, xmm15);
            break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }
          break;
        }
        case IR::OP_VMOV: {
          auto Op = IROp->C<IR::IROp_VMov>();

          switch (OpSize) {
            case 1: {
              vpxor(xmm15, xmm15, xmm15);
              pextrb(eax, GetSrc(Op->Header.Args[0].ID()), 0);
              pinsrb(xmm15, eax, 0);
              movapd(GetDst(Node), xmm15);
              break;
            }
            case 2: {
              vpxor(xmm15, xmm15, xmm15);
              pextrw(eax, GetSrc(Op->Header.Args[0].ID()), 0);
              pinsrw(xmm15, eax, 0);
              movapd(GetDst(Node), xmm15);
              break;
            }
            case 4: {
              vpxor(xmm15, xmm15, xmm15);
              pextrd(eax, GetSrc(Op->Header.Args[0].ID()), 0);
              pinsrd(xmm15, eax, 0);
              movapd(GetDst(Node), xmm15);
              break;
            }
            case 8: {
              movq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
              break;
            }
            case 16: {
              movaps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
              break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", OpSize); break;
          }

          break;
        }
        case IR::OP_VAND: {
          auto Op = IROp->C<IR::IROp_VAnd>();
          vpand(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
        }
        case IR::OP_VNEG: {
          auto Op = IROp->C<IR::IROp_VNeg>();
          vpxor(xmm15, xmm15, xmm15);
          switch (Op->Header.ElementSize) {
            case 1: {
              vpsubb(GetDst(Node), xmm15, GetSrc(Op->Header.Args[0].ID()));
            break;
            }
            case 2: {
              vpsubw(GetDst(Node), xmm15, GetSrc(Op->Header.Args[0].ID()));
            break;
            }
            case 4: {
              vpsubd(GetDst(Node), xmm15, GetSrc(Op->Header.Args[0].ID()));
            break;
            }
            case 8: {
              vpsubq(GetDst(Node), xmm15, GetSrc(Op->Header.Args[0].ID()));
            break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }
          break;
        }
        case IR::OP_VFNEG: {
          auto Op = IROp->C<IR::IROp_VNeg>();
          switch (Op->Header.ElementSize) {
            case 4: {
              mov(rax, 0x80000000);
              vmovd(xmm15, eax);
              pshufd(xmm15, xmm15, 0);
              vxorps(GetDst(Node), xmm15, GetSrc(Op->Header.Args[0].ID()));
            break;
            }
            case 8: {
              mov(rax, 0x8000000000000000ULL);
              vmovq(xmm15, rax);
              pshufd(xmm15, xmm15, 0b01'00'01'00);
              vxorpd(GetDst(Node), xmm15, GetSrc(Op->Header.Args[0].ID()));
            break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }
          break;
        }
        case IR::OP_VNOT: {
          auto Op = IROp->C<IR::IROp_VNot>();
          pcmpeqd(xmm15, xmm15);
          vpxor(GetDst(Node), xmm15, GetSrc(Op->Header.Args[0].ID()));
          break;
        }
        case IR::OP_VXOR: {
          auto Op = IROp->C<IR::IROp_VXor>();
          vpxor(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
        }
        case IR::OP_VOR: {
          auto Op = IROp->C<IR::IROp_VOr>();
          vpor(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
        }
        case IR::OP_VFADD: {
          auto Op = IROp->C<IR::IROp_VFAdd>();
          if (Op->Header.ElementSize == OpSize) {
            // Scalar
            switch (Op->Header.ElementSize) {
              case 4: {
                vaddss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              case 8: {
                vaddsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
          }
          else {
            // Vector
            switch (Op->Header.ElementSize) {
              case 4: {
                vaddps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              case 8: {
                vaddpd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
          }
          break;
        }
        case IR::OP_VFADDP: {
          auto Op = IROp->C<IR::IROp_VFAddP>();
          switch (Op->Header.ElementSize) {
            case 4:
              vhaddps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
            break;
            case 8:
              vhaddpd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
            break;
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }
          break;
        }
        case IR::OP_VFSUB: {
          auto Op = IROp->C<IR::IROp_VFSub>();
          if (Op->Header.ElementSize == OpSize) {
            // Scalar
            switch (Op->Header.ElementSize) {
              case 4: {
                vsubss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              case 8: {
                vsubsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
          }
          else {
            // Vector
            switch (Op->Header.ElementSize) {
              case 4: {
                vsubps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              case 8: {
                vsubpd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
          }
          break;
        }
        case IR::OP_VFMUL: {
          auto Op = IROp->C<IR::IROp_VFMul>();
          if (Op->Header.ElementSize == OpSize) {
            // Scalar
            switch (Op->Header.ElementSize) {
              case 4: {
                vmulss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              case 8: {
                vmulsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
          }
          else {
            // Vector
            switch (Op->Header.ElementSize) {
              case 4: {
                vmulps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              case 8: {
                vmulpd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
          }
          break;
        }
        case IR::OP_VFDIV: {
          auto Op = IROp->C<IR::IROp_VFDiv>();
          if (Op->Header.ElementSize == OpSize) {
            // Scalar
            switch (Op->Header.ElementSize) {
              case 4: {
                vdivss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              case 8: {
                vdivsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
          }
          else {
            // Vector
            switch (Op->Header.ElementSize) {
              case 4: {
                vdivps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              case 8: {
                vdivpd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
          }
          break;
        }
        case IR::OP_VFMAX: {
          auto Op = IROp->C<IR::IROp_VFMax>();
          if (Op->Header.ElementSize == OpSize) {
            // Scalar
            switch (Op->Header.ElementSize) {
              case 4: {
                vmaxss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              case 8: {
                vmaxsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
          }
          else {
            // Vector
            switch (Op->Header.ElementSize) {
              case 4: {
                vmaxps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              case 8: {
                vmaxpd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
          }
          break;
        }
        case IR::OP_VFMIN: {
          auto Op = IROp->C<IR::IROp_VFMin>();
          if (Op->Header.ElementSize == OpSize) {
            // Scalar
            switch (Op->Header.ElementSize) {
              case 4: {
                vminss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              case 8: {
                vminsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
          }
          else {
            // Vector
            switch (Op->Header.ElementSize) {
              case 4: {
                vminps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              case 8: {
                vminpd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
              break;
              }
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
          }
          break;
        }

        case IR::OP_VFRECP: {
          auto Op = IROp->C<IR::IROp_VFRecp>();
          if (Op->Header.ElementSize == OpSize) {
            // Scalar
            switch (Op->Header.ElementSize) {
              case 4: {
                mov(eax, 0x3f800000); // 1.0f
                vmovd(xmm15, eax);
                vdivss(GetDst(Node), xmm15, GetSrc(Op->Header.Args[0].ID()));
              break;
              }
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
          }
          else {
            // Vector
            switch (Op->Header.ElementSize) {
              case 4: {
                mov(eax, 0x3f800000); // 1.0f
                vmovd(xmm15, eax);
                pshufd(xmm15, xmm15, 0);
                vdivps(GetDst(Node), xmm15, GetSrc(Op->Header.Args[0].ID()));
              break;
              }
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
          }
          break;
        }
        case IR::OP_VFSQRT: {
          auto Op = IROp->C<IR::IROp_VFSqrt>();
          if (Op->Header.ElementSize == OpSize) {
            // Scalar
            switch (Op->Header.ElementSize) {
              case 4: {
                vsqrtss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[0].ID()));
              break;
              }
              case 8: {
                vsqrtsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[0].ID()));
              break;
              }
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
          }
          else {
            // Vector
            switch (Op->Header.ElementSize) {
              case 4: {
                vsqrtps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
              break;
              }
              case 8: {
                vsqrtpd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
              break;
              }
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
          }
          break;
        }
        case IR::OP_VFRSQRT: {
          auto Op = IROp->C<IR::IROp_VFRSqrt>();
          if (Op->Header.ElementSize == OpSize) {
            // Scalar
            switch (Op->Header.ElementSize) {
              case 4: {
                mov(eax, 0x3f800000); // 1.0f
                sqrtss(xmm15, GetSrc(Op->Header.Args[0].ID()));
                vmovd(GetDst(Node), eax);
                divss(GetDst(Node), xmm15);
              break;
              }
              case 8: {
                mov(eax, 0x3f800000); // 1.0f
                sqrtsd(xmm15, GetSrc(Op->Header.Args[0].ID()));
                vmovd(GetDst(Node), eax);
                divsd(GetDst(Node), xmm15);
              break;
              }
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
          }
          else {
            // Vector
            switch (Op->Header.ElementSize) {
              case 4: {
                mov(rax, 0x3f800000); // 1.0f
                sqrtps(xmm15, GetSrc(Op->Header.Args[0].ID()));
                vmovd(GetDst(Node), eax);
                pshufd(GetDst(Node), GetDst(Node), 0);
                divps(GetDst(Node), xmm15);
              break;
              }
              default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
          }
          break;
        }
        case IR::OP_VSQXTN: {
          auto Op = IROp->C<IR::IROp_VSQXTN>();
          switch (Op->Header.ElementSize) {
            case 1:
              packsswb(xmm15, GetSrc(Op->Header.Args[0].ID()));
            break;
            case 2:
              packssdw(xmm15, GetSrc(Op->Header.Args[0].ID()));
            break;
            default: LogMan::Msg::A("Unknown element size: %d", Op->Header.ElementSize);
          }
          psrldq(xmm15, 8);
          movaps(GetDst(Node), xmm15);
          break;
        }
        case IR::OP_VSQXTN2: {
          auto Op = IROp->C<IR::IROp_VSQXTN2>();
          // Zero the lower bits
          vpxor(xmm15, xmm15, xmm15);
          switch (Op->Header.ElementSize) {
            case 1:
              packsswb(xmm15, GetSrc(Op->Header.Args[1].ID()));
            break;
            case 2:
              packssdw(xmm15, GetSrc(Op->Header.Args[1].ID()));
            break;
            default: LogMan::Msg::A("Unknown element size: %d", Op->Header.ElementSize);
          }

          if (OpSize == 8) {
            psrldq(xmm15, OpSize / 2);
          }
          vpor(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), xmm15);
          break;
        }
        case IR::OP_VSQXTUN: {
          auto Op = IROp->C<IR::IROp_VSQXTUN>();
          switch (Op->Header.ElementSize) {
            case 1:
              packuswb(xmm15, GetSrc(Op->Header.Args[0].ID()));
            break;
            case 2:
              packusdw(xmm15, GetSrc(Op->Header.Args[0].ID()));
            break;
            default: LogMan::Msg::A("Unknown element size: %d", Op->Header.ElementSize);
          }
          psrldq(xmm15, 8);
          movaps(GetDst(Node), xmm15);
          break;
        }
        case IR::OP_VSQXTUN2: {
          auto Op = IROp->C<IR::IROp_VSQXTUN2>();
          // Zero the lower bits
          vpxor(xmm15, xmm15, xmm15);
          switch (Op->Header.ElementSize) {
            case 1:
              packuswb(xmm15, GetSrc(Op->Header.Args[1].ID()));
            break;
            case 2:
              packusdw(xmm15, GetSrc(Op->Header.Args[1].ID()));
            break;
            default: LogMan::Msg::A("Unknown element size: %d", Op->Header.ElementSize);
          }
          if (OpSize == 8) {
            psrldq(xmm15, OpSize / 2);
          }

          vpor(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), xmm15);
          break;
        }
        case IR::OP_VSXTL: {
          auto Op = IROp->C<IR::IROp_VSXTL>();
          switch (Op->Header.ElementSize) {
            case 2:
              pmovsxbw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
            break;
            case 4:
              pmovsxwd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
            break;
            case 8:
              pmovsxdq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
            break;
            default: LogMan::Msg::A("Unknown element size: %d", Op->Header.ElementSize);
          }
          break;
        }
        case IR::OP_VSXTL2: {
          auto Op = IROp->C<IR::IROp_VSXTL2>();
          vpsrldq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), OpSize / 2);
          switch (Op->Header.ElementSize) {
            case 2:
              pmovsxbw(GetDst(Node), GetDst(Node));
            break;
            case 4:
              pmovsxwd(GetDst(Node), GetDst(Node));
            break;
            case 8:
              pmovsxdq(GetDst(Node), GetDst(Node));
            break;
            default: LogMan::Msg::A("Unknown element size: %d", Op->Header.ElementSize);
          }
          break;
        }
        case IR::OP_VUXTL: {
          auto Op = IROp->C<IR::IROp_VUXTL>();
          switch (Op->Header.ElementSize) {
            case 2:
              pmovzxbw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
            break;
            case 4:
              pmovzxwd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
            break;
            case 8:
              pmovzxdq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
            break;
            default: LogMan::Msg::A("Unknown element size: %d", Op->Header.ElementSize);
          }
          break;
        }
        case IR::OP_VUXTL2: {
          auto Op = IROp->C<IR::IROp_VUXTL2>();
          vpslldq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), OpSize / 2);
          switch (Op->Header.ElementSize) {
            case 2:
              pmovzxbw(GetDst(Node), GetDst(Node));
            break;
            case 4:
              pmovzxwd(GetDst(Node), GetDst(Node));
            break;
            case 8:
              pmovzxdq(GetDst(Node), GetDst(Node));
            break;
            default: LogMan::Msg::A("Unknown element size: %d", Op->Header.ElementSize);
          }
          break;
        }
        case IR::OP_VECTOR_STOF: {
          auto Op = IROp->C<IR::IROp_Vector_SToF>();
          switch (Op->Header.ElementSize) {
            case 4:
              cvtdq2ps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
            break;
            case 8:
              // This operation is a bit disgusting in x86
              // There is no vector form of this instruction until AVX512VL + AVX512DQ (vcvtqq2pd)
              // 1) First extract the top 64bits
              // 2) Do a scalar conversion on each
              // 3) Make sure to merge them together at the end
              pextrq(rax, GetSrc(Op->Header.Args[0].ID()), 1);
              pextrq(rcx, GetSrc(Op->Header.Args[0].ID()), 0);
              cvtsi2sd(GetDst(Node), rcx);
              cvtsi2sd(xmm15, rax);
              movlhps(GetDst(Node), xmm15);
            break;
            default: LogMan::Msg::A("Unknown castGPR element size: %d", Op->Header.ElementSize);
          }
          break;
        }
        case IR::OP_VECTOR_FTOZS: {
          auto Op = IROp->C<IR::IROp_Vector_FToZS>();
          switch (Op->Header.ElementSize) {
            case 4:
              cvttps2dq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
            break;
            case 8:
              cvttpd2dq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
            break;
            default: LogMan::Msg::A("Unknown castGPR element size: %d", Op->Header.ElementSize);
          }
          break;
        }
        case IR::OP_VECTOR_FTOF: {
          auto Op = IROp->C<IR::IROp_Vector_FToF>();
          uint16_t Conv = (Op->Header.ElementSize << 8) | Op->SrcElementSize;

          switch (Conv) {
            case 0x0804: { // Double <- Float
              cvtps2pd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
              break;
            }
            case 0x0408: { // Float <- Double
              cvtpd2ps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
              break;
            }
            default: LogMan::Msg::A("Unknown Conversion Type : 0%04x", Conv); break;
          }
          break;
        }
        case IR::OP_VBITCAST: {
          auto Op = IROp->C<IR::IROp_VBitcast>();
          movaps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
        break;
        }
        case IR::OP_VCASTFROMGPR: {
          auto Op = IROp->C<IR::IROp_VCastFromGPR>();
          switch (Op->Header.ElementSize) {
            case 1:
              movzx(rax, GetSrc<RA_8>(Op->Header.Args[0].ID()));
              vmovq(GetDst(Node), rax);
            break;
            case 2:
              movzx(rax, GetSrc<RA_16>(Op->Header.Args[0].ID()));
              vmovq(GetDst(Node), rax);
            break;
            case 4:
              vmovd(GetDst(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()).cvt32());
            break;
            case 8:
              vmovq(GetDst(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()).cvt64());
            break;
            default: LogMan::Msg::A("Unknown castGPR element size: %d", Op->Header.ElementSize);
          }
          break;
        }
        case IR::OP_VCMPEQ: {
          auto Op = IROp->C<IR::IROp_VCMPEQ>();

          switch (Op->Header.ElementSize) {
          case 1:
            vpcmpeqb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          case 2:
            vpcmpeqw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          case 4:
            vpcmpeqd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          case 8:
            vpcmpeqq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
          }
          break;
        }
        case IR::OP_VCMPGT: {
          auto Op = IROp->C<IR::IROp_VCMPGT>();

          switch (Op->Header.ElementSize) {
          case 1:
            vpcmpgtb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          case 2:
            vpcmpgtw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          case 4:
            vpcmpgtd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          case 8:
            vpcmpgtq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
          }
          break;
        }
        case IR::OP_VFCMPEQ: {
          auto Op = IROp->C<IR::IROp_VFCMPEQ>();

          if (Op->Header.ElementSize == OpSize) {
            switch (Op->Header.ElementSize) {
            case 4:
              vcmpss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 0);
            break;
            case 8:
              vcmpsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 0);
            break;
            default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
            }
          }
          else {
            switch (Op->Header.ElementSize) {
            case 4:
              vcmpps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 0);
            break;
            case 8:
              vcmppd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 0);
            break;
            default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
            }
          }
          break;
        }
        case IR::OP_VFCMPLT: {
          auto Op = IROp->C<IR::IROp_VFCMPLT>();
          if (Op->Header.ElementSize == OpSize) {
            switch (Op->Header.ElementSize) {
            case 4:
              vcmpss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 1);
            break;
            case 8:
              vcmpsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 1);
            break;
            default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
            }
          }
          else {
            switch (Op->Header.ElementSize) {
            case 4:
              vcmpps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 1);
            break;
            case 8:
              vcmppd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 1);
            break;
            default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
            }
          }
          break;
        }
        case IR::OP_VFCMPNEQ: {
          auto Op = IROp->C<IR::IROp_VFCMPNEQ>();
          if (Op->Header.ElementSize == OpSize) {
            switch (Op->Header.ElementSize) {
            case 4:
              vcmpss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 4);
            break;
            case 8:
              vcmpsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 4);
            break;
            default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
            }

          }
          else {
            switch (Op->Header.ElementSize) {
            case 4:
              vcmpps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 4);
            break;
            case 8:
              vcmppd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 4);
            break;
            default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
            }
          }
          break;
        }
        case IR::OP_VFCMPGT: {
          auto Op = IROp->C<IR::IROp_VFCMPGT>();
          if (Op->Header.ElementSize == OpSize) {
            switch (Op->Header.ElementSize) {
            case 4:
              vcmpss(GetDst(Node), GetSrc(Op->Header.Args[1].ID()), GetSrc(Op->Header.Args[0].ID()), 1);
            break;
            case 8:
              vcmpsd(GetDst(Node), GetSrc(Op->Header.Args[1].ID()), GetSrc(Op->Header.Args[0].ID()), 1);
            break;
            default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
            }

          }
          else {
            switch (Op->Header.ElementSize) {
            case 4:
              vcmpps(GetDst(Node), GetSrc(Op->Header.Args[1].ID()), GetSrc(Op->Header.Args[0].ID()), 1);
            break;
            case 8:
              vcmppd(GetDst(Node), GetSrc(Op->Header.Args[1].ID()), GetSrc(Op->Header.Args[0].ID()), 1);
            break;
            default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
            }
          }
          break;
        }
        case IR::OP_VFCMPLE: {
          auto Op = IROp->C<IR::IROp_VFCMPLE>();
          if (Op->Header.ElementSize == OpSize) {
            switch (Op->Header.ElementSize) {
            case 4:
              vcmpss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 2);
            break;
            case 8:
              vcmpsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 2);
            break;
            default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
            }

          }
          else {
            switch (Op->Header.ElementSize) {
            case 4:
              vcmpps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 2);
            break;
            case 8:
              vcmppd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 2);
            break;
            default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
            }
          }
          break;
        }
        case IR::OP_VFCMPUNO: {
          auto Op = IROp->C<IR::IROp_VFCMPUNO>();
          if (Op->Header.ElementSize == OpSize) {
            switch (Op->Header.ElementSize) {
            case 4:
              vcmpss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 3);
            break;
            case 8:
              vcmpsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 3);
            break;
            default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
            }

          }
          else {
            switch (Op->Header.ElementSize) {
            case 4:
              vcmpps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 3);
            break;
            case 8:
              vcmppd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 3);
            break;
            default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
            }
          }
          break;
        }
        case IR::OP_VFCMPORD: {
          auto Op = IROp->C<IR::IROp_VFCMPORD>();
          if (Op->Header.ElementSize == OpSize) {
            switch (Op->Header.ElementSize) {
            case 4:
              vcmpss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 7);
            break;
            case 8:
              vcmpsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 7);
            break;
            default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
            }

          }
          else {
            switch (Op->Header.ElementSize) {
            case 4:
              vcmpps(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 7);
            break;
            case 8:
              vcmppd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), 7);
            break;
            default: LogMan::Msg::A("Unsupported elementSize: %d", Op->Header.ElementSize);
            }
          }
          break;
        }
        case IR::OP_FCMP: {
          auto Op = IROp->C<IR::IROp_FCmp>();

          if (Op->ElementSize == 4) {
            ucomiss(GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          }
          else {
            ucomisd(GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          }
          mov (rdx, 0);

          lahf();
          if (Op->Flags & (1 << FCMP_FLAG_LT)) {
            sahf();
            mov(rcx, 0);
            setb(cl);
            shl(rcx, FCMP_FLAG_LT);
            or(rdx, rcx);
          }
          if (Op->Flags & (1 << FCMP_FLAG_UNORDERED)) {
            sahf();
            mov(rcx, 0);
            setp(cl);
            shl(rcx, FCMP_FLAG_UNORDERED);
            or(rdx, rcx);
          }
          if (Op->Flags & (1 << FCMP_FLAG_EQ)) {
            sahf();
            mov(rcx, 0);
            setz(cl);
            shl(rcx, FCMP_FLAG_EQ);
            or(rdx, rcx);
          }
          mov (GetDst<RA_64>(Node), rdx);
          break;
        }
        case IR::OP_GETHOSTFLAG: {
          auto Op = IROp->C<IR::IROp_GetHostFlag>();

          mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          shr(rax, Op->Flag);
          and(rax, 1);
          mov(GetDst<RA_64>(Node), rax);
          break;
        }
        case IR::OP_VZIP: {
          auto Op = IROp->C<IR::IROp_VZip>();
          movapd(xmm15, GetSrc(Op->Header.Args[0].ID()));

          switch (Op->Header.ElementSize) {
          case 1: {
            punpcklbw(xmm15, GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 2: {
            punpcklwd(xmm15, GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 4: {
            punpckldq(xmm15, GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 8: {
            punpcklqdq(xmm15, GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }
          movapd(GetDst(Node), xmm15);
          break;
        }
        case IR::OP_VZIP2: {
          auto Op = IROp->C<IR::IROp_VZip2>();
          movapd(xmm15, GetSrc(Op->Header.Args[0].ID()));

          if (OpSize == 8) {
            vpslldq(xmm15, GetSrc(Op->Header.Args[0].ID()), 4);
            vpslldq(xmm14, GetSrc(Op->Header.Args[1].ID()), 4);
            switch (Op->Header.ElementSize) {
            case 1: {
              vpunpckhbw(GetDst(Node), xmm15, xmm14);
            break;
            }
            case 2: {
              vpunpckhwd(GetDst(Node), xmm15, xmm14);
            break;
            }
            case 4: {
              vpunpckhdq(GetDst(Node), xmm15, xmm14);
            break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
          }
          else {
            switch (Op->Header.ElementSize) {
            case 1: {
              punpckhbw(xmm15, GetSrc(Op->Header.Args[1].ID()));
            break;
            }
            case 2: {
              punpckhwd(xmm15, GetSrc(Op->Header.Args[1].ID()));
            break;
            }
            case 4: {
              punpckhdq(xmm15, GetSrc(Op->Header.Args[1].ID()));
            break;
            }
            case 8: {
              punpckhqdq(xmm15, GetSrc(Op->Header.Args[1].ID()));
            break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
            }
            movapd(GetDst(Node), xmm15);
          }
          break;
        }
        case IR::OP_VUSHLS: {
          auto Op = IROp->C<IR::IROp_VUShlS>();

          switch (Op->Header.ElementSize) {
          case 2: {
            vpsllw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 4: {
            vpslld(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 8: {
            vpsllq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }

          break;
        }
        case IR::OP_VUSHRS: {
          auto Op = IROp->C<IR::IROp_VUShrS>();

          switch (Op->Header.ElementSize) {
          case 2: {
            vpsrlw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 4: {
            vpsrld(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 8: {
            vpsrlq(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }

          break;
        }
        case IR::OP_VSSHRS: {
          auto Op = IROp->C<IR::IROp_VSShrS>();

          switch (Op->Header.ElementSize) {
          case 2: {
            vpsraw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 4: {
            vpsrad(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 8: // Doesn't exist on x86
          default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }

          break;
        }
        case IR::OP_VSLI: {
          auto Op = IROp->C<IR::IROp_VSLI>();
          movapd(xmm15, GetSrc(Op->Header.Args[0].ID()));
          pslldq(xmm15, Op->ByteShift);
          movapd(GetDst(Node), xmm15);
          break;
        }
        case IR::OP_VSRI: {
          auto Op = IROp->C<IR::IROp_VSRI>();
          movapd(xmm15, GetSrc(Op->Header.Args[0].ID()));
          psrldq(xmm15, Op->ByteShift);
          movapd(GetDst(Node), xmm15);
          break;
        }
        case IR::OP_VUSHRI: {
          auto Op = IROp->C<IR::IROp_VUShrI>();
          movapd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
          switch (Op->Header.ElementSize) {
            case 2: {
              psrlw(GetDst(Node), Op->BitShift);
              break;
            }
            case 4: {
              psrld(GetDst(Node), Op->BitShift);
              break;
            }
            case 8: {
              psrlq(GetDst(Node), Op->BitShift);
              break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }
          break;
        }
        case IR::OP_VSHLI: {
          auto Op = IROp->C<IR::IROp_VShlI>();
          movapd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
          switch (Op->Header.ElementSize) {
            case 2: {
              psllw(GetDst(Node), Op->BitShift);
              break;
            }
            case 4: {
              pslld(GetDst(Node), Op->BitShift);
              break;
            }
            case 8: {
              psllq(GetDst(Node), Op->BitShift);
              break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }
          break;
        }
        case IR::OP_VSSHRI: {
          auto Op = IROp->C<IR::IROp_VSShrI>();
          movapd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
          switch (Op->Header.ElementSize) {
            case 2: {
              psraw(GetDst(Node), Op->BitShift);
              break;
            }
            case 4: {
              psrad(GetDst(Node), Op->BitShift);
              break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }
          break;
        }
        case IR::OP_VUSHRNI: {
          auto Op = IROp->C<IR::IROp_VUShrNI>();
          movapd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
          vpxor(xmm15, xmm15, xmm15);
          switch (Op->Header.ElementSize) {
            case 1: {
              psrlw(GetDst(Node), Op->BitShift);
              // <8 x i16> -> <8 x i8>
              mov(rax, 0x0E'0C'0A'08'06'04'02'00); // Lower
              mov(rcx, 0x80'80'80'80'80'80'80'80); // Upper
              break;
            }
            case 2: {
              psrld(GetDst(Node), Op->BitShift);
              // <4 x i32> -> <4 x i16>
              mov(rax, 0x0D'0C'09'08'05'04'01'00); // Lower
              mov(rcx, 0x80'80'80'80'80'80'80'80); // Upper
              break;
            }
            case 4: {
              psrlq(GetDst(Node), Op->BitShift);
              // <2 x i64> -> <2 x i32>
              mov(rax, 0x0B'0A'09'08'03'02'01'00); // Lower
              mov(rcx, 0x80'80'80'80'80'80'80'80); // Upper
              break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }

          vmovq(xmm15, rax);
          vmovq(xmm14, rcx);
          punpcklqdq(xmm15, xmm14);
          pshufb(GetDst(Node), xmm15);
          break;
        }
        case IR::OP_VUSHRNI2: {
          // Src1 = Lower results
          // Src2 = Upper Results
          auto Op = IROp->C<IR::IROp_VUShrNI2>();
          movapd(xmm13, GetSrc(Op->Header.Args[1].ID()));
          switch (Op->Header.ElementSize) {
            case 1: {
              psrlw(xmm13, Op->BitShift);
              // <8 x i16> -> <8 x i8>
              mov(rax, 0x80'80'80'80'80'80'80'80); // Lower
              mov(rcx, 0x0E'0C'0A'08'06'04'02'00); // Upper
              break;
            }
            case 2: {
              psrld(xmm13, Op->BitShift);
              // <4 x i32> -> <4 x i16>
              mov(rax, 0x80'80'80'80'80'80'80'80); // Lower
              mov(rcx, 0x0D'0C'09'08'05'04'01'00); // Upper
              break;
            }
            case 4: {
              psrlq(xmm13, Op->BitShift);
              // <2 x i64> -> <2 x i32>
              mov(rax, 0x80'80'80'80'80'80'80'80); // Lower
              mov(rcx, 0x0B'0A'09'08'03'02'01'00); // Upper
              break;
            }
            default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }

          vmovq(xmm15, rax);
          vmovq(xmm14, rcx);
          punpcklqdq(xmm15, xmm14);
          vpshufb(xmm14, xmm13, xmm15);
          vpor(GetDst(Node), xmm14, GetSrc(Op->Header.Args[0].ID()));
          break;
        }
        case IR::OP_VEXTR: {
          auto Op = IROp->C<IR::IROp_VExtr>();
          vpalignr(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()), Op->Index);
          break;
        }
        case IR::OP_VUMIN: {
          auto Op = IROp->C<IR::IROp_VUMin>();
          switch (Op->Header.ElementSize) {
          case 1: {
            vpminub(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 2: {
            vpminuw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 4: {
            vpminud(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }
          break;
        }
        case IR::OP_VSMIN: {
          auto Op = IROp->C<IR::IROp_VSMin>();
          switch (Op->Header.ElementSize) {
          case 1: {
            vpminsb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 2: {
            vpminsw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 4: {
            vpminsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }
          break;
        }
        case IR::OP_VUMAX: {
          auto Op = IROp->C<IR::IROp_VUMax>();
          switch (Op->Header.ElementSize) {
          case 1: {
            vpmaxub(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 2: {
            vpmaxuw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 4: {
            vpmaxud(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }
          break;
        }
        case IR::OP_VSMAX: {
          auto Op = IROp->C<IR::IROp_VSMax>();
          switch (Op->Header.ElementSize) {
          case 1: {
            vpmaxsb(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 2: {
            vpmaxsw(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          case 4: {
            vpmaxsd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()), GetSrc(Op->Header.Args[1].ID()));
          break;
          }
          default: LogMan::Msg::A("Unknown Element Size: %d", Op->Header.ElementSize); break;
          }
          break;
        }
        case IR::OP_FLOAT_FROMGPR_S: {
          auto Op = IROp->C<IR::IROp_Float_FromGPR_S>();
          if (Op->Header.ElementSize == 8) {
            cvtsi2sd(GetDst(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()));
          }
          else
            cvtsi2ss(GetDst(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()));
          break;
        }
        case IR::OP_FLOAT_TOGPR_ZS: {
          auto Op = IROp->C<IR::IROp_Float_ToGPR_ZS>();
          if (Op->Header.ElementSize == 8) {
            cvttsd2si(GetDst<RA_64>(Node), GetSrc(Op->Header.Args[0].ID()));
          }
          else
            cvttss2si(GetDst<RA_32>(Node), GetSrc(Op->Header.Args[0].ID()));
          break;
        }
        case IR::OP_FLOAT_FTOF: {
          auto Op = IROp->C<IR::IROp_Float_FToF>();
          uint16_t Conv = (Op->Header.ElementSize << 8) | Op->SrcElementSize;
          switch (Conv) {
            case 0x0804: { // Double <- Float
              cvtss2sd(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
              break;
            }
            case 0x0408: { // Float <- Double
              cvtsd2ss(GetDst(Node), GetSrc(Op->Header.Args[0].ID()));
              break;
            }
            default: LogMan::Msg::A("Unknown FCVT sizes: 0x%x", Conv);
          }
          break;
        }
        case IR::OP_CAS: {
          auto Op = IROp->C<IR::IROp_CAS>();
          // Args[0]: Desired
          // Args[1]: Expected
          // Args[2]: Pointer
          // DataSrc = *Src1
          // if (DataSrc == Src3) { *Src1 == Src2; } Src2 = DataSrc
          // This will write to memory! Careful!
          // Third operand must be a calculated guest memory address
          //OrderedNode *CASResult = _CAS(Src3, Src2, Src1);
          uint64_t Memory = CTX->MemoryMapper.GetBaseOffset<uint64_t>(0);

          Xbyak::Reg MemReg = rcx;
          if (CTX->Config.UnifiedMemory) {
            MemReg = GetSrc<RA_64>(Op->Header.Args[2].ID());
          }
          else {
            mov(MemReg, Memory);
            add(MemReg, GetSrc<RA_64>(Op->Header.Args[2].ID()));
          }

          mov(rdx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
          mov(rax, GetSrc<RA_64>(Op->Header.Args[0].ID()));

          // RCX now contains pointer
          // RAX contains our expected value
          // RDX contains our desired

          lock();

          switch (OpSize) {
          case 1: {
            cmpxchg(byte [MemReg], dl);
            movzx(rax, al);
          break;
          }
          case 2: {
            cmpxchg(word [MemReg], dx);
            movzx(rax, ax);
          break;
          }
          case 4: {
            cmpxchg(dword [MemReg], edx);
          break;
          }
          case 8: {
            cmpxchg(qword [MemReg], rdx);
          break;
          }
          default: LogMan::Msg::A("Unsupported: %d", OpSize);
          }

          // RAX now contains the result
          mov (GetDst<RA_64>(Node), rax);
          break;
        }
        case IR::OP_ATOMICADD: {
          auto Op = IROp->C<IR::IROp_AtomicAdd>();
          uint64_t Memory = CTX->MemoryMapper.GetBaseOffset<uint64_t>(0);

          Xbyak::Reg MemReg = rax;
          if (CTX->Config.UnifiedMemory) {
            MemReg = GetSrc<RA_64>(Op->Header.Args[0].ID());
          }
          else {
            mov(MemReg, Memory);
            add(MemReg, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          }

          lock();
          switch (Op->Size) {
          case 1:
            add(byte [MemReg], GetSrc<RA_8>(Op->Header.Args[1].ID()));
          break;
          case 2:
            add(word [MemReg], GetSrc<RA_16>(Op->Header.Args[1].ID()));
          break;
          case 4:
            add(dword [MemReg], GetSrc<RA_32>(Op->Header.Args[1].ID()));
          break;
          case 8:
            add(qword [MemReg], GetSrc<RA_64>(Op->Header.Args[1].ID()));
          break;
          default:  LogMan::Msg::A("Unhandled AtomicAdd size: %d", Op->Size);
          }

          break;
        }
        case IR::OP_ATOMICSUB: {
          auto Op = IROp->C<IR::IROp_AtomicSub>();
          uint64_t Memory = CTX->MemoryMapper.GetBaseOffset<uint64_t>(0);

          Xbyak::Reg MemReg = rax;
          if (CTX->Config.UnifiedMemory) {
            MemReg = GetSrc<RA_64>(Op->Header.Args[0].ID());
          }
          else {
            mov(MemReg, Memory);
            add(MemReg, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          }
          lock();
          switch (Op->Size) {
          case 1:
            sub(byte [MemReg], GetSrc<RA_8>(Op->Header.Args[1].ID()));
          break;
          case 2:
            sub(word [MemReg], GetSrc<RA_16>(Op->Header.Args[1].ID()));
          break;
          case 4:
            sub(dword [MemReg], GetSrc<RA_32>(Op->Header.Args[1].ID()));
          break;
          case 8:
            sub(qword [MemReg], GetSrc<RA_64>(Op->Header.Args[1].ID()));
          break;
          default:  LogMan::Msg::A("Unhandled AtomicAdd size: %d", Op->Size);
          }

          break;
        }
        case IR::OP_ATOMICAND: {
          auto Op = IROp->C<IR::IROp_AtomicAnd>();
          uint64_t Memory = CTX->MemoryMapper.GetBaseOffset<uint64_t>(0);

          Xbyak::Reg MemReg = rax;
          if (CTX->Config.UnifiedMemory) {
            MemReg = GetSrc<RA_64>(Op->Header.Args[0].ID());
          }
          else {
            mov(MemReg, Memory);
            add(MemReg, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          }
          lock();
          switch (Op->Size) {
          case 1:
            and(byte [MemReg], GetSrc<RA_8>(Op->Header.Args[1].ID()));
          break;
          case 2:
            and(word [MemReg], GetSrc<RA_16>(Op->Header.Args[1].ID()));
          break;
          case 4:
            and(dword [MemReg], GetSrc<RA_32>(Op->Header.Args[1].ID()));
          break;
          case 8:
            and(qword [MemReg], GetSrc<RA_64>(Op->Header.Args[1].ID()));
          break;
          default:  LogMan::Msg::A("Unhandled AtomicAdd size: %d", Op->Size);
          }

          break;
        }
        case IR::OP_ATOMICOR: {
          auto Op = IROp->C<IR::IROp_AtomicOr>();
          uint64_t Memory = CTX->MemoryMapper.GetBaseOffset<uint64_t>(0);

          Xbyak::Reg MemReg = rax;
          if (CTX->Config.UnifiedMemory) {
            MemReg = GetSrc<RA_64>(Op->Header.Args[0].ID());
          }
          else {
            mov(MemReg, Memory);
            add(MemReg, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          }
          lock();
          switch (Op->Size) {
          case 1:
            or(byte [MemReg], GetSrc<RA_8>(Op->Header.Args[1].ID()));
          break;
          case 2:
            or(word [MemReg], GetSrc<RA_16>(Op->Header.Args[1].ID()));
          break;
          case 4:
            or(dword [MemReg], GetSrc<RA_32>(Op->Header.Args[1].ID()));
          break;
          case 8:
            or(qword [MemReg], GetSrc<RA_64>(Op->Header.Args[1].ID()));
          break;
          default:  LogMan::Msg::A("Unhandled AtomicAdd size: %d", Op->Size);
          }

          break;
        }
        case IR::OP_ATOMICXOR: {
          auto Op = IROp->C<IR::IROp_AtomicXor>();
          uint64_t Memory = CTX->MemoryMapper.GetBaseOffset<uint64_t>(0);

          Xbyak::Reg MemReg = rax;
          if (CTX->Config.UnifiedMemory) {
            MemReg = GetSrc<RA_64>(Op->Header.Args[0].ID());
          }
          else {
            mov(MemReg, Memory);
            add(MemReg, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          }
          lock();
          switch (Op->Size) {
          case 1:
            xor(byte [MemReg], GetSrc<RA_8>(Op->Header.Args[1].ID()));
          break;
          case 2:
            xor(word [MemReg], GetSrc<RA_16>(Op->Header.Args[1].ID()));
          break;
          case 4:
            xor(dword [MemReg], GetSrc<RA_32>(Op->Header.Args[1].ID()));
          break;
          case 8:
            xor(qword [MemReg], GetSrc<RA_64>(Op->Header.Args[1].ID()));
          break;
          default:  LogMan::Msg::A("Unhandled AtomicAdd size: %d", Op->Size);
          }

          break;
        }
        case IR::OP_ATOMICSWAP: {
          auto Op = IROp->C<IR::IROp_AtomicSwap>();
          uint64_t Memory = CTX->MemoryMapper.GetBaseOffset<uint64_t>(0);

          Xbyak::Reg MemReg = rax;
          if (CTX->Config.UnifiedMemory) {
            mov(MemReg, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          }
          else {
            mov(MemReg, Memory);
            add(MemReg, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          }
          switch (Op->Size) {
          case 1:
            mov(GetDst<RA_8>(Node), GetSrc<RA_8>(Op->Header.Args[1].ID()));
            lock();
            xchg(byte [MemReg], GetDst<RA_8>(Node));
          break;
          case 2:
            mov(GetDst<RA_16>(Node), GetSrc<RA_16>(Op->Header.Args[1].ID()));
            lock();
            xchg(word [MemReg], GetDst<RA_16>(Node));
          break;
          case 4:
            mov(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Header.Args[1].ID()));
            lock();
            xchg(dword [MemReg], GetDst<RA_32>(Node));
          break;
          case 8:
            mov(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[1].ID()));
            lock();
            xchg(qword [MemReg], GetDst<RA_64>(Node));
          break;
          default:  LogMan::Msg::A("Unhandled AtomicAdd size: %d", Op->Size);
          }

          break;
        }
        case IR::OP_ATOMICFETCHADD: {
          auto Op = IROp->C<IR::IROp_AtomicFetchAdd>();
          uint64_t Memory = CTX->MemoryMapper.GetBaseOffset<uint64_t>(0);

          Xbyak::Reg MemReg = rax;
          if (CTX->Config.UnifiedMemory) {
            MemReg = GetSrc<RA_64>(Op->Header.Args[0].ID());
          }
          else {
            mov(MemReg, Memory);
            add(MemReg, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          }
          switch (Op->Size) {
          case 1:
            mov(cl, GetSrc<RA_8>(Op->Header.Args[1].ID()));
            lock();
            xadd(byte [MemReg], cl);
            movzx(GetDst<RA_32>(Node), cl);
          break;
          case 2:
            mov(cx, GetSrc<RA_16>(Op->Header.Args[1].ID()));
            lock();
            xadd(word [MemReg], cx);
            movzx(GetDst<RA_32>(Node), cx);
          break;
          case 4:
            mov(ecx, GetSrc<RA_32>(Op->Header.Args[1].ID()));
            lock();
            xadd(dword [MemReg], ecx);
            mov(GetDst<RA_32>(Node), ecx);
          break;
          case 8:
            mov(rcx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
            lock();
            xadd(qword [MemReg], rcx);
            mov(GetDst<RA_64>(Node), rcx);
          break;
          default:  LogMan::Msg::A("Unhandled AtomicFetchAdd size: %d", Op->Size);
          }

          break;
        }
        case IR::OP_ATOMICFETCHSUB: {
          auto Op = IROp->C<IR::IROp_AtomicFetchSub>();
          uint64_t Memory = CTX->MemoryMapper.GetBaseOffset<uint64_t>(0);

          Xbyak::Reg MemReg = rax;
          if (CTX->Config.UnifiedMemory) {
            MemReg = GetSrc<RA_64>(Op->Header.Args[0].ID());
          }
          else {
            mov(MemReg, Memory);
            add(MemReg, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          }
          switch (Op->Size) {
          case 1:
            mov(cl, GetSrc<RA_8>(Op->Header.Args[1].ID()));
            neg(cl);
            lock();
            xadd(byte [MemReg], cl);
            movzx(GetDst<RA_32>(Node), cl);
          break;
          case 2:
            mov(cx, GetSrc<RA_16>(Op->Header.Args[1].ID()));
            neg(cx);
            lock();
            xadd(word [MemReg], cx);
            movzx(GetDst<RA_32>(Node), cx);
          break;
          case 4:
            mov(ecx, GetSrc<RA_32>(Op->Header.Args[1].ID()));
            neg(ecx);
            lock();
            xadd(dword [MemReg], ecx);
            mov(GetDst<RA_32>(Node), ecx);
          break;
          case 8:
            mov(rcx, GetSrc<RA_64>(Op->Header.Args[1].ID()));
            neg(rcx);
            lock();
            xadd(qword [MemReg], rcx);
            mov(GetDst<RA_64>(Node), rcx);
          break;
          default:  LogMan::Msg::A("Unhandled AtomicFetchAdd size: %d", Op->Size);
          }
          break;
        }
        case IR::OP_ATOMICFETCHAND: {
          auto Op = IROp->C<IR::IROp_AtomicFetchAnd>();
          uint64_t Memory = CTX->MemoryMapper.GetBaseOffset<uint64_t>(0);

          // TMP1 = rax
          Xbyak::Reg MemReg = TMP4;
          if (CTX->Config.UnifiedMemory) {
            MemReg = GetSrc<RA_64>(Op->Header.Args[0].ID());
          }
          else {
            mov(MemReg, Memory);
            add(MemReg, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          }

          switch (Op->Size) {
            case 1: {
              mov(TMP1.cvt8(), byte [MemReg]);

              Label Loop;
              L(Loop);
              mov(TMP2.cvt8(), TMP1.cvt8());
              mov(TMP3.cvt8(), TMP1.cvt8());
              and(TMP2.cvt8(), GetSrc<RA_8>(Op->Header.Args[1].ID()));

              // Updates RAX with the value from memory
              lock(); cmpxchg(byte [MemReg], TMP2.cvt8());
              jne(Loop);
              // Result is the previous value from memory, which is currently in TMP3
              movzx(GetDst<RA_64>(Node), TMP3.cvt8());
              break;
            }
            case 2: {
              mov(TMP1.cvt16(), word [MemReg]);

              Label Loop;
              L(Loop);
              mov(TMP2.cvt16(), TMP1.cvt16());
              mov(TMP3.cvt16(), TMP1.cvt16());
              and(TMP2.cvt16(), GetSrc<RA_16>(Op->Header.Args[1].ID()));

              // Updates RAX with the value from memory
              lock(); cmpxchg(word [MemReg], TMP2.cvt16());
              jne(Loop);

              // Result is the previous value from memory, which is currently in TMP3
              movzx(GetDst<RA_64>(Node), TMP3.cvt16());
              break;
            }
            case 4: {
              mov(TMP1.cvt32(), dword [MemReg]);

              Label Loop;
              L(Loop);
              mov(TMP2.cvt32(), TMP1.cvt32());
              mov(TMP3.cvt32(), TMP1.cvt32());
              and(TMP2.cvt32(), GetSrc<RA_32>(Op->Header.Args[1].ID()));

              // Updates RAX with the value from memory
              lock(); cmpxchg(dword [MemReg], TMP2.cvt32());
              jne(Loop);

              // Result is the previous value from memory, which is currently in TMP3
              mov(GetDst<RA_32>(Node), TMP3.cvt32());
              break;
            }
            case 8: {
              mov(TMP1.cvt64(), qword [MemReg]);

              Label Loop;
              L(Loop);
              mov(TMP2.cvt64(), TMP1.cvt64());
              mov(TMP3.cvt64(), TMP1.cvt64());
              and(TMP2.cvt64(), GetSrc<RA_64>(Op->Header.Args[1].ID()));

              // Updates RAX with the value from memory
              lock(); cmpxchg(qword [MemReg], TMP2.cvt64());
              jne(Loop);

              // Result is the previous value from memory, which is currently in TMP3
              mov(GetDst<RA_64>(Node), TMP3.cvt64());
              break;
            }
            default:  LogMan::Msg::A("Unhandled AtomicFetchAdd size: %d", Op->Size);
          }
          break;
        }
        case IR::OP_ATOMICFETCHOR: {
          auto Op = IROp->C<IR::IROp_AtomicFetchOr>();
          uint64_t Memory = CTX->MemoryMapper.GetBaseOffset<uint64_t>(0);

          // TMP1 = rax
          Xbyak::Reg MemReg = TMP4;
          if (CTX->Config.UnifiedMemory) {
            MemReg = GetSrc<RA_64>(Op->Header.Args[0].ID());
          }
          else {
            mov(MemReg, Memory);
            add(MemReg, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          }
          switch (Op->Size) {
            case 1: {
              mov(TMP1.cvt8(), byte [MemReg]);

              Label Loop;
              L(Loop);
              mov(TMP2.cvt8(), TMP1.cvt8());
              mov(TMP3.cvt8(), TMP1.cvt8());
              or(TMP2.cvt8(), GetSrc<RA_8>(Op->Header.Args[1].ID()));

              // Updates RAX with the value from memory
              lock(); cmpxchg(byte [MemReg], TMP2.cvt8());
              jne(Loop);
              // Result is the previous value from memory, which is currently in TMP3
              movzx(GetDst<RA_64>(Node), TMP3.cvt8());
              break;
            }
            case 2: {
              mov(TMP1.cvt16(), word [MemReg]);

              Label Loop;
              L(Loop);
              mov(TMP2.cvt16(), TMP1.cvt16());
              mov(TMP3.cvt16(), TMP1.cvt16());
              or(TMP2.cvt16(), GetSrc<RA_16>(Op->Header.Args[1].ID()));

              // Updates RAX with the value from memory
              lock(); cmpxchg(word [MemReg], TMP2.cvt16());
              jne(Loop);

              // Result is the previous value from memory, which is currently in TMP3
              movzx(GetDst<RA_64>(Node), TMP3.cvt16());
              break;
            }
            case 4: {
              mov(TMP1.cvt32(), dword [MemReg]);

              Label Loop;
              L(Loop);
              mov(TMP2.cvt32(), TMP1.cvt32());
              mov(TMP3.cvt32(), TMP1.cvt32());
              or(TMP2.cvt32(), GetSrc<RA_32>(Op->Header.Args[1].ID()));

              // Updates RAX with the value from memory
              lock(); cmpxchg(dword [MemReg], TMP2.cvt32());
              jne(Loop);

              // Result is the previous value from memory, which is currently in TMP3
              mov(GetDst<RA_32>(Node), TMP3.cvt32());
              break;
            }
            case 8: {
              mov(TMP1.cvt64(), qword [MemReg]);

              Label Loop;
              L(Loop);
              mov(TMP2.cvt64(), TMP1.cvt64());
              mov(TMP3.cvt64(), TMP1.cvt64());
              or(TMP2.cvt64(), GetSrc<RA_64>(Op->Header.Args[1].ID()));

              // Updates RAX with the value from memory
              lock(); cmpxchg(qword [MemReg], TMP2.cvt64());
              jne(Loop);

              // Result is the previous value from memory, which is currently in TMP3
              mov(GetDst<RA_64>(Node), TMP3.cvt64());
              break;
            }
            default:  LogMan::Msg::A("Unhandled AtomicFetchAdd size: %d", Op->Size);
          }
          break;
        }
        case IR::OP_ATOMICFETCHXOR: {
          auto Op = IROp->C<IR::IROp_AtomicFetchXor>();
          uint64_t Memory = CTX->MemoryMapper.GetBaseOffset<uint64_t>(0);

          // TMP1 = rax
          Xbyak::Reg MemReg = TMP4;
          if (CTX->Config.UnifiedMemory) {
            MemReg = GetSrc<RA_64>(Op->Header.Args[0].ID());
          }
          else {
            mov(MemReg, Memory);
            add(MemReg, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          }
          switch (Op->Size) {
            case 1: {
              mov(TMP1.cvt8(), byte [MemReg]);

              Label Loop;
              L(Loop);
              mov(TMP2.cvt8(), TMP1.cvt8());
              mov(TMP3.cvt8(), TMP1.cvt8());
              xor(TMP2.cvt8(), GetSrc<RA_8>(Op->Header.Args[1].ID()));

              // Updates RAX with the value from memory
              lock(); cmpxchg(byte [MemReg], TMP2.cvt8());
              jne(Loop);
              // Result is the previous value from memory, which is currently in TMP3
              movzx(GetDst<RA_64>(Node), TMP3.cvt8());
              break;
            }
            case 2: {
              mov(TMP1.cvt16(), word [MemReg]);

              Label Loop;
              L(Loop);
              mov(TMP2.cvt16(), TMP1.cvt16());
              mov(TMP3.cvt16(), TMP1.cvt16());
              xor(TMP2.cvt16(), GetSrc<RA_16>(Op->Header.Args[1].ID()));

              // Updates RAX with the value from memory
              lock(); cmpxchg(word [MemReg], TMP2.cvt16());
              jne(Loop);

              // Result is the previous value from memory, which is currently in TMP3
              movzx(GetDst<RA_64>(Node), TMP3.cvt16());
              break;
            }
            case 4: {
              mov(TMP1.cvt32(), dword [MemReg]);

              Label Loop;
              L(Loop);
              mov(TMP2.cvt32(), TMP1.cvt32());
              mov(TMP3.cvt32(), TMP1.cvt32());
              xor(TMP2.cvt32(), GetSrc<RA_32>(Op->Header.Args[1].ID()));

              // Updates RAX with the value from memory
              lock(); cmpxchg(dword [MemReg], TMP2.cvt32());
              jne(Loop);

              // Result is the previous value from memory, which is currently in TMP3
              mov(GetDst<RA_32>(Node), TMP3.cvt32());
              break;
            }
            case 8: {
              mov(TMP1.cvt64(), qword [MemReg]);

              Label Loop;
              L(Loop);
              mov(TMP2.cvt64(), TMP1.cvt64());
              mov(TMP3.cvt64(), TMP1.cvt64());
              xor(TMP2.cvt64(), GetSrc<RA_64>(Op->Header.Args[1].ID()));

              // Updates RAX with the value from memory
              lock(); cmpxchg(qword [MemReg], TMP2.cvt64());
              jne(Loop);

              // Result is the previous value from memory, which is currently in TMP3
              mov(GetDst<RA_64>(Node), TMP3.cvt64());
              break;
            }
            default:  LogMan::Msg::A("Unhandled AtomicFetchAdd size: %d", Op->Size);
          }
          break;
        }
        case IR::OP_CYCLECOUNTER: {
          #ifdef DEBUG_CYCLES
          mov (GetDst<RA_64>(Node), 0);
          #else
          rdtsc();
          shl(rdx, 32);
          or(rax, rdx);
          mov (GetDst<RA_64>(Node), rax);
          #endif
          break;
        }
        case IR::OP_FINDLSB: {
          auto Op = IROp->C<IR::IROp_FindLSB>();
          bsf(rcx, GetSrc<RA_64>(Op->Header.Args[0].ID()));
          mov(rax, 0x40);
          cmovz(rcx, rax);
          xor(rax, rax);
          cmp(GetSrc<RA_64>(Op->Header.Args[0].ID()), 1);
          sbb(rax, rax);
          or(rax, rcx);
          mov (GetDst<RA_64>(Node), rax);
          break;
        }
        case IR::OP_FINDMSB: {
          auto Op = IROp->C<IR::IROp_FindMSB>();
          switch (OpSize) {
          case 2:
            bsr(GetDst<RA_16>(Node), GetSrc<RA_16>(Op->Header.Args[0].ID()));
            movzx(GetDst<RA_32>(Node), GetDst<RA_16>(Node));
            break;
          case 4:
            bsr(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()));
            break;
          case 8:
            bsr(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()));
            break;
          default: LogMan::Msg::A("Unknown OpSize: %d", OpSize);
          }
          break;
        }
        case IR::OP_REV: {
          auto Op = IROp->C<IR::IROp_Rev>();
          switch (OpSize) {
            case 2:
              mov (GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()));
              rol(GetDst<RA_16>(Node), 8);
            break;
            case 4:
              mov (GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()));
              bswap(GetDst<RA_32>(Node).cvt32());
              break;
            case 8:
              mov (GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()));
              bswap(GetDst<RA_64>(Node).cvt64());
              break;
            default: LogMan::Msg::A("Unknown REV size: %d", OpSize); break;
          }
          break;
        }
        case IR::OP_FINDTRAILINGZEROS: {
          auto Op = IROp->C<IR::IROp_FindTrailingZeros>();
          switch (OpSize) {
            case 2:
              bsf(GetDst<RA_16>(Node), GetSrc<RA_16>(Op->Header.Args[0].ID()));
              mov(ax, 0x10);
              cmovz(GetDst<RA_16>(Node), ax);
            break;
            case 4:
              bsf(GetDst<RA_32>(Node), GetSrc<RA_32>(Op->Header.Args[0].ID()));
              mov(eax, 0x20);
              cmovz(GetDst<RA_32>(Node), eax);
              break;
            case 8:
              bsf(GetDst<RA_64>(Node), GetSrc<RA_64>(Op->Header.Args[0].ID()));
              mov(rax, 0x40);
              cmovz(GetDst<RA_64>(Node), rax);
              break;
            default: LogMan::Msg::A("Unknown size: %d", OpSize); break;
          }
          break;
        }
        case IR::OP_FENCE: {
          auto Op = IROp->C<IR::IROp_Fence>();
          switch (Op->Fence) {
            case IR::Fence_Load.Val:
              lfence();
              break;
            case IR::Fence_LoadStore.Val:
              mfence();
              break;
            case IR::Fence_Store.Val:
              sfence();
              break;
            default: LogMan::Msg::A("Unknown Fence: %d", Op->Fence); break;
          }
          break;
        }
        case IR::OP_DUMMY:
        case IR::OP_IRHEADER:
        case IR::OP_PHIVALUE:
        case IR::OP_PHI:
          break;
        default:
          LogMan::Msg::A("Unknown IR Op: %d(%s)", IROp->Op, FEXCore::IR::GetName(IROp->Op).data());
          break;
      }

      // CodeLast is inclusive. So we still need to dump the CodeLast op as well
      if (CodeBegin == CodeLast) {
        break;
      }
      ++CodeBegin;
    }

    if (BlockIROp->Next.ID() == 0) {
      break;
    } else {
      BlockNode = BlockIROp->Next.GetNode(ListBegin);
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
