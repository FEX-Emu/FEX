/*
$info$
glossary: Splatter ~ a code generator backend that concaternates configurable macros instead of doing isel
glossary: IR ~ Intermediate Representation, our high-level opcode representation, loosely modeling arm64
glossary: SSA ~ Single Static Assignment, a form of representing IR in memory
glossary: Basic Block ~ A block of instructions with no control flow, terminated by control flow
glossary: Fragment ~ A Collection of basic blocks, possibly an entire guest function or a subset of it
tags: backend|arm64
desc: Main glue logic of the arm64 splatter backend
$end_info$
*/

#include "Interface/Context/Context.h"
#include "Interface/Core/LookupCache.h"

#include "Interface/Core/ArchHelpers/Arm64.h"
#include "Interface/Core/ArchHelpers/MContext.h"
#include "Interface/Core/Dispatcher/Arm64Dispatcher.h"
#include "Interface/Core/JIT/Arm64/JITClass.h"
#include "Interface/Core/InternalThreadState.h"

#include "Interface/IR/Passes/RegisterAllocationPass.h"

#include "Utils/MemberFunctionToPointer.h"

#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Core/UContext.h>
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/CompilerDefs.h>

#include "Interface/Core/Interpreter/InterpreterOps.h"

#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

namespace {
static uint64_t LUDIV(uint64_t SrcHigh, uint64_t SrcLow, uint64_t Divisor) {
  __uint128_t Source = (static_cast<__uint128_t>(SrcHigh) << 64) | SrcLow;
  __uint128_t Res = Source / Divisor;
  return Res;
}

static int64_t LDIV(int64_t SrcHigh, int64_t SrcLow, int64_t Divisor) {
  __int128_t Source = (static_cast<__int128_t>(SrcHigh) << 64) | SrcLow;
  __int128_t Res = Source / Divisor;
  return Res;
}

static uint64_t LUREM(uint64_t SrcHigh, uint64_t SrcLow, uint64_t Divisor) {
  __uint128_t Source = (static_cast<__uint128_t>(SrcHigh) << 64) | SrcLow;
  __uint128_t Res = Source % Divisor;
  return Res;
}

static int64_t LREM(int64_t SrcHigh, int64_t SrcLow, int64_t Divisor) {
  __int128_t Source = (static_cast<__int128_t>(SrcHigh) << 64) | SrcLow;
  __int128_t Res = Source % Divisor;
  return Res;
}

static void PrintValue(uint64_t Value) {
  LogMan::Msg::DFmt("Value: 0x{:x}", Value);
}

static void PrintVectorValue(uint64_t Value, uint64_t ValueUpper) {
  LogMan::Msg::DFmt("Value: 0x{:016x}'{:016x}", ValueUpper, Value);
}
}

