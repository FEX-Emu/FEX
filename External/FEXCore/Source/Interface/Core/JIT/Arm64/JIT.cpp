#include "Interface/Context/Context.h"

#include "Interface/Core/JIT/Arm64/JITClass.h"

#include "Interface/HLE/Syscalls.h"

#include "Interface/IR/Passes/RegisterAllocationPass.h"

#include <FEXCore/Core/X86Enums.h>

#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <ucontext.h>

namespace FEXCore::CPU {
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
  SignalDelegator::GuestSigAction *GuestAction;
  FEXCore::Core::CPUState GuestState;
};

bool JITCore::HandleSIGILL(int Signal, void *info, void *ucontext) {
  ucontext_t* _context = (ucontext_t*)ucontext;
  mcontext_t* _mcontext = &_context->uc_mcontext;

  if (_mcontext->pc == SignalReturnInstruction) {
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
  Context->GuestAction = GuestAction;

  // Save guest state
  // We can't guarantee if registers are in context or host GPRs
  // So we need to save everything
  memcpy(&Context->GuestState, &State->State, sizeof(FEXCore::Core::CPUState));

  // Set the new SP
  _mcontext->sp = NewSP;
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

  if (!!GuestStack->ss_sp) {
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

JITCore::JITCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread, CodeBuffer Buffer)
  : vixl::aarch64::MacroAssembler(Buffer.Ptr, Buffer.Size, vixl::aarch64::PositionDependentCode)
  , CTX {ctx}
  , State {Thread}
  , InitialCodeBuffer {Buffer}
{
  auto Features = vixl::CPUFeatures::InferFromOS();
  SupportsAtomics = Features.Has(vixl::CPUFeatures::Feature::kAtomics);

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

  if (!CustomDispatchGenerated) {
    ERROR_AND_DIE("THIS IS A CODE PATH THAT WILL BREAK! We now REQUIRE custom dispatch!");
    // If we aren't using our custom dispatcher then cut out the callee saved registers
    NumUsedGPRs -= NumCalleeGPRs;
    NumUsedGPRPairs -= NumCalleeGPRPairs;
    UsedRegisterCount -= NumCalleeGPRs + NumCalleeGPRPairs;
  }

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

  // This will register the host signal handler per thread, which is fine
  CTX->SignalDelegation.RegisterHostSignalHandler(SIGILL, [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) -> bool {
    JITCore *Core = reinterpret_cast<JITCore*>(Thread->CPUBackend.get());
    return Core->HandleSIGILL(Signal, info, ucontext);
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
      // Resize the code buffer and reallocate our code size
      CurrentCodeBuffer->Size *= 1.5;
      CurrentCodeBuffer->Size = std::min(CurrentCodeBuffer->Size, MAX_CODE_SIZE);

      FreeCodeBuffer(InitialCodeBuffer);
      InitialCodeBuffer = JITCore::AllocateNewCodeBuffer(CurrentCodeBuffer->Size);
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
}

void JITCore::LoadConstant(vixl::aarch64::Register Reg, uint64_t Constant) {
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

void *JITCore::CompileCode([[maybe_unused]] FEXCore::IR::IRListView<true> const *IR, [[maybe_unused]] FEXCore::Core::DebugData *DebugData) {
  using namespace aarch64;
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
    return reinterpret_cast<void*>(InterpreterFallbackHelperAddress);
  }

  // Fairly excessive buffer range to make sure we don't overflow
  uint32_t BufferRange = SSACount * 16;
  if ((GetCursorOffset() + BufferRange) > MAX_CODE_SIZE) {
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

  if (!CustomDispatchGenerated) {
    mov(STATE, x0);
  }

  if (SpillSlots) {
    add(TMP1, sp, 0); // Move that supports SP
    sub(sp, sp, SpillSlots * 16);
    stp(TMP1, lr, MemOperand(sp, -16, PreIndex));
  }
  else {
    add(TMP1, sp, 0); // Move that supports SP
    stp(TMP1, lr, MemOperand(sp, -16, PreIndex));
  }

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

      bind(&IsTarget->second);
    }

    while (1) {
      OrderedNodeWrapper *WrapperOp = CodeBegin();
      FEXCore::IR::IROp_Header *IROp = WrapperOp->GetNode(ListBegin)->Op(DataBegin);
      uint32_t Node = WrapperOp->ID();

      // Execute handler
      OpHandler Handler = OpHandlers[IROp->Op];
      (this->*Handler)(IROp, Node);

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

  FinalizeCode();

  auto CodeEnd = Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset());
  CPU.EnsureIAndDCacheCoherency(reinterpret_cast<void*>(Entry), CodeEnd - reinterpret_cast<uint64_t>(Entry));

  return reinterpret_cast<void*>(Entry);
}

void JITCore::CreateCustomDispatch(FEXCore::Core::InternalThreadState *Thread) {
  auto OriginalBuffer = *GetBuffer();

  // Dispatcher lives outside of traditional space-time
  DispatcherCodeBuffer = JITCore::AllocateNewCodeBuffer(MAX_DISPATCHER_CODE_SIZE);
  *GetBuffer() = vixl::CodeBuffer(DispatcherCodeBuffer.Ptr, DispatcherCodeBuffer.Size);

  auto Buffer = GetBuffer();

  DispatchPtr = Buffer->GetOffsetAddress<CustomDispatch>(GetCursorOffset());
  EmissionCheckScope(this, 0);

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

  aarch64::Label LoopTop{};
  bind(&LoopTop);
  AbsoluteLoopTopAddress = GetLabelAddress<uint64_t>(&LoopTop);

  // Load in our RIP
  // Don't modify x2 since it contains our RIP once the block doesn't exist
  ldr(x2, MemOperand(STATE, offsetof(FEXCore::Core::ThreadState, State.rip)));
  auto RipReg = x2;

  // Mask the address by the virtual address size so we can check for aliases
  LoadConstant(x3, Thread->BlockCache->GetVirtualMemorySize() - 1);
  and_(x3, RipReg, x3);

  aarch64::Label NoBlock;
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
    ldr(x0, MemOperand(x0, offsetof(FEXCore::BlockCache::BlockCacheEntry, HostCode)));
    cbz(x0, &NoBlock);

    // If we've made it here then we have a real compiled block
    {
      blr(x0);
    }
  }

  aarch64::Label ExitCheck;
  {
    bind(&ExitCheck);

    // If we don't need to stop then keep going
    add(x1, STATE, offsetof(FEXCore::Core::ThreadState, RunningEvents.ShouldStop));
    ldarb(x0, MemOperand(x1));
    cbz(x0, &LoopTop);

    PopCalleeSavedRegisters();

    // Return from the function
    // LR is set to the correct return location now
    ret();
  }

  aarch64::Label FallbackCore;
  // Need to create the block
  {
    bind(&NoBlock);

    LoadConstant(x0, reinterpret_cast<uintptr_t>(CTX));
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

    // X0 now contains either nullptr or block pointer
    cbz(x0, &FallbackCore);
    blr(x0);

    b(&ExitCheck);
  }

  aarch64::Label ExitError;
  // We need to fallback to our fallback core
  {
    bind(&FallbackCore);

    LoadConstant(x0, reinterpret_cast<uintptr_t>(CTX));
    mov(x1, STATE);

    using ClassPtrType = uintptr_t (FEXCore::Context::Context::*)(FEXCore::Core::InternalThreadState *, uint64_t);
    union PtrCast {
      ClassPtrType ClassPtr;
      uintptr_t Data;
    };

    PtrCast Ptr;
    Ptr.ClassPtr = &FEXCore::Context::Context::CompileFallbackBlock;
    LoadConstant(x3, Ptr.Data);

    // X2 contains our guest RIP
    blr(x3); // {ThreadState, RIP}

    // X0 now contains either nullptr or block pointer
    cbz(x0, &ExitError);
    blr(x0);

    b(&ExitCheck);
  }

  // Exit error
  {
    bind(&ExitError);
    LoadConstant(x0, 1);
    add(x1, STATE, offsetof(FEXCore::Core::ThreadState, RunningEvents.ShouldStop));
    stlrb(x0, MemOperand(x1));
    b(&ExitCheck);
  }

  FinalizeCode();
  CPU.EnsureIAndDCacheCoherency(reinterpret_cast<void*>(DispatchPtr), Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset()) - reinterpret_cast<uint64_t>(DispatchPtr));

  // Disabling will be useful for debugging ThreadState
  CustomDispatchGenerated = true;
  GenerateDispatchHelpers();
  *GetBuffer() = OriginalBuffer;
}

void JITCore::GenerateDispatchHelpers() {
  auto Buffer = GetBuffer();
  auto HelperStart = Buffer->GetOffsetAddress<void*>(GetCursorOffset());

  {
    Label RestoreContextStateHelperLabel{};
    Bind(&RestoreContextStateHelperLabel);
    SignalReturnInstruction = Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset());

    // Now to get back to our old location we need to do a fault dance
    // We can't use SIGTRAP here since gdb catches it and never gives it to the application!
    hlt(0);
  }

  {
    Label InterpreterFallback{};
    Bind(&InterpreterFallback);
    InterpreterFallbackHelperAddress = Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset());
    mov(x0, STATE);
    LoadConstant(x1, reinterpret_cast<uint64_t>(State->IntBackend->CompileCode(nullptr, nullptr)));
    // This is a tail-call optimized call
    // We will return to the dispatcher at this point
    br(x1);
  }

  auto HelperEnd = Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset());
  FinalizeCode();
  CPU.EnsureIAndDCacheCoherency(HelperStart, HelperEnd - reinterpret_cast<uint64_t>(HelperStart));
}

FEXCore::CPU::CPUBackend *CreateJITCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread) {
  return new JITCore(ctx, Thread, JITCore::AllocateNewCodeBuffer(JITCore::INITIAL_CODE_SIZE));
}
}
