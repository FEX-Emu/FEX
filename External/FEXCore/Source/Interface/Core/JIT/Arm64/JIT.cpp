#include "Interface/Context/Context.h"

#include "Interface/Core/JIT/Arm64/JITClass.h"

#include "Interface/HLE/Syscalls.h"

#include "Interface/IR/Passes/RegisterAllocationPass.h"

#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <ucontext.h>

namespace FEXCore::CPU {
using namespace vixl;
using namespace vixl::aarch64;

static uint64_t CompileBlockThunk(FEXCore::Context::Context* CTX, FEXCore::Core::InternalThreadState *Thread, uint64_t RIP) {
  uint64_t Result = CTX->CompileBlock(Thread, RIP);
  return Result;
}

static uint64_t CompileFallbackBlockThunk(FEXCore::Context::Context* CTX, FEXCore::Core::InternalThreadState *Thread, uint64_t RIP) {
  uint64_t Result = CTX->CompileFallbackBlock(Thread, RIP);
  return Result;
}
#if _M_X86_64
void JITCore::ExecuteCustomDispatch(FEXCore::Core::ThreadState *Thread) {
  PrintDisassembler PrintDisasm(stdout);
  PrintDisasm.DisassembleBuffer(vixl::aarch64::Instruction::Cast(DispatchPtr), vixl::aarch64::Instruction::Cast(CustomDispatchEnd));

  Sim.WriteXRegister(0, reinterpret_cast<uint64_t>(Thread));
  Sim.RunFrom(vixl::aarch64::Instruction::Cast(DispatchPtr));
}

static void SimulatorExecution(FEXCore::Core::InternalThreadState *Thread) {
  JITCore *Core = reinterpret_cast<JITCore*>(Thread->CPUBackend.get());
  Core->SimulationExecution(Thread);
}

void JITCore::SimulationExecution(FEXCore::Core::InternalThreadState *Thread) {
  using namespace vixl::aarch64;
  auto SimulatorAddress = HostToGuest[Thread->State.State.rip];
  // PrintDisassembler PrintDisasm(stdout);
  // PrintDisasm.DisassembleBuffer(vixl::aarch64::Instruction::Cast(SimulatorAddress.first), vixl::aarch64::Instruction::Cast(SimulatorAddress.second));

  Sim.WriteXRegister(0, reinterpret_cast<uint64_t>(Thread));
  Sim.RunFrom(vixl::aarch64::Instruction::Cast(SimulatorAddress.first));
}

#endif

void JITCore::Op_Unhandled(FEXCore::IR::IROp_Header *IROp, uint32_t Node) {
  auto Name = FEXCore::IR::GetName(IROp->Op);
  LogMan::Msg::A("Unhandled IR Op: %s", std::string(Name).c_str());
}

void JITCore::Op_NoOp(FEXCore::IR::IROp_Header *IROp, uint32_t Node) {
}

JITCore::JITCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread)
  : vixl::aarch64::MacroAssembler(MAX_CODE_SIZE, vixl::aarch64::PositionDependentCode)
  , CTX {ctx}
  , State {Thread}
#if _M_X86_64
  , Sim {&Decoder}
#endif
{

#if _M_X86_64
  auto Features = vixl::CPUFeatures::All();
  SupportsAtomics = true;
#else
  auto Features = vixl::CPUFeatures::InferFromOS();
  SupportsAtomics = Features.Has(vixl::CPUFeatures::Feature::kAtomics);
  if (SupportsAtomics) {
    // Hypervisor can hide this on the c630?
    Features.Combine(vixl::CPUFeatures::Feature::kLORegions);
  }
#endif

  SetCPUFeatures(Features);

  if (!SupportsAtomics) {
    WARN_ONCE("Host CPU doesn't support atomics. Expect bad performance");
  }

  RAPass = Thread->PassManager->GetRAPass();

  // Just set the entire range as executable
  auto Buffer = GetBuffer();
  mprotect(Buffer->GetOffsetAddress<void*>(0), Buffer->GetCapacity(), PROT_READ | PROT_WRITE | PROT_EXEC);
#if DEBUG
  Decoder.AppendVisitor(&Disasm)
#endif
#if _M_X86_64
  Sim.SetCPUFeatures(vixl::CPUFeatures::All());
#endif
  CPU.SetUp();
  SetAllowAssembler(true);
  CreateCustomDispatch(Thread);
  GenerateDispatchHelpers();

  ConstantCodeCacheOffset = GetCursorOffset();

  uint32_t NumUsedGPRs = NumGPRs;
  uint32_t NumUsedGPRPairs = NumGPRPairs;
  uint32_t UsedRegisterCount = RegisterCount;

  if (!CustomDispatchGenerated) {
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
}

void JITCore::ClearCache() {
  // Get the backing code buffer
  auto Buffer = GetBuffer();

  // Rewind the code buffer's cursor back to just after the helpers
  Buffer->Rewind(ConstantCodeCacheOffset);

  // Make sure to set internal state as clean
  Buffer->SetClean();
}

JITCore::~JITCore() {
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
    return State->IntBackend->CompileCode(IR, DebugData);
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
    sub(sp, sp, SpillSlots * 16);
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
  CPU.EnsureIAndDCacheCoherency(reinterpret_cast<void*>(Entry), Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset()) - reinterpret_cast<uint64_t>(Entry));
#if _M_X86_64
  if (!CustomDispatchGenerated) {
    HostToGuest[State->State.State.rip] = std::make_pair(Entry, CodeEnd);
    return (void*)SimulatorExecution;
  }
#endif

  return reinterpret_cast<void*>(Entry);
}