namespace FEXCore::CPU {

using namespace vixl;
using namespace vixl::aarch64;

void Arm64JITCore::Op_Unhandled(IR::IROp_Header *IROp, IR::NodeID Node) {
  FallbackInfo Info;
  if (!InterpreterOps::GetFallbackHandler(IROp, &Info)) {
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
    LOGMAN_MSG_A_FMT("Unhandled IR Op: {}", FEXCore::IR::GetName(IROp->Op));
#endif
  } else {
    switch(Info.ABI) {
      case FABI_VOID_U16:{
        SpillStaticRegs();

        PushDynamicRegsAndLR();

        uxth(w0, GetReg<RA_32>(IROp->Args[0].ID()));
        ldr(x1, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.AArch64.FallbackHandlerPointers[Info.HandlerIndex])));
        blr(x1);

        PopDynamicRegsAndLR();

        FillStaticRegs();
      }
      break;

      case FABI_F80_F32:{
        SpillStaticRegs();

        PushDynamicRegsAndLR();

        fmov(v0.S(), GetSrc(IROp->Args[0].ID()).S()) ;
        ldr(x0, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.AArch64.FallbackHandlerPointers[Info.HandlerIndex])));
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
        ldr(x0, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.AArch64.FallbackHandlerPointers[Info.HandlerIndex])));
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

        if (Info.ABI == FABI_F80_I16) {
          uxth(w0, GetReg<RA_32>(IROp->Args[0].ID()));
        }
        else {
          mov(w0, GetReg<RA_32>(IROp->Args[0].ID()));
        }
        ldr(x1, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.AArch64.FallbackHandlerPointers[Info.HandlerIndex])));
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
        umov(w1, GetSrc(IROp->Args[0].ID()).V8H(), 4);

        ldr(x2, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.AArch64.FallbackHandlerPointers[Info.HandlerIndex])));
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
        umov(w1, GetSrc(IROp->Args[0].ID()).V8H(), 4);

        ldr(x2, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.AArch64.FallbackHandlerPointers[Info.HandlerIndex])));
        blr(x2);

        PopDynamicRegsAndLR();

        FillStaticRegs();

        mov(GetDst(Node).D(), v0.D());
      }
      break;

      case FABI_F64_F64: {
        SpillStaticRegs();

        PushDynamicRegsAndLR();

        mov(v0.D(), GetSrc(IROp->Args[0].ID()).D());
        ldr(x0, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.AArch64.FallbackHandlerPointers[Info.HandlerIndex])));
        blr(x0);

        PopDynamicRegsAndLR();

        FillStaticRegs();

        mov(GetDst(Node).D(), v0.D());

      }
      break;

      case FABI_F64_F64_F64: {
        SpillStaticRegs();

        PushDynamicRegsAndLR();

        mov(v0.D(), GetSrc(IROp->Args[0].ID()).D());
        mov(v1.D(), GetSrc(IROp->Args[1].ID()).D());
        ldr(x0, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.AArch64.FallbackHandlerPointers[Info.HandlerIndex])));
        blr(x0);

        PopDynamicRegsAndLR();

        FillStaticRegs();

        mov(GetDst(Node).D(), v0.D());

      }
      break;

      case FABI_I16_F80:{
        SpillStaticRegs();

        PushDynamicRegsAndLR();

        umov(x0, GetSrc(IROp->Args[0].ID()).V2D(), 0);
        umov(w1, GetSrc(IROp->Args[0].ID()).V8H(), 4);

        ldr(x2, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.AArch64.FallbackHandlerPointers[Info.HandlerIndex])));
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
        umov(w1, GetSrc(IROp->Args[0].ID()).V8H(), 4);

        ldr(x2, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.AArch64.FallbackHandlerPointers[Info.HandlerIndex])));
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
        umov(w1, GetSrc(IROp->Args[0].ID()).V8H(), 4);

        ldr(x2, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.AArch64.FallbackHandlerPointers[Info.HandlerIndex])));
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
        umov(w1, GetSrc(IROp->Args[0].ID()).V8H(), 4);

        umov(x2, GetSrc(IROp->Args[1].ID()).V2D(), 0);
        umov(w3, GetSrc(IROp->Args[1].ID()).V8H(), 4);

        ldr(x4, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.AArch64.FallbackHandlerPointers[Info.HandlerIndex])));
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
        umov(w1, GetSrc(IROp->Args[0].ID()).V8H(), 4);

        ldr(x2, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.AArch64.FallbackHandlerPointers[Info.HandlerIndex])));
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
        umov(w1, GetSrc(IROp->Args[0].ID()).V8H(), 4);

        umov(x2, GetSrc(IROp->Args[1].ID()).V2D(), 0);
        umov(w3, GetSrc(IROp->Args[1].ID()).V8H(), 4);

        ldr(x4, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.AArch64.FallbackHandlerPointers[Info.HandlerIndex])));
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
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
        LOGMAN_MSG_A_FMT("Unhandled IR Fallback ABI: {} {}", FEXCore::IR::GetName(IROp->Op), Info.ABI);
#endif
      break;
    }
  }
}

void Arm64JITCore::Op_NoOp(IR::IROp_Header *IROp, IR::NodeID Node) {
}

