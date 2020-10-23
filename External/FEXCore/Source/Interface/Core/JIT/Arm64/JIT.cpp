#include "Interface/Context/Context.h"

#include "Interface/Core/JIT/Arm64/JITClass.h"
#include "Interface/Core/InternalThreadState.h"

#include "Interface/IR/Passes/RegisterAllocationPass.h"

#include <FEXCore/Core/X86Enums.h>

#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <ucontext.h>

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

void JITCore::Op_Unhandled(FEXCore::IR::IROp_Header *IROp, uint32_t Node) {
  auto Name = FEXCore::IR::GetName(IROp->Op);
  LogMan::Msg::A("Unhandled IR Op: %s", std::string(Name).c_str());
}

void JITCore::Op_NoOp(FEXCore::IR::IROp_Header *IROp, uint32_t Node) {
}

bool JITCore::IsAddressInJITCode(uint64_t Address) {
  // Check the initial code buffer first
  // It's the most likely place to end up

  uint64_t CodeBase = reinterpret_cast<uint64_t>(InitialCodeBuffer.Ptr);
  uint64_t CodeEnd = CodeBase + InitialCodeBuffer.Size;
  if (Address >= CodeBase &&
      Address < CodeEnd) {
    return true;
  }

  // Check the generated code buffers
  // Not likely to have any but can happen with recursive signals
  for (auto &CodeBuffer : CodeBuffers) {
    CodeBase = reinterpret_cast<uint64_t>(CodeBuffer.Ptr);
    CodeEnd = CodeBase + CodeBuffer.Size;
    if (Address >= CodeBase &&
        Address < CodeEnd) {
      return true;
    }
  }

  // Check the dispatcher. Unlikely to crash here but not impossible
  CodeBase = reinterpret_cast<uint64_t>(DispatcherCodeBuffer.Ptr);
  CodeEnd = CodeBase + DispatcherCodeBuffer.Size;
  if (Address >= CodeBase &&
      Address < CodeEnd) {
    return true;
  }
  return false;
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

void JITCore::StoreThreadState(int Signal, void *ucontext) {
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

void JITCore::RestoreThreadState(void *ucontext) {
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

bool JITCore::HandleSIGILL(int Signal, void *info, void *ucontext) {
  ucontext_t* _context = (ucontext_t*)ucontext;
  mcontext_t* _mcontext = &_context->uc_mcontext;

  if (_mcontext->pc == SignalReturnInstruction) {
    RestoreThreadState(ucontext);

    // Ref count our faults
    // We use this to track if it is safe to clear cache
    --SignalHandlerRefCounter;
    return true;
  }

  if (_mcontext->pc == PauseReturnInstruction) {
    RestoreThreadState(ucontext);

    // Ref count our faults
    // We use this to track if it is safe to clear cache
    --SignalHandlerRefCounter;
    return true;
  }

  return false;
}

bool JITCore::HandleGuestSignal(int Signal, void *info, void *ucontext, SignalDelegator::GuestSigAction *GuestAction, stack_t *GuestStack) {
  ucontext_t* _context = (ucontext_t*)ucontext;
  mcontext_t* _mcontext = &_context->uc_mcontext;

  StoreThreadState(Signal, ucontext);

  // Set the new PC
  _mcontext->pc = AbsoluteLoopTopAddress;
  // Set x28 (which is our state register) to point to our guest thread data
  _mcontext->regs[28 /* STATE */] = reinterpret_cast<uint64_t>(State);

  // Ref count our faults
  // We use this to track if it is safe to clear cache
  ++SignalHandlerRefCounter;

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

JITCore::CodeBuffer JITCore::AllocateNewCodeBuffer(size_t Size) {
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

void JITCore::FreeCodeBuffer(CodeBuffer Buffer) {
  munmap(Buffer.Ptr, Buffer.Size);
}

bool JITCore::HandleSIGBUS(int Signal, void *info, void *ucontext) {
  ucontext_t* _context = (ucontext_t*)ucontext;
  mcontext_t* _mcontext = &_context->uc_mcontext;
  uint32_t *PC = (uint32_t*)_mcontext->pc;
  uint32_t Instr = PC[0];

  if (!IsAddressInJITCode(_mcontext->pc)) {
    // Wasn't a sigbus in JIT code
    return false;
  }

  // 1 = 16bit
  // 2 = 32bit
  // 3 = 64bit
  uint32_t Size = (Instr & 0xC000'0000) >> 30;
  uint32_t AddrReg = (Instr >> 5) & 0x1F;
  uint32_t DataReg = Instr & 0x1F;
  uint32_t DMB = 0b1101'0101'0000'0011'0011'0000'1011'1111 |
    0b1011'0000'0000; // Inner shareable all
  if ((Instr & 0x3F'FF'FC'00) == 0x08'DF'FC'00 || // LDAR*
      (Instr & 0x3F'FF'FC'00) == 0x38'BF'C0'00) { // LDAPR*
    uint32_t LDR = 0b0011'1000'0111'1111'0110'1000'0000'0000;
    LDR |= Size << 30;
    LDR |= AddrReg << 5;
    LDR |= DataReg;
    PC[-1] = DMB;
    PC[0] = LDR;
    PC[1] = DMB;
  }
  else if ( (Instr & 0x3F'FF'FC'00) == 0x08'9F'FC'00) { // STLR*
    uint32_t STR = 0b0011'1000'0011'1111'0110'1000'0000'0000;
    STR |= Size << 30;
    STR |= AddrReg << 5;
    STR |= DataReg;
    PC[-1] = DMB;
    PC[0] = STR;
    PC[1] = DMB;

  }
  else {
    LogMan::Msg::E("Unhandled JIT SIGBUS: PC: %p Instruction: 0x%08x\n", PC, PC[0]);
    return false;
  }

  // Back up one instruction and have another go
  _mcontext->pc -= 4;
  vixl::aarch64::CPU::EnsureIAndDCacheCoherency(&PC[-1], 16);
  return true;
}

bool JITCore::HandleSignalPause(int Signal, void *info, void *ucontext) {
  FEXCore::Core::SignalEvent SignalReason = State->SignalReason.load();

  if (SignalReason == FEXCore::Core::SignalEvent::SIGNALEVENT_PAUSE) {
    ucontext_t* _context = (ucontext_t*)ucontext;
    mcontext_t* _mcontext = &_context->uc_mcontext;

    // Store our thread state so we can come back to this
    StoreThreadState(Signal, ucontext);

    // Set the new PC
    _mcontext->pc = ThreadPauseHandlerAddress;

    // Set x28 (which is our state register) to point to our guest thread data
    _mcontext->regs[28 /* STATE */] = reinterpret_cast<uint64_t>(State);

    // Ref count our faults
    // We use this to track if it is safe to clear cache
    ++SignalHandlerRefCounter;

    State->SignalReason.store(FEXCore::Core::SIGNALEVENT_NONE);
    return true;
  }

  if (SignalReason == FEXCore::Core::SignalEvent::SIGNALEVENT_RETURN) {
    RestoreThreadState(ucontext);

    // Ref count our faults
    // We use this to track if it is safe to clear cache
    --SignalHandlerRefCounter;

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

    // Our ref counting doesn't matter anymore
    SignalHandlerRefCounter = 0;

    // Set the new PC
    _mcontext->pc = ThreadStopHandlerAddress;
    State->SignalReason.store(FEXCore::Core::SIGNALEVENT_NONE);
    return true;
  }

  return false;
}

JITCore::JITCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread, CodeBuffer Buffer)
  : vixl::aarch64::Assembler(Buffer.Ptr, Buffer.Size, vixl::aarch64::PositionDependentCode)
  , CTX {ctx}
  , State {Thread}
  , InitialCodeBuffer {Buffer}
{
  CurrentCodeBuffer = &InitialCodeBuffer;
  auto Features = vixl::CPUFeatures::InferFromOS();
  SupportsAtomics = Features.Has(vixl::CPUFeatures::Feature::kAtomics);
  SupportsRCPC = Features.Has(vixl::CPUFeatures::Feature::kRCpc);

  if (SupportsAtomics) {
    // Hypervisor can hide this on the c630?
    Features.Combine(vixl::CPUFeatures::Feature::kLORegions);
  }

  SetCPUFeatures(Features);

  if (!SupportsAtomics) {
    WARN_ONCE("Host CPU doesn't support atomics. Expect bad performance");
  }

  RAPass = Thread->PassManager->GetRAPass();

#if DEBUG
  Decoder.AppendVisitor(&Disasm)
#endif
  CPU.SetUp();
  SetAllowAssembler(true);
  CreateCustomDispatch(Thread);

  uint32_t NumUsedGPRs = NumGPRs;
  uint32_t NumUsedGPRPairs = NumGPRPairs;
  uint32_t UsedRegisterCount = RegisterCount;

  RAPass->AllocateRegisterSet(UsedRegisterCount, RegisterClasses);

  RAPass->AddRegisters(FEXCore::IR::GPRClass, NumUsedGPRs);
  RAPass->AddRegisters(FEXCore::IR::FPRClass, NumFPRs);
  RAPass->AddRegisters(FEXCore::IR::GPRPairClass, NumUsedGPRPairs);

  RAPass->AllocateRegisterConflicts(FEXCore::IR::GPRClass, NumUsedGPRs);
  RAPass->AllocateRegisterConflicts(FEXCore::IR::GPRPairClass, NumUsedGPRs);

  for (uint32_t i = 0; i < NumUsedGPRPairs; ++i) {
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

  // This will register the host signal handler per thread, which is fine
  CTX->SignalDelegation.RegisterHostSignalHandler(SIGILL, [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) -> bool {
    JITCore *Core = reinterpret_cast<JITCore*>(Thread->CPUBackend.get());
    return Core->HandleSIGILL(Signal, info, ucontext);
  });

  CTX->SignalDelegation.RegisterHostSignalHandler(SIGBUS, [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) -> bool {
    JITCore *Core = reinterpret_cast<JITCore*>(Thread->CPUBackend.get());
    return Core->HandleSIGBUS(Signal, info, ucontext);
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

void JITCore::ClearCache() {
  // Get the backing code buffer
  auto Buffer = GetBuffer();
  if (SignalHandlerRefCounter == 0) {
    if (!CodeBuffers.empty()) {
      // If we have more than one code buffer we are tracking then walk them and delete
      // This is a cleanup step
      for (auto CodeBuffer : CodeBuffers) {
        FreeCodeBuffer(CodeBuffer);
      }
      CodeBuffers.clear();

      // Set the current code buffer to the initial
      *Buffer = vixl::CodeBuffer(InitialCodeBuffer.Ptr, InitialCodeBuffer.Size);
      CurrentCodeBuffer = &InitialCodeBuffer;
    }

    if (CurrentCodeBuffer->Size == MAX_CODE_SIZE) {
      // Rewind to the start of the code cache start
      Buffer->Reset();
    }
    else {
      FreeCodeBuffer(InitialCodeBuffer);

      // Resize the code buffer and reallocate our code size
      InitialCodeBuffer.Size *= 1.5;
      InitialCodeBuffer.Size = std::min(InitialCodeBuffer.Size, MAX_CODE_SIZE);

      InitialCodeBuffer = JITCore::AllocateNewCodeBuffer(InitialCodeBuffer.Size);
      *Buffer = vixl::CodeBuffer(InitialCodeBuffer.Ptr, InitialCodeBuffer.Size);
    }
  }
  else {
    // We have signal handlers that have generated code
    // This means that we can not safely clear the code at this point in time
    // Allocate some new code buffers that we can switch over to instead
    auto NewCodeBuffer = JITCore::AllocateNewCodeBuffer(JITCore::INITIAL_CODE_SIZE);
    CurrentCodeBuffer->Size = JITCore::INITIAL_CODE_SIZE;
    EmplaceNewCodeBuffer(NewCodeBuffer);
    *Buffer = vixl::CodeBuffer(NewCodeBuffer.Ptr, NewCodeBuffer.Size);
  }
}

JITCore::~JITCore() {
  FreeCodeBuffer(DispatcherCodeBuffer);
  FreeCodeBuffer(InitialCodeBuffer);
  for (auto &Buffer : CodeBuffers) {
    FreeCodeBuffer(Buffer);
  }
}

void JITCore::LoadConstant(vixl::aarch64::Register Reg, uint64_t Constant) {
  bool Is64Bit = Reg.IsX();
  int Segments = Is64Bit ? 4 : 2;

  if (Is64Bit && ((~Constant)>> 16) == 0) {
    movn(Reg, (~Constant) & 0xFFFF);
    return;
  }

  movz(Reg, (Constant) & 0xFFFF, 0);
  for (int i = 1; i < Segments; ++i) {
    uint16_t Part = (Constant >> (i * 16)) & 0xFFFF;
    if (Part) {
      movk(Reg, Part, i * 16);
    }
  }
}

static uint32_t GetPhys(IR::RegisterAllocationPass *RAPass, uint32_t Node) {
  uint64_t Reg = RAPass->GetNodeRegister(Node);

  if ((uint32_t)Reg != ~0U)
    return Reg;
  else
    LogMan::Msg::A("Couldn't Allocate register for node: ssa%d. Class: %d", Node, Reg >> 32);

  return ~0U;
}

template<>
aarch64::Register JITCore::GetReg<JITCore::RA_32>(uint32_t Node) {
  uint32_t Reg = GetPhys(RAPass, Node);
  return RA64[Reg].W();
}

template<>
aarch64::Register JITCore::GetReg<JITCore::RA_64>(uint32_t Node) {
  uint32_t Reg = GetPhys(RAPass, Node);
  return RA64[Reg];
}

template<>
std::pair<aarch64::Register, aarch64::Register> JITCore::GetSrcPair<JITCore::RA_32>(uint32_t Node) {
  uint32_t Reg = GetPhys(RAPass, Node);
  return RA32Pair[Reg];
}

template<>
std::pair<aarch64::Register, aarch64::Register> JITCore::GetSrcPair<JITCore::RA_64>(uint32_t Node) {
  uint32_t Reg = GetPhys(RAPass, Node);
  return RA64Pair[Reg];
}

aarch64::VRegister JITCore::GetSrc(uint32_t Node) {
  uint32_t Reg = GetPhys(RAPass, Node);
  return RAFPR[Reg];
}

aarch64::VRegister JITCore::GetDst(uint32_t Node) {
  uint32_t Reg = GetPhys(RAPass, Node);
  return RAFPR[Reg];
}

bool JITCore::IsInlineConstant(const IR::OrderedNodeWrapper& WNode, uint64_t* Value) {
  auto OpHeader = IR->GetOp<IR::IROp_Header>(WNode);

  if (OpHeader->Op == IR::IROps::OP_INLINECONSTANT) {
    auto Op = OpHeader->C<IR::IROp_InlineConstant>();
    if (Value) {
      *Value = Op->Constant;
    }
    return true;
  } else {
    return false;
  }
}

void *JITCore::CompileCode([[maybe_unused]] FEXCore::IR::IRListView<true> const *IR, [[maybe_unused]] FEXCore::Core::DebugData *DebugData) {
  using namespace aarch64;
  JumpTargets.clear();
  uint32_t SSACount = IR->GetSSACount();

  auto HeaderOp = IR->GetHeader();
  if (HeaderOp->ShouldInterpret) {
    return reinterpret_cast<void*>(InterpreterFallbackHelperAddress);
  }

  this->IR = IR;

  // Fairly excessive buffer range to make sure we don't overflow
  uint32_t BufferRange = SSACount * 16;
  if ((GetCursorOffset() + BufferRange) > CurrentCodeBuffer->Size) {
    State->CTX->ClearCodeCache(State, HeaderOp->Entry);
  }

  LogMan::Throw::A(RAPass->HasFullRA(), "Arm64 JIT only works with RA");

  SpillSlots = RAPass->SpillSlots();

  // AAPCS64
  // r30      = LR
  // r29      = FP
  // r19..r28 = Callee saved
  // r18      = Platform Register (Matters if we target Windows or iOS)
  // r16..r17 = Inter-procedure scratch
  //  r9..r15 = Temp
  //  r8      = Indirect Result
  //  r0...r7 = Parameter/Results
  //
  //  FPRS:
  //  v8..v15 = (lower 64bits) Callee saved

  // Our allocation:
  // X0 = ThreadState
  // X1 = MemBase
  //
  // X1-X3 = Temp
  // X4-r18 = RA

  auto Buffer = GetBuffer();
  auto Entry = Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset());

  if (SpillSlots) {
    add(TMP1, sp, 0); // Move that supports SP
    sub(sp, sp, SpillSlots * 16);
    stp(TMP1, lr, MemOperand(sp, -16, PreIndex));
  }
  else {
    add(TMP1, sp, 0); // Move that supports SP
    stp(TMP1, lr, MemOperand(sp, -16, PreIndex));
  }

  PendingTargetLabel = nullptr;
  for (auto [BlockNode, BlockHeader] : IR->GetBlocks()) {
    using namespace FEXCore::IR;
    auto BlockIROp = BlockHeader->CW<FEXCore::IR::IROp_CodeBlock>();
    LogMan::Throw::A(BlockIROp->Header.Op == IR::OP_CODEBLOCK, "IR type failed to be a code block");

    {
      uint32_t Node = IR->GetID(BlockNode);
      auto IsTarget = JumpTargets.find(Node);
      if (IsTarget == JumpTargets.end()) {
        IsTarget = JumpTargets.try_emplace(Node).first;
      }

      if (PendingTargetLabel && PendingTargetLabel != &IsTarget->second)
      {
        b(PendingTargetLabel);
      }
      PendingTargetLabel = nullptr;
      bind(&IsTarget->second);
    }

    for (auto [CodeNode, IROp] : IR->GetCode(BlockNode)) {
      uint32_t ID = IR->GetID(CodeNode);

      // Execute handler
      OpHandler Handler = OpHandlers[IROp->Op];
      (this->*Handler)(IROp, ID);
    }
  }

  // Make sure last branch is generated. It certainly can't be eliminated here.
  if (PendingTargetLabel)
  {
    b(PendingTargetLabel);
  }
  PendingTargetLabel = nullptr;

  FinalizeCode();

  auto CodeEnd = Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset());
  CPU.EnsureIAndDCacheCoherency(reinterpret_cast<void*>(Entry), CodeEnd - reinterpret_cast<uint64_t>(Entry));

  if (DebugData) {
    DebugData->HostCodeSize = reinterpret_cast<uintptr_t>(CodeEnd) - reinterpret_cast<uintptr_t>(Entry);
  }

  this->IR = nullptr;

  return reinterpret_cast<void*>(Entry);
}

void JITCore::PushCalleeSavedRegisters() {
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

void JITCore::PopCalleeSavedRegisters() {
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

void JITCore::CreateCustomDispatch(FEXCore::Core::InternalThreadState *Thread) {
  auto OriginalBuffer = *GetBuffer();

  // Dispatcher lives outside of traditional space-time
  DispatcherCodeBuffer = JITCore::AllocateNewCodeBuffer(MAX_DISPATCHER_CODE_SIZE);
  *GetBuffer() = vixl::CodeBuffer(DispatcherCodeBuffer.Ptr, DispatcherCodeBuffer.Size);

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


  uint64_t VirtualMemorySize = Thread->BlockCache->GetVirtualMemorySize();
  Literal l_VirtualMemory {VirtualMemorySize};
  Literal l_PagePtr {Thread->BlockCache->GetPagePointer()};
  Literal l_CTX {reinterpret_cast<uintptr_t>(CTX)};
  Literal l_Interpreter {reinterpret_cast<uint64_t>(State->IntBackend->CompileCode(nullptr, nullptr))};
  Literal l_Sleep {reinterpret_cast<uint64_t>(SleepThread)};

  uintptr_t CompileBlockPtr{};
  uintptr_t CompileFallbackPtr{};
  {
    using ClassPtrType = uintptr_t (FEXCore::Context::Context::*)(FEXCore::Core::InternalThreadState *, uint64_t);
    union PtrCast {
      ClassPtrType ClassPtr;
      uintptr_t Data;
    };

    PtrCast Ptr;
    Ptr.ClassPtr = &FEXCore::Context::Context::CompileBlock;
    CompileBlockPtr = Ptr.Data;
  }
  {
    using ClassPtrType = uintptr_t (FEXCore::Context::Context::*)(FEXCore::Core::InternalThreadState *, uint64_t);
    union PtrCast {
      ClassPtrType ClassPtr;
      uintptr_t Data;
    };

    PtrCast Ptr;
    Ptr.ClassPtr = &FEXCore::Context::Context::CompileFallbackBlock;
    CompileFallbackPtr = Ptr.Data;
  }

  Literal l_CompileBlock {CompileBlockPtr};
  Literal l_CompileFallback {CompileFallbackPtr};

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

  auto Align16B = [&Buffer, this]() {
    uint64_t CurrentOffset = Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset());
    for (uint64_t i = (16 - (CurrentOffset & 0xF)); i != 0; i -= 4) {
      nop();
    }
  };

  // We want to ensure that we are 16 byte aligned at the top of this loop
  Align16B();

  bind(&LoopTop);
  AbsoluteLoopTopAddress = GetLabelAddress<uint64_t>(&LoopTop);

  // This is the block cache lookup routine
  // It matches what is going on it BlockCache.h::FindBlock
  ldr(x0, &l_PagePtr);

  // Load in our RIP
  // Don't modify x2 since it contains our RIP once the block doesn't exist
  ldr(x2, MemOperand(STATE, offsetof(FEXCore::Core::ThreadState, State.rip)));
  auto RipReg = x2;

  // Mask the address by the virtual address size so we can check for aliases
  if (__builtin_popcountl(VirtualMemorySize) == 1) {
    and_(x3, RipReg, Thread->BlockCache->GetVirtualMemorySize() - 1);
  }
  else {
    ldr(x3, &l_VirtualMemory);
    and_(x3, RipReg, x3);
  }

  aarch64::Label NoBlock;
  {
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
    ldr(x0, MemOperand(x0, offsetof(FEXCore::BlockCache::BlockCacheEntry, HostCode)));
    cbz(x0, &NoBlock);

    // If we've made it here then we have a real compiled block
    {
      blr(x0);
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

  aarch64::Label FallbackCore;
  // Need to create the block
  {
    bind(&NoBlock);

    ldr(x0, MemOperand(STATE, offsetof(FEXCore::Core::InternalThreadState, CTX)));
    mov(x1, STATE);
    ldr(x3, &l_CompileBlock);

    // X2 contains our guest RIP
    blr(x3); // { CTX, ThreadState, RIP}

    // X0 now contains either nullptr or block pointer
    cbz(x0, &FallbackCore);
    blr(x0);

    b(&LoopTop);
  }

  // We need to fallback to our fallback core
  {
    bind(&FallbackCore);

    ldr(x0, &l_CTX);
    mov(x1, STATE);
    ldr(x3, &l_CompileFallback);

    // X2 contains our guest RIP
    blr(x3); // {ThreadState, RIP}

    // X0 now contains either nullptr or block pointer
    cbz(x0, &Exit);
    blr(x0);

    b(&LoopTop);
  }

  {
    Label RestoreContextStateHelperLabel{};
    bind(&RestoreContextStateHelperLabel);
    SignalReturnInstruction = Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset());

    // Now to get back to our old location we need to do a fault dance
    // We can't use SIGTRAP here since gdb catches it and never gives it to the application!
    hlt(0);
  }

  {
    Label InterpreterFallback{};
    bind(&InterpreterFallback);
    InterpreterFallbackHelperAddress = Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset());
    mov(x0, STATE);
    ldr(x1, &l_Interpreter);

    blr(x1);
    b(&LoopTop);
  }

  {
    bind(&ThreadPauseHandler);
    ThreadPauseHandlerAddress = Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset());
    // We are pausing, this means the frontend should be waiting for this thread to idle
    // We will have faulted and jumped to this location at this point

    // Call our sleep handler
    ldr(x0, &l_CTX);
    mov(x1, STATE);
    ldr(x2, &l_Sleep);
    blr(x2);

    PauseReturnInstruction = Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset());

    // Fault to start running again
    hlt(0);
  }

  {
    // The expectation here is that a thunked function needs to call back in to the JIT in a reentrant safe way
    // To do this safely we need to do some state tracking and register saving
    //
    // eg:
    // JIT Call->
    //  Thunk->
    //    Thunk callback->
    //
    // The thunk callback needs to execute JIT code and when it returns, it needs to safely return to the thunk rather than JIT space
    // This is handled by pushing a return address trampoline to the stack so when the guest address returns it hits our custom thunk return
    //  - This will safely return us to the thunk
    //
    // On return to the thunk, the thunk can get whatever its return value is from the thread context depending on ABI handling on its end
    // When the thunk itself returns, it'll do its regular return logic there
    // void ReentrantCallback(FEXCore::Core::InternalThreadState *Thread, uint64_t RIP);
    CallbackPtr = Buffer->GetOffsetAddress<CPUBackend::JITCallback>(GetCursorOffset());

    // We expect the thunk to have previously pushed the registers it was using
    PushCalleeSavedRegisters();

    // First thing we need to move the thread state pointer back in to our register
    mov(STATE, x0);

    // Make sure to adjust the refcounter so we don't clear the cache now
    LoadConstant(x0, reinterpret_cast<uint64_t>(&SignalHandlerRefCounter));
    ldr(w2, MemOperand(x0));
    add(w2, w2, 1);
    str(w2, MemOperand(x0));

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

  place(&l_VirtualMemory);
  place(&l_PagePtr);
  place(&l_CTX);
  place(&l_Interpreter);
  place(&l_Sleep);
  place(&l_CompileBlock);
  place(&l_CompileFallback);

  FinalizeCode();
  uint64_t CodeEnd = Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset());
  CPU.EnsureIAndDCacheCoherency(reinterpret_cast<void*>(DispatchPtr), CodeEnd - reinterpret_cast<uint64_t>(DispatchPtr));

  *GetBuffer() = OriginalBuffer;
}

FEXCore::CPU::CPUBackend *CreateJITCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread) {
  return new JITCore(ctx, Thread, JITCore::AllocateNewCodeBuffer(JITCore::INITIAL_CODE_SIZE));
}
}