void JITCore::CreateCustomDispatch(FEXCore::Core::InternalThreadState *Thread) {
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

  aarch64::Label LoopTop;
  bind(&LoopTop);

  // Load in our RIP
  // Don't modify x2 since it contains our RIP once the block doesn't exist
  ldr(x2, MemOperand(STATE, offsetof(FEXCore::Core::ThreadState, State.rip)));
  auto RipReg = x2;
  if (CTX->Config.UnifiedMemory) {
    LoadConstant(x3, reinterpret_cast<uint64_t>(CTX->MemoryMapper.GetMemoryBase()));
    sub(x3, x2, x3);
    RipReg = x3;
  }
  LoadConstant(x0, Thread->BlockCache->GetPagePointer());

  // Offset the address and add to our page pointer
  lsr(x1, RipReg, 12);

  // Load the pointer from the offset
  ldr(x0, MemOperand(x0, x1, Shift::LSL, 3));
  aarch64::Label NoBlock;

  // If page pointer is zero then we have no block
  cbz(x0, &NoBlock);

  // Steal the page offset
  and_(x1, RipReg, 0x0FFF);

  // Now load from that pointer offset by the page offset to get our real block
  ldr(x0, MemOperand(x0, x1, Shift::LSL, 3));
  cbz(x0, &NoBlock);

  // If we've made it here then we have a real compiled block
  {
    blr(x0);
  }

  aarch64::Label ExitCheck;
  bind(&ExitCheck);

  constexpr uint64_t ShouldStopOffset = offsetof(FEXCore::Core::ThreadState, RunningEvents.ShouldStop);
  // If we don't need to stop then keep going
  add(x1, STATE, ShouldStopOffset);
  ldarb(x0, MemOperand(x1));
  cbz(x0, &LoopTop);

  PopCalleeSavedRegisters();

  // Return from the function
  // LR is set to the correct return location now
  ret();

  aarch64::Label FallbackCore;
  // Need to create the block
  {
    bind(&NoBlock);

    LoadConstant(x0, reinterpret_cast<uintptr_t>(CTX));
    mov(x1, STATE);

#if _M_X86_64
    CallRuntime(CompileBlockThunk);
#else
    using ClassPtrType = uintptr_t (FEXCore::Context::Context::*)(FEXCore::Core::InternalThreadState *, uint64_t);
    union PtrCast {
      ClassPtrType ClassPtr;
      uintptr_t Data;
    };

    PtrCast Ptr;
    Ptr.ClassPtr = &FEXCore::Context::Context::CompileBlock;
    LoadConstant(x3, Ptr.Data);

    str(STATE, MemOperand(sp, -16, PreIndex));
    // X2 contains our guest RIP
    blr(x3); // { CTX, ThreadState, RIP}
    ldr(STATE, MemOperand(sp, 16, PostIndex));
#endif
    // X0 now contains either nullptr or block pointer
    cbz(x0, &FallbackCore);
    blr(x0);

    b(&ExitCheck);
  }

  aarch64::Label ExitError;
  // We need to fallback to our fallback core
  {
    bind(&FallbackCore);

#if _M_X86_64
    // XXX: Fallback core doesn't work on x86-64
    // We can't tell the difference between simulator entry points and not
    b(&ExitError);
#else
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

    str(STATE, MemOperand(sp, -16, PreIndex));
    // X2 contains our guest RIP
    blr(x3); // {ThreadState, RIP}
    ldr(STATE, MemOperand(sp, 16, PostIndex));
#endif
    // X0 now contains either nullptr or block pointer
    cbz(x0, &ExitError);
    blr(x0);

    b(&ExitCheck);
  }

  // Exit error
  {
    bind(&ExitError);
    LoadConstant(x0, 1);
    add(x1, STATE, ShouldStopOffset);
    stlrb(x0, MemOperand(x1));
    b(&ExitCheck);
  }

#if _M_X86_64
  CustomDispatchEnd = Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset());
#endif

  FinalizeCode();
  CPU.EnsureIAndDCacheCoherency(reinterpret_cast<void*>(DispatchPtr), Buffer->GetOffsetAddress<uint64_t>(GetCursorOffset()) - reinterpret_cast<uint64_t>(DispatchPtr));
  // Disabling will be useful for debugging ThreadState
  //CustomDispatchGenerated = true;
}


void JITCore::GenerateDispatchHelpers() {
  // Nothing here yet
}

FEXCore::CPU::CPUBackend *CreateJITCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread) {
  return new JITCore(ctx, Thread);
}
}