Arm64JITCore::Arm64JITCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread)
  : CPUBackend(Thread, 1024 * 1024 * 16, 1024 * 1024 * 128)
  , Arm64Emitter(ctx, 0)
  , CTX {ctx} {

  RAPass = Thread->PassManager->GetPass<IR::RegisterAllocationPass>("RA");

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

  {
    DispatcherConfig config;
    config.ExitFunctionLink = reinterpret_cast<uintptr_t>(&ExitFunctionLink);
    config.ExitFunctionLinkThis = reinterpret_cast<uintptr_t>(this);
    config.StaticRegisterAssignment = ctx->Config.StaticRegisterAllocation;

    Dispatcher = std::make_unique<Arm64Dispatcher>(CTX, ThreadState, config);
    DispatchPtr = Dispatcher->DispatchPtr;
    CallbackPtr = Dispatcher->CallbackPtr;
  }

  {
    // Set up pointers that the JIT needs to load
    auto &Pointers = ThreadState->CurrentFrame->Pointers.AArch64;
    // Process specific
    Pointers.LUDIV = reinterpret_cast<uint64_t>(LUDIV);
    Pointers.LDIV = reinterpret_cast<uint64_t>(LDIV);
    Pointers.LUREM = reinterpret_cast<uint64_t>(LUREM);
    Pointers.LREM = reinterpret_cast<uint64_t>(LREM);
    Pointers.PrintValue = reinterpret_cast<uint64_t>(PrintValue);
    Pointers.PrintVectorValue = reinterpret_cast<uint64_t>(PrintVectorValue);
    Pointers.RemoveThreadCodeEntryFromJIT = reinterpret_cast<uintptr_t>(&Context::Context::RemoveThreadCodeEntryFromJit);
    Pointers.CPUIDObj = reinterpret_cast<uint64_t>(&CTX->CPUID);

    {
      FEXCore::Utils::MemberFunctionToPointerCast PMF(&FEXCore::CPUIDEmu::RunFunction);
      Pointers.CPUIDFunction = PMF.GetConvertedPointer();
    }

    Pointers.SyscallHandlerObj = reinterpret_cast<uint64_t>(CTX->SyscallHandler);
    Pointers.SyscallHandlerFunc = reinterpret_cast<uint64_t>(FEXCore::Context::HandleSyscall);

    // Fill in the fallback handlers
    InterpreterOps::FillFallbackIndexPointers(Pointers.FallbackHandlerPointers);
  }

  // Must be done after Dispatcher init
  SetAllowAssembler(true);
  ClearCache();
}

void Arm64JITCore::InitializeSignalHandlers(FEXCore::Context::Context *CTX) {
  CTX->SignalDelegation->RegisterHostSignalHandler(SIGILL, [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) -> bool {
    Arm64JITCore *Core = reinterpret_cast<Arm64JITCore*>(Thread->CPUBackend.get());
    return Core->Dispatcher->HandleSIGILL(Signal, info, ucontext);
  }, true);

  CTX->SignalDelegation->RegisterHostSignalHandler(SIGBUS, [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) -> bool {
    Arm64JITCore *Core = reinterpret_cast<Arm64JITCore*>(Thread->CPUBackend.get());

    if (!Core->Dispatcher->IsAddressInJITCode(ArchHelpers::Context::GetPc(ucontext))) {
      // Wasn't a sigbus in JIT code
      return false;
    }

    return FEXCore::ArchHelpers::Arm64::HandleSIGBUS(Core->CTX->Config.ParanoidTSO(), Signal, info, ucontext);
  }, true);

  CTX->SignalDelegation->RegisterHostSignalHandler(SignalDelegator::SIGNAL_FOR_PAUSE, [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) -> bool {
    Arm64JITCore *Core = reinterpret_cast<Arm64JITCore*>(Thread->CPUBackend.get());
    return Core->Dispatcher->HandleSignalPause(Signal, info, ucontext);
  }, true);

  auto GuestSignalHandler = [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext, GuestSigAction *GuestAction, stack_t *GuestStack) -> bool {
    Arm64JITCore *Core = reinterpret_cast<Arm64JITCore*>(Thread->CPUBackend.get());
    return Core->Dispatcher->HandleGuestSignal(Signal, info, ucontext, GuestAction, GuestStack);
  };

  for (uint32_t Signal = 0; Signal <= SignalDelegator::MAX_SIGNALS; ++Signal) {
    CTX->SignalDelegation->RegisterHostSignalHandlerForGuest(Signal, GuestSignalHandler);
  }
}

void Arm64JITCore::EmitDetectionString() {
  const char JITString[] = "FEXJIT::Arm64JITCore::";
  auto Buffer = GetBuffer();
  Buffer->EmitString(JITString);
  Buffer->Align();
}

void Arm64JITCore::ClearCache() {
  // Get the backing code buffer
  
  auto CodeBuffer = GetEmptyCodeBuffer();
  *GetBuffer() = vixl::CodeBuffer(CodeBuffer->Ptr, CodeBuffer->Size);
  EmitDetectionString();
}

Arm64JITCore::~Arm64JITCore() {

}

