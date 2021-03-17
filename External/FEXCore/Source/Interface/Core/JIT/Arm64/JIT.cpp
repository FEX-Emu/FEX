#include "Interface/Context/Context.h"

#include "Interface/Core/ArchHelpers/Arm64.h"
#include "Interface/Core/ArchHelpers/MContext.h"
#include "Interface/Core/Dispatcher/Arm64Dispatcher.h"
#include "Interface/Core/JIT/Arm64/JITClass.h"
#include "Interface/Core/InternalThreadState.h"
#include "Interface/Core/CodeBuffer.h"

#include "Interface/IR/Passes/RegisterAllocationPass.h"

#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Core/UContext.h>
#include "Interface/Core/Interpreter/InterpreterOps.h"

#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

namespace FEXCore::CPU {

void Arm64JITCore::CopyNecessaryDataForCompileThread(CPUBackend *Original) {
  Arm64JITCore *Core = reinterpret_cast<Arm64JITCore*>(Original);
  ThreadSharedData = Core->ThreadSharedData;
}

using namespace vixl;
using namespace vixl::aarch64;

void Arm64JITCore::Op_Unhandled(FEXCore::IR::IROp_Header *IROp, uint32_t Node) {
  FallbackInfo Info;
  if (!InterpreterOps::GetFallbackHandler(IROp, &Info)) {
    auto Name = FEXCore::IR::GetName(IROp->Op);
    LogMan::Msg::A("Unhandled IR Op: %s", std::string(Name).c_str());
  } else {
    switch(Info.ABI) {
      case FABI_VOID_U16:{
        SpillStaticRegs();

        PushDynamicRegsAndLR();

        mov(w0, GetReg<RA_32>(IROp->Args[0].ID()));
        LoadConstant(x1, (uintptr_t)Info.fn);

        blr(x1);

        PopDynamicRegsAndLR();

        FillStaticRegs();
      }
      break;

      case FABI_F80_F32:{
        SpillStaticRegs();

        PushDynamicRegsAndLR();

        fmov(v0.S(), GetSrc(IROp->Args[0].ID()).S()) ;
        LoadConstant(x0, (uintptr_t)Info.fn);

        blr(x0);

        PopDynamicRegsAndLR();

        FillStaticRegs();

        eor(GetDst(Node).V16B(), GetDst(Node).V16B(), GetDst(Node).V16B());
        ins(GetDst(Node).V2D(), 0, x0);
        ins(GetDst(Node).V8H(), 4, w1);
      }
      break;

      case FABI_F80_F64:{
        SpillStaticRegs();

        PushDynamicRegsAndLR();

        mov(v0.D(), GetSrc(IROp->Args[0].ID()).D());
        LoadConstant(x0, (uintptr_t)Info.fn);

        blr(x0);

        PopDynamicRegsAndLR();

        FillStaticRegs();

        eor(GetDst(Node).V16B(), GetDst(Node).V16B(), GetDst(Node).V16B());
        ins(GetDst(Node).V2D(), 0, x0);
        ins(GetDst(Node).V8H(), 4, w1);
      }
      break;

      case FABI_F80_I16:
      case FABI_F80_I32: {
        SpillStaticRegs();

        PushDynamicRegsAndLR();

        mov(w0, GetReg<RA_32>(IROp->Args[0].ID()));
        LoadConstant(x1, (uintptr_t)Info.fn);

        blr(x1);

        PopDynamicRegsAndLR();

        FillStaticRegs();

        eor(GetDst(Node).V16B(), GetDst(Node).V16B(), GetDst(Node).V16B());
        ins(GetDst(Node).V2D(), 0, x0);
        ins(GetDst(Node).V8H(), 4, w1);
      }
      break;

      case FABI_F32_F80:{
        SpillStaticRegs();

        PushDynamicRegsAndLR();

        umov(x0, GetSrc(IROp->Args[0].ID()).V2D(), 0);
        umov(x1, GetSrc(IROp->Args[0].ID()).V2D(), 1);

        LoadConstant(x2, (uintptr_t)Info.fn);

        blr(x2);

        PopDynamicRegsAndLR();

        FillStaticRegs();

        fmov(GetDst(Node).S(), v0.S());
      }
      break;

      case FABI_F64_F80:{
        SpillStaticRegs();

        PushDynamicRegsAndLR();

        umov(x0, GetSrc(IROp->Args[0].ID()).V2D(), 0);
        umov(x1, GetSrc(IROp->Args[0].ID()).V2D(), 1);

        LoadConstant(x2, (uintptr_t)Info.fn);

        blr(x2);

        PopDynamicRegsAndLR();

        FillStaticRegs();

        mov(GetDst(Node).D(), v0.D());
      }
      break;

      case FABI_I16_F80:{
        SpillStaticRegs();

        PushDynamicRegsAndLR();

        umov(x0, GetSrc(IROp->Args[0].ID()).V2D(), 0);
        umov(x1, GetSrc(IROp->Args[0].ID()).V2D(), 1);

        LoadConstant(x2, (uintptr_t)Info.fn);

        blr(x2);

        PopDynamicRegsAndLR();

        FillStaticRegs();

        uxth(GetReg<RA_64>(Node), x0);
      }
      break;
      case FABI_I32_F80:{
        SpillStaticRegs();

        PushDynamicRegsAndLR();

        umov(x0, GetSrc(IROp->Args[0].ID()).V2D(), 0);
        umov(x1, GetSrc(IROp->Args[0].ID()).V2D(), 1);

        LoadConstant(x2, (uintptr_t)Info.fn);

        blr(x2);

        PopDynamicRegsAndLR();

        FillStaticRegs();

        mov(GetReg<RA_32>(Node), w0);
      }
      break;
      case FABI_I64_F80:{
        SpillStaticRegs();

        PushDynamicRegsAndLR();

        umov(x0, GetSrc(IROp->Args[0].ID()).V2D(), 0);
        umov(x1, GetSrc(IROp->Args[0].ID()).V2D(), 1);

        LoadConstant(x2, (uintptr_t)Info.fn);

        blr(x2);

        PopDynamicRegsAndLR();

        FillStaticRegs();

        mov(GetReg<RA_64>(Node), x0);
      }
      break;
      case FABI_I64_F80_F80:{
        SpillStaticRegs();

        PushDynamicRegsAndLR();

        umov(x0, GetSrc(IROp->Args[0].ID()).V2D(), 0);
        umov(x1, GetSrc(IROp->Args[0].ID()).V2D(), 1);

        umov(x2, GetSrc(IROp->Args[1].ID()).V2D(), 0);
        umov(x3, GetSrc(IROp->Args[1].ID()).V2D(), 1);

        LoadConstant(x4, (uintptr_t)Info.fn);

        blr(x4);

        PopDynamicRegsAndLR();

        FillStaticRegs();

        mov(GetReg<RA_64>(Node), x0);
      }
      break;
      case FABI_F80_F80:{
        SpillStaticRegs();

        PushDynamicRegsAndLR();

        umov(x0, GetSrc(IROp->Args[0].ID()).V2D(), 0);
        umov(x1, GetSrc(IROp->Args[0].ID()).V2D(), 1);

        LoadConstant(x2, (uintptr_t)Info.fn);

        blr(x2);

        PopDynamicRegsAndLR();

        FillStaticRegs();

        eor(GetDst(Node).V16B(), GetDst(Node).V16B(), GetDst(Node).V16B());
        ins(GetDst(Node).V2D(), 0, x0);
        ins(GetDst(Node).V8H(), 4, w1);
      }
      break;
      case FABI_F80_F80_F80:{
        SpillStaticRegs();

        PushDynamicRegsAndLR();

        umov(x0, GetSrc(IROp->Args[0].ID()).V2D(), 0);
        umov(x1, GetSrc(IROp->Args[0].ID()).V2D(), 1);

        umov(x2, GetSrc(IROp->Args[1].ID()).V2D(), 0);
        umov(x3, GetSrc(IROp->Args[1].ID()).V2D(), 1);

        LoadConstant(x4, (uintptr_t)Info.fn);

        blr(x4);

        PopDynamicRegsAndLR();

        FillStaticRegs();

        eor(GetDst(Node).V16B(), GetDst(Node).V16B(), GetDst(Node).V16B());
        ins(GetDst(Node).V2D(), 0, x0);
        ins(GetDst(Node).V8H(), 4, w1);
      }
      break;

      case FABI_UNKNOWN:
      default:
      auto Name = FEXCore::IR::GetName(IROp->Op);
        LogMan::Msg::A("Unhandled IR Fallback abi: %s %d", std::string(Name).c_str(), Info.ABI);
    }
  }
}

void Arm64JITCore::Op_NoOp(FEXCore::IR::IROp_Header *IROp, uint32_t Node) {
}

bool Arm64JITCore::HandleSIGBUS(int Signal, void *info, void *ucontext) {

  uint32_t *PC = (uint32_t*)ArchHelpers::Context::GetPc(ucontext);
  uint32_t Instr = PC[0];

  if (!Dispatcher->IsAddressInJITCode(ArchHelpers::Context::GetPc(ucontext))) {
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
    // Back up one instruction and have another go
    ArchHelpers::Context::SetPc(ucontext, ArchHelpers::Context::GetPc(ucontext) - 4);
  }
  else if ( (Instr & 0x3F'FF'FC'00) == 0x08'9F'FC'00) { // STLR*
    uint32_t STR = 0b0011'1000'0011'1111'0110'1000'0000'0000;
    STR |= Size << 30;
    STR |= AddrReg << 5;
    STR |= DataReg;
    PC[-1] = DMB;
    PC[0] = STR;
    PC[1] = DMB;
    // Back up one instruction and have another go
    ArchHelpers::Context::SetPc(ucontext, ArchHelpers::Context::GetPc(ucontext) - 4);
  }
  else if ((Instr & FEXCore::ArchHelpers::Arm64::CASPAL_MASK) == FEXCore::ArchHelpers::Arm64::CASPAL_INST) { // CASPAL
    if (FEXCore::ArchHelpers::Arm64::HandleCASPAL(ucontext, info, Instr)) {
      // Skip this instruction now
      ArchHelpers::Context::SetPc(ucontext, ArchHelpers::Context::GetPc(ucontext) + 4);
      return true;
    }
    else {
      LogMan::Msg::E("Unhandled JIT SIGBUS CASPAL: PC: %p Instruction: 0x%08x\n", PC, PC[0]);
      return false;
    }
  }
  else if ((Instr & FEXCore::ArchHelpers::Arm64::CASAL_MASK) == FEXCore::ArchHelpers::Arm64::CASAL_INST) { // CASAL
    if (FEXCore::ArchHelpers::Arm64::HandleCASAL(ucontext, info, Instr)) {
      // Skip this instruction now
      ArchHelpers::Context::SetPc(ucontext, ArchHelpers::Context::GetPc(ucontext) + 4);
      return true;
    }
    else {
      LogMan::Msg::E("Unhandled JIT SIGBUS CASAL: PC: %p Instruction: 0x%08x\n", PC, PC[0]);
      return false;
    }
  }
  else if ((Instr & FEXCore::ArchHelpers::Arm64::ATOMIC_MEM_MASK) == FEXCore::ArchHelpers::Arm64::ATOMIC_MEM_INST) { // Atomic memory op
    if (FEXCore::ArchHelpers::Arm64::HandleAtomicMemOp(ucontext, info, Instr)) {
      // Skip this instruction now
      ArchHelpers::Context::SetPc(ucontext, ArchHelpers::Context::GetPc(ucontext) + 4);
      return true;
    }
    else {
      uint8_t Op = (PC[0] >> 12) & 0xF;
      LogMan::Msg::E("Unhandled JIT SIGBUS Atomic mem op 0x%02x: PC: %p Instruction: 0x%08x\n", Op, PC, PC[0]);
      return false;
    }
  }
  else {
    LogMan::Msg::E("Unhandled JIT SIGBUS: PC: %p Instruction: 0x%08x\n", PC, PC[0]);
    return false;
  }

  vixl::aarch64::CPU::EnsureIAndDCacheCoherency(&PC[-1], 16);
  return true;
}

Arm64JITCore::Arm64JITCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread, bool CompileThread)
  : CTX {ctx}
  , ThreadState {Thread}
  , Arm64Emitter(0) {

  SetCodeBuffer(InitialCodeBuffer);
  SetAllowAssembler(true);

  RAPass = Thread->PassManager->GetRAPass();

#if DEBUG
  Decoder.AppendVisitor(&Disasm)
#endif

  uint32_t NumUsedGPRs = NumGPRs;
  uint32_t NumUsedGPRPairs = NumGPRPairs;
  uint32_t UsedRegisterCount = RegisterCount;

  RAPass->AllocateRegisterSet(UsedRegisterCount, RegisterClasses);

  RAPass->AddRegisters(FEXCore::IR::GPRClass, NumUsedGPRs);
  RAPass->AddRegisters(FEXCore::IR::GPRFixedClass, SRA64.size());
  RAPass->AddRegisters(FEXCore::IR::FPRClass, NumFPRs);
  RAPass->AddRegisters(FEXCore::IR::FPRFixedClass, SRAFPR.size()  );
  RAPass->AddRegisters(FEXCore::IR::GPRPairClass, NumUsedGPRPairs);
  RAPass->AddRegisters(FEXCore::IR::ComplexClass, 1);

  for (uint32_t i = 0; i < NumUsedGPRPairs; ++i) {
    RAPass->AddRegisterConflict(FEXCore::IR::GPRClass, i * 2,     FEXCore::IR::GPRPairClass, i);
    RAPass->AddRegisterConflict(FEXCore::IR::GPRClass, i * 2 + 1, FEXCore::IR::GPRPairClass, i);
  }

  for (uint32_t i = 0; i < FEXCore::IR::IROps::OP_LAST + 1; ++i) {
    OpHandlers[i] = &Arm64JITCore::Op_Unhandled;
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

  if (!CompileThread) {
    DispatcherConfig config;
    config.ExitFunctionLink = reinterpret_cast<uintptr_t>(&ExitFunctionLink);
    config.ExitFunctionLinkThis = reinterpret_cast<uintptr_t>(this);
    config.StaticRegisterAssignment = true;

    Dispatcher = new Arm64Dispatcher(CTX, ThreadState, config);
    DispatchPtr = Dispatcher->DispatchPtr;
    CallbackPtr = Dispatcher->CallbackPtr;

    ThreadSharedData.SignalHandlerRefCounterPtr = &Dispatcher->SignalHandlerRefCounter;
    ThreadSharedData.SignalReturnInstruction = Dispatcher->SignalHandlerReturnAddress;

    Dispatcher->RegisterCodeBuffer(InitialCodeBuffer);

    // This will register the host signal handler per thread, which is fine
    CTX->SignalDelegation->RegisterHostSignalHandler(SIGILL, [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) -> bool {
      Arm64JITCore *Core = reinterpret_cast<Arm64JITCore*>(Thread->CPUBackend.get());
      return Core->Dispatcher->HandleSIGILL(Signal, info, ucontext);
    });

    CTX->SignalDelegation->RegisterHostSignalHandler(SIGBUS, [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) -> bool {
      Arm64JITCore *Core = reinterpret_cast<Arm64JITCore*>(Thread->CPUBackend.get());
      return Core->HandleSIGBUS(Signal, info, ucontext);
    });

    CTX->SignalDelegation->RegisterHostSignalHandler(SignalDelegator::SIGNAL_FOR_PAUSE, [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) -> bool {
      Arm64JITCore *Core = reinterpret_cast<Arm64JITCore*>(Thread->CPUBackend.get());
      return Core->Dispatcher->HandleSignalPause(Signal, info, ucontext);
    });

    auto GuestSignalHandler = [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext, GuestSigAction *GuestAction, stack_t *GuestStack) -> bool {
      Arm64JITCore *Core = reinterpret_cast<Arm64JITCore*>(Thread->CPUBackend.get());
      return Core->Dispatcher->HandleGuestSignal(Signal, info, ucontext, GuestAction, GuestStack);
    };

    for (uint32_t Signal = 0; Signal < SignalDelegator::MAX_SIGNALS; ++Signal) {
      CTX->SignalDelegation->RegisterHostSignalHandlerForGuest(Signal, GuestSignalHandler);
    }
  }
}

void Arm64JITCore::ClearCache() {
  if (*ThreadSharedData.SignalHandlerRefCounterPtr == 0) {
    if (!CodeBuffers.empty()) {
      for (auto &Buffer : CodeBuffers) {
        Dispatcher->ForgetCodeBuffer(Buffer);
      }
      CodeBuffers.clear();

      // Set the current code buffer to the initial
      SetCodeBuffer(InitialCodeBuffer);
    }

    if (CurrentCodeBuffer->Size == MAX_CODE_SIZE) {
      // Rewind to the start of the code cache start
      GetBuffer()->Reset();
    }
    else {
      Dispatcher->ForgetCodeBuffer(InitialCodeBuffer);

      // Resize the code buffer and reallocate our code size
      size_t NewSize = std::min<size_t>(InitialCodeBuffer.Size * 1.5, MAX_CODE_SIZE);
      InitialCodeBuffer = CodeBuffer(NewSize);

      Dispatcher->RegisterCodeBuffer(InitialCodeBuffer);

      SetCodeBuffer(InitialCodeBuffer);
    }
  }
  else {
    // We have signal handlers that have generated code
    // This means that we can not safely clear the code at this point in time
    // Allocate some new code buffers that we can switch over to instead
    auto &NewCodeBuffer = CodeBuffers.emplace_back(INITIAL_CODE_SIZE);
    Dispatcher->RegisterCodeBuffer(NewCodeBuffer);
    SetCodeBuffer(NewCodeBuffer);
  }
}

Arm64JITCore::~Arm64JITCore() {
  if (Dispatcher)
    delete Dispatcher;
}

static IR::PhysicalRegister GetPhys(IR::RegisterAllocationData *RAData, uint32_t Node) {
  auto PhyReg = RAData->GetNodeRegister(Node);

  LogMan::Throw::A(!PhyReg.IsInvalid(), "Couldn't Allocate register for node: ssa%d. Class: %d", Node, PhyReg.Class);

  return PhyReg;
}

template<>
aarch64::Register Arm64JITCore::GetReg<Arm64JITCore::RA_32>(uint32_t Node) {
auto Reg = GetPhys(RAData, Node);
  if (Reg.Class == IR::GPRFixedClass.Val) {
    return SRA64[Reg.Reg].W();
  } else if (Reg.Class == IR::GPRClass.Val) {
    return RA64[Reg.Reg].W();
  } else {
    LogMan::Throw::A(false, "Unexpected Class: %d", Reg.Class);
  }
}

template<>
aarch64::Register Arm64JITCore::GetReg<Arm64JITCore::RA_64>(uint32_t Node) {
  auto Reg = GetPhys(RAData, Node);
  if (Reg.Class == IR::GPRFixedClass.Val) {
    return SRA64[Reg.Reg];
  } else if (Reg.Class == IR::GPRClass.Val) {
    return RA64[Reg.Reg];
  } else {
    LogMan::Throw::A(false, "Unexpected Class: %d", Reg.Class);
  }
}

template<>
std::pair<aarch64::Register, aarch64::Register> Arm64JITCore::GetSrcPair<Arm64JITCore::RA_32>(uint32_t Node) {
  uint32_t Reg = GetPhys(RAData, Node).Reg;
  return RA32Pair[Reg];
}

template<>
std::pair<aarch64::Register, aarch64::Register> Arm64JITCore::GetSrcPair<Arm64JITCore::RA_64>(uint32_t Node) {
  uint32_t Reg = GetPhys(RAData, Node).Reg;
  return RA64Pair[Reg];
}

aarch64::VRegister Arm64JITCore::GetSrc(uint32_t Node) {
  auto Reg = GetPhys(RAData, Node);
  if (Reg.Class == IR::FPRFixedClass.Val) {
    return SRAFPR[Reg.Reg];
  } else if (Reg.Class == IR::FPRClass.Val) {
    return RAFPR[Reg.Reg];
  } else {
    LogMan::Throw::A(false, "Unexpected Class: %d", Reg.Class);
  }
}

aarch64::VRegister Arm64JITCore::GetDst(uint32_t Node) {
  auto Reg = GetPhys(RAData, Node);
  if (Reg.Class == IR::FPRFixedClass.Val) {
    return SRAFPR[Reg.Reg];
  } else if (Reg.Class == IR::FPRClass.Val) {
    return RAFPR[Reg.Reg];
  } else {
    LogMan::Throw::A(false, "Unexpected Class: %d", Reg.Class);
  }
}

bool Arm64JITCore::IsInlineConstant(const IR::OrderedNodeWrapper& WNode, uint64_t* Value) {
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

bool Arm64JITCore::IsInlineEntrypointOffset(const IR::OrderedNodeWrapper& WNode, uint64_t* Value) {
  auto OpHeader = IR->GetOp<IR::IROp_Header>(WNode);

  if (OpHeader->Op == IR::IROps::OP_INLINEENTRYPOINTOFFSET) {
    auto Op = OpHeader->C<IR::IROp_InlineEntrypointOffset>();
    if (Value) {
      *Value = IR->GetHeader()->Entry + Op->Offset;
    }
    return true;
  } else {
    return false;
  }
}

FEXCore::IR::RegisterClassType Arm64JITCore::GetRegClass(uint32_t Node) {
  return FEXCore::IR::RegisterClassType {GetPhys(RAData, Node).Class};
}


bool Arm64JITCore::IsFPR(uint32_t Node) {
  auto Class = GetRegClass(Node);

  return Class == IR::FPRClass || Class == IR::FPRFixedClass;
}

bool Arm64JITCore::IsGPR(uint32_t Node) {
  auto Class = GetRegClass(Node);

  return Class == IR::GPRClass || Class == IR::GPRFixedClass;
}

void *Arm64JITCore::CompileCode([[maybe_unused]] FEXCore::IR::IRListView const *IR, [[maybe_unused]] FEXCore::Core::DebugData *DebugData, FEXCore::IR::RegisterAllocationData *RAData) {
  using namespace aarch64;
  JumpTargets.clear();
  uint32_t SSACount = IR->GetSSACount();

  this->RAData = RAData;

  auto HeaderOp = IR->GetHeader();

  #ifndef NDEBUG
  LoadConstant(x0, HeaderOp->Entry);
  #endif

  this->IR = IR;

  // Fairly excessive buffer range to make sure we don't overflow
  uint32_t BufferRange = SSACount * 16;
  if ((GetCursorOffset() + BufferRange) > CurrentCodeBuffer->Size) {
    ThreadState->CTX->ClearCodeCache(ThreadState, false);
  }

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

 if (CTX->GetGdbServerStatus()) {
    aarch64::Label RunBlock;

    // If we have a gdb server running then run in a less efficient mode that checks if we need to exit
    // This happens when single stepping

    static_assert(sizeof(CTX->Config.RunningMode) == 4, "This is expected to be size of 4");
    ldr(x0, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, Thread))); // Get thread
    ldr(x0, MemOperand(x0, offsetof(FEXCore::Core::InternalThreadState, CTX))); // Get Context
    ldr(w0, MemOperand(x0, offsetof(FEXCore::Context::Context, Config.RunningMode)));

    // If the value == 0 then we don't need to stop
    cbz(w0, &RunBlock);
    {
      // Make sure RIP is syncronized to the context
      LoadConstant(x0, HeaderOp->Entry);
      str(x0, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.rip)));

      // Stop the thread
      LoadConstant(x0, Dispatcher->ThreadPauseHandlerAddressSpillSRA);
      br(x0);
    }
    bind(&RunBlock);
  }

  //LogMan::Throw::A(RAData->HasFullRA(), "Arm64 JIT only works with RA");

  SpillSlots = RAData->SpillSlots();

  if (SpillSlots) {
    if (IsImmAddSub(SpillSlots * 16)) {
      sub(sp, sp, SpillSlots * 16);
    } else {
      LoadConstant(x0, SpillSlots * 16);
      sub(sp, sp, x0);
    }
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

      // if there's a pending branch, and it is not fall-through
      if (PendingTargetLabel && PendingTargetLabel != &IsTarget->second)
      {
        b(PendingTargetLabel);
      }
      PendingTargetLabel = nullptr;

      bind(&IsTarget->second);
    }

    if (DebugData) {
      DebugData->Subblocks.push_back({Buffer->GetOffsetAddress<uintptr_t>(GetCursorOffset()), 0, IR->GetID(BlockNode)});
    }

    for (auto [CodeNode, IROp] : IR->GetCode(BlockNode)) {
      uint32_t ID = IR->GetID(CodeNode);

      // Execute handler
      OpHandler Handler = OpHandlers[IROp->Op];
      (this->*Handler)(IROp, ID);
    }

    if (DebugData) {
      DebugData->Subblocks.back().HostCodeSize = Buffer->GetOffsetAddress<uintptr_t>(GetCursorOffset()) - DebugData->Subblocks.back().HostCodeStart;
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

uint64_t Arm64JITCore::ExitFunctionLink(Arm64JITCore *core, FEXCore::Core::CpuStateFrame *Frame, uint64_t *record) {
  auto Thread = Frame->Thread;
  auto GuestRip = record[1];

  auto HostCode = Thread->LookupCache->FindBlock(GuestRip);

  if (!HostCode) {
    //printf("ExitFunctionLink: Aborting, %lX not in cache\n", GuestRip);
    Frame->State.rip = GuestRip;
    return core->Dispatcher->AbsoluteLoopTopAddress;
  }

  uintptr_t branch = (uintptr_t)(record) - 8;
  auto LinkerAddress = core->Dispatcher->ExitFunctionLinkerAddress;

  auto offset = HostCode/4 - branch/4;
  if (IsInt26(offset)) {
    // optimal case - can branch directly
    // patch the code
    vixl::aarch64::Assembler emit((uint8_t*)(branch), 24);
    vixl::CodeBufferCheckScope scope(&emit, 24, vixl::CodeBufferCheckScope::kDontReserveBufferSpace, vixl::CodeBufferCheckScope::kNoAssert);
    emit.b(offset);
    emit.FinalizeCode();
    vixl::aarch64::CPU::EnsureIAndDCacheCoherency((void*)branch, 24);

    // Add de-linking handler
    Thread->LookupCache->AddBlockLink(GuestRip, (uintptr_t)record, [branch, LinkerAddress]{
      vixl::aarch64::Assembler emit((uint8_t*)(branch), 24);
      vixl::CodeBufferCheckScope scope(&emit, 24, vixl::CodeBufferCheckScope::kDontReserveBufferSpace, vixl::CodeBufferCheckScope::kNoAssert);
      Literal l_BranchHost{LinkerAddress};
      emit.ldr(x0, &l_BranchHost);
      emit.blr(x0);
      emit.place(&l_BranchHost);
      emit.FinalizeCode();
      vixl::aarch64::CPU::EnsureIAndDCacheCoherency((void*)branch, 24);
    });
  } else {
    // fallback case - do a soft-er link by patching the pointer
    record[0] = HostCode;

    // Add de-linking handler
    Thread->LookupCache->AddBlockLink(GuestRip, (uintptr_t)record, [record, LinkerAddress]{
      record[0] = LinkerAddress;
    });
  }

  return HostCode;
}

FEXCore::CPU::CPUBackend *CreateArm64JITCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread, bool CompileThread) {
  return new Arm64JITCore(ctx, Thread, CompileThread);
}
}