IR::PhysicalRegister Arm64JITCore::GetPhys(IR::NodeID Node) const {
  auto PhyReg = RAData->GetNodeRegister(Node);

  LOGMAN_THROW_A_FMT(!PhyReg.IsInvalid(), "Couldn't Allocate register for node: ssa{}. Class: {}", Node, PhyReg.Class);

  return PhyReg;
}

template<>
aarch64::Register Arm64JITCore::GetReg<Arm64JITCore::RA_32>(IR::NodeID Node) const {
  auto Reg = GetPhys(Node);

  if (Reg.Class == IR::GPRFixedClass.Val) {
    return SRA64[Reg.Reg].W();
  } else if (Reg.Class == IR::GPRClass.Val) {
    return RA64[Reg.Reg].W();
  } else {
    LOGMAN_THROW_A_FMT(false, "Unexpected Class: {}", Reg.Class);
  }

  FEX_UNREACHABLE;
}

template<>
aarch64::Register Arm64JITCore::GetReg<Arm64JITCore::RA_64>(IR::NodeID Node) const {
  auto Reg = GetPhys(Node);

  if (Reg.Class == IR::GPRFixedClass.Val) {
    return SRA64[Reg.Reg];
  } else if (Reg.Class == IR::GPRClass.Val) {
    return RA64[Reg.Reg];
  } else {
    LOGMAN_THROW_A_FMT(false, "Unexpected Class: {}", Reg.Class);
  }

  FEX_UNREACHABLE;
}

template<>
std::pair<aarch64::Register, aarch64::Register> Arm64JITCore::GetSrcPair<Arm64JITCore::RA_32>(IR::NodeID Node) const {
  uint32_t Reg = GetPhys(Node).Reg;
  return RA32Pair[Reg];
}

template<>
std::pair<aarch64::Register, aarch64::Register> Arm64JITCore::GetSrcPair<Arm64JITCore::RA_64>(IR::NodeID Node) const {
  uint32_t Reg = GetPhys(Node).Reg;
  return RA64Pair[Reg];
}

aarch64::VRegister Arm64JITCore::GetSrc(IR::NodeID Node) const {
  auto Reg = GetPhys(Node);

  if (Reg.Class == IR::FPRFixedClass.Val) {
    return SRAFPR[Reg.Reg];
  } else if (Reg.Class == IR::FPRClass.Val) {
    return RAFPR[Reg.Reg];
  } else {
    LOGMAN_THROW_A_FMT(false, "Unexpected Class: {}", Reg.Class);
  }

  FEX_UNREACHABLE;
}

aarch64::VRegister Arm64JITCore::GetDst(IR::NodeID Node) const {
  auto Reg = GetPhys(Node);

  if (Reg.Class == IR::FPRFixedClass.Val) {
    return SRAFPR[Reg.Reg];
  } else if (Reg.Class == IR::FPRClass.Val) {
    return RAFPR[Reg.Reg];
  } else {
    LOGMAN_THROW_A_FMT(false, "Unexpected Class: {}", Reg.Class);
  }

  FEX_UNREACHABLE;
}

bool Arm64JITCore::IsInlineConstant(const IR::OrderedNodeWrapper& WNode, uint64_t* Value) const {
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

bool Arm64JITCore::IsInlineEntrypointOffset(const IR::OrderedNodeWrapper& WNode, uint64_t* Value) const {
  auto OpHeader = IR->GetOp<IR::IROp_Header>(WNode);

  if (OpHeader->Op == IR::IROps::OP_INLINEENTRYPOINTOFFSET) {
    auto Op = OpHeader->C<IR::IROp_InlineEntrypointOffset>();
    if (Value) {
      uint64_t Mask = ~0ULL;
      uint8_t OpSize = OpHeader->Size;
      if (OpSize == 4) {
        Mask = 0xFFFF'FFFFULL;
      }
      *Value = (Entry + Op->Offset) & Mask;
    }
    return true;
  } else {
    return false;
  }
}

FEXCore::IR::RegisterClassType Arm64JITCore::GetRegClass(IR::NodeID Node) const {
  return FEXCore::IR::RegisterClassType {GetPhys(Node).Class};
}


bool Arm64JITCore::IsFPR(IR::NodeID Node) const {
  auto Class = GetRegClass(Node);

  return Class == IR::FPRClass || Class == IR::FPRFixedClass;
}

bool Arm64JITCore::IsGPR(IR::NodeID Node) const {
  auto Class = GetRegClass(Node);

  return Class == IR::GPRClass || Class == IR::GPRFixedClass;
}

void *Arm64JITCore::CompileCode(uint64_t Entry, [[maybe_unused]] FEXCore::IR::IRListView const *IR, [[maybe_unused]] FEXCore::Core::DebugData *DebugData, FEXCore::IR::RegisterAllocationData *RAData) {
  using namespace aarch64;
  JumpTargets.clear();
  uint32_t SSACount = IR->GetSSACount();

  this->Entry = Entry;
  this->RAData = RAData;

  #ifndef NDEBUG
  LoadConstant(x0, Entry);
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

  GuestEntry = GetCursorAddress<uint64_t>();

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
      LoadConstant(x0, Entry);
      str(x0, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, State.rip)));

      // Stop the thread
      ldr(x0, MemOperand(STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.AArch64.ThreadPauseHandlerSpillSRA)));
      br(x0);
    }
    bind(&RunBlock);
  }

  //LOGMAN_THROW_A_FMT(RAData->HasFullRA(), "Arm64 JIT only works with RA");

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
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
    auto BlockIROp = BlockHeader->CW<FEXCore::IR::IROp_CodeBlock>();
    LOGMAN_THROW_A_FMT(BlockIROp->Header.Op == IR::OP_CODEBLOCK, "IR type failed to be a code block");
#endif

    uintptr_t BlockStartHostCode = GetCursorAddress<uintptr_t>();
    {
      const auto Node = IR->GetID(BlockNode);
      const auto IsTarget = JumpTargets.try_emplace(Node).first;

      // if there's a pending branch, and it is not fall-through
      if (PendingTargetLabel && PendingTargetLabel != &IsTarget->second)
      {
        b(PendingTargetLabel);
      }
      PendingTargetLabel = nullptr;

      bind(&IsTarget->second);
    }

    for (auto [CodeNode, IROp] : IR->GetCode(BlockNode)) {
      const auto ID = IR->GetID(CodeNode);

      // Execute handler
      OpHandler Handler = OpHandlers[IROp->Op];
      (this->*Handler)(IROp, ID);
    }

    if (DebugData) {
      DebugData->Subblocks.push_back({BlockStartHostCode, static_cast<uint32_t>(GetCursorAddress<uintptr_t>() - BlockStartHostCode)});
    }
  }

  // Make sure last branch is generated. It certainly can't be eliminated here.
  if (PendingTargetLabel)
  {
    b(PendingTargetLabel);
  }
  PendingTargetLabel = nullptr;

  FinalizeCode();

  auto CodeEnd = GetCursorAddress<uint64_t>();
  CPU.EnsureIAndDCacheCoherency(reinterpret_cast<void*>(GuestEntry), CodeEnd - reinterpret_cast<uint64_t>(GuestEntry));

  if (DebugData) {
    DebugData->HostCodeSize = reinterpret_cast<uintptr_t>(CodeEnd) - reinterpret_cast<uintptr_t>(GuestEntry);
    DebugData->Relocations = &Relocations;
  }

  this->IR = nullptr;

  return reinterpret_cast<void*>(GuestEntry);
}

uint64_t Arm64JITCore::ExitFunctionLink(Arm64JITCore *core, FEXCore::Core::CpuStateFrame *Frame, uint64_t *record) {
  auto Thread = Frame->Thread;
  auto GuestRip = record[1];

  auto HostCode = Thread->LookupCache->FindBlock(GuestRip);

  if (!HostCode) {
    //fmt::print("ExitFunctionLink: Aborting, {:X} not in cache\n", GuestRip);
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

void Arm64JITCore::ResetStack() {
  if (SpillSlots == 0)
    return;

  if (IsImmAddSub(SpillSlots * 16)) {
    add(sp, sp, SpillSlots * 16);
  } else {
   // Too big to fit in a 12bit immediate
   LoadConstant(x0, SpillSlots * 16);
   add(sp, sp, x0);
  }
}

std::unique_ptr<CPUBackend> CreateArm64JITCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread) {
  return std::make_unique<Arm64JITCore>(ctx, Thread);
}

void InitializeArm64JITSignalHandlers(FEXCore::Context::Context *CTX) {
  Arm64JITCore::InitializeSignalHandlers(CTX);
}
}
