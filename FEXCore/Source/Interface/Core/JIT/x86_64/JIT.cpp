/*
$info$
tags: backend|x86-64
desc: Main glue logic of the x86-64 splatter backend
$end_info$
*/

#include "Interface/Context/Context.h"
#include "Interface/Core/LookupCache.h"

#include "Interface/Core/Dispatcher/Dispatcher.h"
#include "Interface/Core/Dispatcher/X86Dispatcher.h"
#include "Interface/Core/Interpreter/InterpreterOps.h"
#include "Interface/Core/JIT/x86_64/JITClass.h"
#include "Interface/IR/PassManager.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"

#include "Utils/MemberFunctionToPointer.h"

#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/IR/RegisterAllocationData.h>
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/EnumUtils.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/Profiler.h>
#include <FEXCore/fextl/sstream.h>

#include <algorithm>
#include <array>
#include <memory>
#include <stddef.h>
#include <stdint.h>
#include <signal.h>
#include <tuple>
#include <unordered_map>
#include <utility>

// #define DEBUG_RA 1
// #define DEBUG_CYCLES

static constexpr size_t INITIAL_CODE_SIZE = 1024 * 1024 * 16;
static constexpr size_t MAX_CODE_SIZE = 1024 * 1024 * 256;

namespace {
static void PrintValue(uint64_t Value) {
  LogMan::Msg::DFmt("Value: 0x{:x}", Value);
}

static void PrintVectorValue(uint64_t Value, uint64_t ValueUpper) {
  LogMan::Msg::DFmt("Value: 0x{:016x}'{:016x}", ValueUpper, Value);
}
}

namespace FEXCore::CPU {

void X86JITCore::PushRegs() {
  const auto AVXRegSize = Core::CPUState::XMM_AVX_REG_SIZE;

  sub(rsp, AVXRegSize * RAXMM_x.size());
  for (size_t i = 0; i < RAXMM_x.size(); ++i) {
    vmovups(ptr[rsp + i * AVXRegSize], ToYMM(RAXMM_x[i]));
  }

  for (const auto &Reg : RA64) {
    push(Reg);
  }

  const auto NumPush = RA64.size();
  if ((NumPush & 1) != 0) {
    // Align
    sub(rsp, 8);
  }
}

void X86JITCore::PopRegs() {
  const auto AVXRegSize = Core::CPUState::XMM_AVX_REG_SIZE;
  const auto NumPush = RA64.size();

  if ((NumPush & 1) != 0) {
    // Align
    add(rsp, 8);
  }

  for (uint32_t i = RA64.size(); i > 0; --i) {
    pop(RA64[i - 1]);
  }

  for (size_t i = 0; i < RAXMM_x.size(); ++i) {
    vmovups(ToYMM(RAXMM_x[i]), ptr[rsp + i * AVXRegSize]);
  }

  add(rsp, AVXRegSize * RAXMM_x.size());
}

void X86JITCore::Op_Unhandled(IR::IROp_Header *IROp, IR::NodeID Node) {
  FallbackInfo Info;
  if (!InterpreterOps::GetFallbackHandler(IROp, &Info)) {
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
    LOGMAN_MSG_A_FMT("Unhandled IR Op: {}", FEXCore::IR::GetName(IROp->Op));
#endif
  } else {
    switch(Info.ABI) {
      case FABI_VOID_U16: {
        PushRegs();
        mov(edi, GetSrc<RA_32>(IROp->Args[0].ID()));
        call(qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex])]);

        PopRegs();
        break;
      }
      case FABI_F80_F32:{
        PushRegs();

        movss(xmm0, GetSrc(IROp->Args[0].ID()));
        call(qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex])]);

        PopRegs();

        pxor(GetDst(Node), GetDst(Node));
        movq(GetDst(Node), rax);
        pinsrw(GetDst(Node), edx, 4);
      }
      break;

      case FABI_F80_F64:{
        PushRegs();

        movsd(xmm0, GetSrc(IROp->Args[0].ID()));
        call(qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex])]);

        PopRegs();

        pxor(GetDst(Node), GetDst(Node));
        movq(GetDst(Node), rax);
        pinsrw(GetDst(Node), edx, 4);
      }
      break;

      case FABI_F80_I16:
      case FABI_F80_I32: {
        PushRegs();

        if (Info.ABI == FABI_F80_I16) {
          movsx(rdi, GetSrc<RA_32>(IROp->Args[0].ID()).cvt16());
        }
        else {
          mov(edi, GetSrc<RA_32>(IROp->Args[0].ID()));
        }
        call(qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex])]);

        PopRegs();

        pxor(GetDst(Node), GetDst(Node));
        movq(GetDst(Node), rax);
        pinsrw(GetDst(Node), edx, 4);
      }
      break;

      case FABI_F32_F80:{
        PushRegs();

        movq(rdi, GetSrc(IROp->Args[0].ID()));
        pextrq(rsi, GetSrc(IROp->Args[0].ID()), 1);

        call(qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex])]);

        PopRegs();

        movss(GetDst(Node), xmm0);
      }
      break;

      case FABI_F64_F80:{
        PushRegs();

        movq(rdi, GetSrc(IROp->Args[0].ID()));
        pextrq(rsi, GetSrc(IROp->Args[0].ID()), 1);

        call(qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex])]);

        PopRegs();

        movsd(GetDst(Node), xmm0);
      }
      break;

      case FABI_F64_F64: {
        PushRegs();

        movsd(xmm0, GetSrc(IROp->Args[0].ID()));

        call(qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex])]);

        PopRegs();

        movsd(GetDst(Node), xmm0);
      }
      break;

      case FABI_F64_F64_F64: {
        PushRegs();

        movsd(xmm0, GetSrc(IROp->Args[0].ID()));
        movsd(xmm1, GetSrc(IROp->Args[1].ID()));

        call(qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex])]);

        PopRegs();

        movsd(GetDst(Node), xmm0);
      }
      break;

      case FABI_I16_F80:{
        PushRegs();

        movq(rdi, GetSrc(IROp->Args[0].ID()));
        pextrq(rsi, GetSrc(IROp->Args[0].ID()), 1);

        call(qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex])]);

        PopRegs();

        movsx(GetDst<RA_64>(Node), ax);
      }
      break;
      case FABI_I32_F80:{
        PushRegs();

        movq(rdi, GetSrc(IROp->Args[0].ID()));
        pextrq(rsi, GetSrc(IROp->Args[0].ID()), 1);

        call(qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex])]);

        PopRegs();

        mov(GetDst<RA_32>(Node), eax);
      }
      break;
      case FABI_I64_F80:{
        PushRegs();

        movq(rdi, GetSrc(IROp->Args[0].ID()));
        pextrq(rsi, GetSrc(IROp->Args[0].ID()), 1);

        call(qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex])]);

        PopRegs();

        mov(GetDst<RA_64>(Node), rax);
      }
      break;
      case FABI_I64_F80_F80:{
        PushRegs();

        movq(rdi, GetSrc(IROp->Args[0].ID()));
        pextrq(rsi, GetSrc(IROp->Args[0].ID()), 1);

        movq(rdx, GetSrc(IROp->Args[1].ID()));
        pextrq(rcx, GetSrc(IROp->Args[1].ID()), 1);

        call(qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex])]);

        PopRegs();

        mov(GetDst<RA_64>(Node), rax);
      }
      break;
      case FABI_F80_F80:{
        PushRegs();

        movq(rdi, GetSrc(IROp->Args[0].ID()));
        pextrq(rsi, GetSrc(IROp->Args[0].ID()), 1);

        call(qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex])]);

        PopRegs();

        pxor(GetDst(Node), GetDst(Node));
        movq(GetDst(Node), rax);
        pinsrw(GetDst(Node), edx, 4);
      }
      break;
      case FABI_F80_F80_F80:{
        PushRegs();

        movq(rdi, GetSrc(IROp->Args[0].ID()));
        pextrq(rsi, GetSrc(IROp->Args[0].ID()), 1);

        movq(rdx, GetSrc(IROp->Args[1].ID()));
        pextrq(rcx, GetSrc(IROp->Args[1].ID()), 1);

        call(qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex])]);

        PopRegs();

        pxor(GetDst(Node), GetDst(Node));
        movq(GetDst(Node), rax);
        pinsrw(GetDst(Node), edx, 4);
      }
      break;

      case FABI_I32_I64_I64_I128_I128_I16: {
        PushRegs();

        const auto Op = IROp->C<IR::IROp_VPCMPESTRX>();
        const auto Control = Op->Control;

        const auto LHS = GetSrc(Op->LHS.ID());
        const auto RHS = GetSrc(Op->RHS.ID());
        const auto SrcRAX = GetSrc<RA_64>(Op->RAX.ID());
        const auto SrcRDX = GetSrc<RA_64>(Op->RDX.ID());

        mov(rdi, SrcRAX);
        mov(rsi, SrcRDX);

        movq(rdx, LHS);
        pextrq(rcx, LHS, 1);

        movq(r8, RHS);
        pextrq(r9, RHS, 1);

        sub(rsp, 16);
        mov(dword [rsp], Control);

        call(qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex])]);

        add(rsp, 16);
        PopRegs();

        mov(GetDst<RA_32>(Node), rax);
        break;
      }

      case FABI_I32_I128_I128_I16: {
        PushRegs();

        const auto Op = IROp->C<IR::IROp_VPCMPISTRX>();

        const auto LHS = GetSrc(Op->LHS.ID());
        const auto RHS = GetSrc(Op->RHS.ID());
        const auto Control = Op->Control;

        movq(rdi, LHS);
        pextrq(rsi, LHS, 1);

        movq(rdx, RHS);
        pextrq(rcx, RHS, 1);

        mov(r8, Control);

        call(qword [STATE + offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex])]);

        PopRegs();

        mov(GetDst<RA_32>(Node), rax);
        break;
      }

      case FABI_UNKNOWN:
      default:
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
        LOGMAN_MSG_A_FMT("Unhandled IR Fallback ABI: {} {}",
                         IR::GetName(IROp->Op), ToUnderlying(Info.ABI));
#endif
        break;
    }
  }
}

static uint64_t X86JITCore_ExitFunctionLink(FEXCore::Core::CpuStateFrame *Frame, uint64_t *record) {
  auto Thread = Frame->Thread;
  auto GuestRip = record[1];

  auto HostCode = Thread->LookupCache->FindBlock(GuestRip);

  if (!HostCode) {
    Thread->CurrentFrame->State.rip = GuestRip;
    return Frame->Pointers.Common.DispatcherLoopTop;
  }

  auto LinkerAddress = Frame->Pointers.Common.ExitFunctionLinker;
  Thread->LookupCache->AddBlockLink(GuestRip, (uintptr_t)record, [record, LinkerAddress]{
    // undo the link
    record[0] = LinkerAddress;
  });

  record[0] = HostCode;
  return HostCode;
}

void X86JITCore::Op_NoOp(IR::IROp_Header *IROp, IR::NodeID Node) {
}

X86JITCore::X86JITCore(FEXCore::Context::ContextImpl *ctx, FEXCore::Core::InternalThreadState *Thread)
  : CPUBackend(Thread, INITIAL_CODE_SIZE, MAX_CODE_SIZE)
  , CodeGenerator(0, this, nullptr) // this is not used here
  , CTX {ctx} {

  RAPass = Thread->PassManager->GetPass<IR::RegisterAllocationPass>("RA");

  RAPass->AllocateRegisterSet(RegisterClasses);
  RAPass->AddRegisters(FEXCore::IR::GPRClass, NumGPRs);
  RAPass->AddRegisters(FEXCore::IR::FPRClass, NumXMMs);
  RAPass->AddRegisters(FEXCore::IR::GPRPairClass, NumGPRPairs);

  for (uint32_t i = 0; i < NumGPRPairs; ++i) {
    RAPass->AddRegisterConflict(FEXCore::IR::GPRClass, i * 2,     FEXCore::IR::GPRPairClass, i);
    RAPass->AddRegisterConflict(FEXCore::IR::GPRClass, i * 2 + 1, FEXCore::IR::GPRPairClass, i);
  }

  for (uint32_t i = 0; i < FEXCore::IR::IROps::OP_LAST + 1; ++i) {
    OpHandlers[i] = &X86JITCore::Op_Unhandled;
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
    auto &Common = ThreadState->CurrentFrame->Pointers.Common;

    Common.PrintValue = reinterpret_cast<uint64_t>(PrintValue);
    Common.PrintVectorValue = reinterpret_cast<uint64_t>(PrintVectorValue);
    Common.ThreadRemoveCodeEntryFromJIT = reinterpret_cast<uintptr_t>(&Context::ContextImpl::ThreadRemoveCodeEntryFromJit);
    Common.CPUIDObj = reinterpret_cast<uint64_t>(&CTX->CPUID);

    {
      FEXCore::Utils::MemberFunctionToPointerCast PMF(&FEXCore::CPUIDEmu::RunFunction);
      Common.CPUIDFunction = PMF.GetConvertedPointer();
    }

    {
      FEXCore::Utils::MemberFunctionToPointerCast PMF(&FEXCore::CPUIDEmu::RunXCRFunction);
      Common.XCRFunction = PMF.GetConvertedPointer();
    }

    Common.SyscallHandlerObj = reinterpret_cast<uint64_t>(CTX->SyscallHandler);
    Common.SyscallHandlerFunc = reinterpret_cast<uint64_t>(FEXCore::Context::HandleSyscall);
    Common.ExitFunctionLink = reinterpret_cast<uintptr_t>(&Context::ContextImpl::ThreadExitFunctionLink<X86JITCore_ExitFunctionLink>);

    // Fill in the fallback handlers
    InterpreterOps::FillFallbackIndexPointers(Common.FallbackHandlerPointers);
  }

  // Must be done after Dispatcher init
  ClearCache();
}

X86JITCore::~X86JITCore() {

}

void X86JITCore::EmitDetectionString() {
  const char JITString[] = "FEXJIT::X86JITCore::";
  for (char c : JITString) {
    db(c);
  }
}

void X86JITCore::ClearCache() {
  auto CodeBuffer = GetEmptyCodeBuffer();
  setNewBuffer(CodeBuffer->Ptr, CodeBuffer->Size);
  EmitDetectionString();
}

IR::PhysicalRegister X86JITCore::GetPhys(IR::NodeID Node) const {
  auto PhyReg = RAData->GetNodeRegister(Node);

  LOGMAN_THROW_AA_FMT(PhyReg.Raw != 255, "Couldn't Allocate register for node: ssa{}. Class: {}", Node, PhyReg.Class);

  return PhyReg;
}

bool X86JITCore::IsFPR(IR::NodeID Node) const {
  return RAData->GetNodeRegister(Node).Class == IR::FPRClass;
}

bool X86JITCore::IsGPR(IR::NodeID Node) const {
  return RAData->GetNodeRegister(Node).Class == IR::GPRClass;
}

bool X86JITCore::IsGPRPair(IR::NodeID Node) const {
  return RAData->GetNodeRegister(Node).Class == IR::GPRPairClass;
}

template<uint8_t RAType>
Xbyak::Reg X86JITCore::GetSrc(IR::NodeID Node) const {
  // rax, rcx, rdx, rsi, r8, r9,
  // r10
  // Callee Saved
  // rbx, rbp, r12, r13, r14, r15
  auto PhyReg = GetPhys(Node);
  if constexpr (RAType == RA_64)
    return RA64[PhyReg.Reg].cvt64();
  else if constexpr (RAType == RA_XMM)
    return RAXMM[PhyReg.Reg];
  else if constexpr (RAType == RA_32)
    return RA64[PhyReg.Reg].cvt32();
  else if constexpr (RAType == RA_16)
    return RA64[PhyReg.Reg].cvt16();
  else if constexpr (RAType == RA_8)
    return RA64[PhyReg.Reg].cvt8();
}

template
Xbyak::Reg X86JITCore::GetSrc<X86JITCore::RA_64>(IR::NodeID Node) const;

template
Xbyak::Reg X86JITCore::GetSrc<X86JITCore::RA_32>(IR::NodeID Node) const;

template
Xbyak::Reg X86JITCore::GetSrc<X86JITCore::RA_16>(IR::NodeID Node) const;

template
Xbyak::Reg X86JITCore::GetSrc<X86JITCore::RA_8>(IR::NodeID Node) const;

Xbyak::Xmm X86JITCore::GetSrc(IR::NodeID Node) const {
  auto PhyReg = GetPhys(Node);
  return RAXMM_x[PhyReg.Reg];
}

template<uint8_t RAType>
Xbyak::Reg X86JITCore::GetDst(IR::NodeID Node) const {
  auto PhyReg = GetPhys(Node);
  if constexpr (RAType == RA_64)
    return RA64[PhyReg.Reg].cvt64();
  else if constexpr (RAType == RA_XMM)
    return RAXMM[PhyReg.Reg];
  else if constexpr (RAType == RA_32)
    return RA64[PhyReg.Reg].cvt32();
  else if constexpr (RAType == RA_16)
    return RA64[PhyReg.Reg].cvt16();
  else if constexpr (RAType == RA_8)
    return RA64[PhyReg.Reg].cvt8();
}

template
Xbyak::Reg X86JITCore::GetDst<X86JITCore::RA_64>(IR::NodeID Node) const;

template
Xbyak::Reg X86JITCore::GetDst<X86JITCore::RA_32>(IR::NodeID Node) const;

template
Xbyak::Reg X86JITCore::GetDst<X86JITCore::RA_16>(IR::NodeID Node) const;

template
Xbyak::Reg X86JITCore::GetDst<X86JITCore::RA_8>(IR::NodeID Node) const;

template<uint8_t RAType>
std::pair<Xbyak::Reg, Xbyak::Reg> X86JITCore::GetSrcPair(IR::NodeID Node) const {
  auto PhyReg = GetPhys(Node);
  if constexpr (RAType == RA_64)
    return RA64Pair[PhyReg.Reg];
  else if constexpr (RAType == RA_32)
    return {RA64Pair[PhyReg.Reg].first.cvt32(), RA64Pair[PhyReg.Reg].second.cvt32()};
}

template
std::pair<Xbyak::Reg, Xbyak::Reg> X86JITCore::GetSrcPair<X86JITCore::RA_64>(IR::NodeID Node) const;

template
std::pair<Xbyak::Reg, Xbyak::Reg> X86JITCore::GetSrcPair<X86JITCore::RA_32>(IR::NodeID Node) const;

Xbyak::Xmm X86JITCore::GetDst(IR::NodeID Node) const {
  auto PhyReg = GetPhys(Node);
  return RAXMM_x[PhyReg.Reg];
}

bool X86JITCore::IsInlineConstant(const IR::OrderedNodeWrapper& WNode, uint64_t* Value) const {
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

bool X86JITCore::IsInlineEntrypointOffset(const IR::OrderedNodeWrapper& WNode, uint64_t* Value) const {
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

std::tuple<X86JITCore::SetCC, X86JITCore::CMovCC, X86JITCore::JCC> X86JITCore::GetCC(IR::CondClassType cond) {
    switch (cond) {
    case IR::CondClassType::EQ:  return { &CodeGenerator::sete , &CodeGenerator::cmove , &CodeGenerator::je  };
    case IR::CondClassType::NEQ: return { &CodeGenerator::setne, &CodeGenerator::cmovne, &CodeGenerator::jne };
    case IR::CondClassType::SGE: return { &CodeGenerator::setge, &CodeGenerator::cmovge, &CodeGenerator::jge };
    case IR::CondClassType::SLT: return { &CodeGenerator::setl , &CodeGenerator::cmovl , &CodeGenerator::jl  };
    case IR::CondClassType::SGT: return { &CodeGenerator::setg , &CodeGenerator::cmovg , &CodeGenerator::jg  };
    case IR::CondClassType::SLE: return { &CodeGenerator::setle, &CodeGenerator::cmovle, &CodeGenerator::jle };
    case IR::CondClassType::UGE: return { &CodeGenerator::setae, &CodeGenerator::cmovae, &CodeGenerator::jae };
    case IR::CondClassType::ULT: return { &CodeGenerator::setb , &CodeGenerator::cmovb , &CodeGenerator::jb  };
    case IR::CondClassType::UGT: return { &CodeGenerator::seta , &CodeGenerator::cmova , &CodeGenerator::ja  };
    case IR::CondClassType::ULE: return { &CodeGenerator::setna, &CodeGenerator::cmovna, &CodeGenerator::jna };

    case IR::CondClassType::FLU:  return { &CodeGenerator::setb , &CodeGenerator::cmovb , &CodeGenerator::jb  };
    case IR::CondClassType::FGE:  return { &CodeGenerator::setae, &CodeGenerator::cmovae, &CodeGenerator::jae };
    case IR::CondClassType::FLEU: return { &CodeGenerator::setna, &CodeGenerator::cmovna, &CodeGenerator::jna };
    case IR::CondClassType::FGT:  return { &CodeGenerator::seta , &CodeGenerator::cmova , &CodeGenerator::ja  };
    case IR::CondClassType::FU:   return { &CodeGenerator::setp , &CodeGenerator::cmovp , &CodeGenerator::jp  };
    case IR::CondClassType::FNU:  return { &CodeGenerator::setnp, &CodeGenerator::cmovnp, &CodeGenerator::jnp };

    case IR::CondClassType::MI:
    case IR::CondClassType::PL:
    case IR::CondClassType::VS:
    case IR::CondClassType::VC:
    default:
      LOGMAN_MSG_A_FMT("Unsupported compare type");
      break;
  }

  // Hope for the best
  return { &CodeGenerator::sete , &CodeGenerator::cmove , &CodeGenerator::je  };
}

CPUBackend::CompiledCode X86JITCore::CompileCode(uint64_t Entry, [[maybe_unused]] FEXCore::IR::IRListView const *IR, [[maybe_unused]] FEXCore::Core::DebugData *DebugData, FEXCore::IR::RegisterAllocationData *RAData, bool GDBEnabled) {

  FEXCORE_PROFILE_SCOPED("x86::CompileCode");
  JumpTargets.clear();
  uint32_t SSACount = IR->GetSSACount();

  this->Entry = Entry;
  this->RAData = RAData;
  this->DebugData = DebugData;

  // Fairly excessive buffer range to make sure we don't overflow
  uint32_t BufferRange = SSACount * 16 + GDBEnabled * Dispatcher::MaxGDBPauseCheckSize;
  if ((getSize() + BufferRange) > CurrentCodeBuffer->Size) {
    CTX->ClearCodeCache(ThreadState);
  }

  CodeData.BlockBegin = getCurr<uint8_t*>();

  // Put the code header at the start of the data block.
  Label JITCodeHeaderLabel{};
  L(JITCodeHeaderLabel);

  JITCodeHeader *CodeHeader = getCurr<JITCodeHeader *>();
  setSize(getSize() + sizeof(JITCodeHeader));

  CodeData.BlockEntry = getCurr<uint8_t*>();

  // Get the address of the JITCodeHeader and store in to the core state.
  // Only two instructions, so very low overhead.
  lea(TMP1, ptr [rip + JITCodeHeaderLabel]);
  mov(qword [STATE + offsetof(FEXCore::Core::CPUState, InlineJITBlockHeader)], TMP1);

  CursorEntry = getSize();
  this->IR = IR;

  if (GDBEnabled) {
    auto GDBSize = CTX->Dispatcher->GenerateGDBPauseCheck(CodeData.BlockBegin, Entry);
    setSize(getSize() + GDBSize);
  }

  LOGMAN_THROW_AA_FMT(RAData != nullptr, "Needs RA");

  SpillSlots = RAData->SpillSlots();

  if (SpillSlots) {
    sub(rsp, SpillSlots * MaxSpillSlotSize);
  }

#ifdef BLOCKSTATS
  BlockSamplingData::BlockData *SamplingData = CTX->BlockData->GetBlockData(Entry);
  if (GetSamplingData) {
    mov(rcx, reinterpret_cast<uintptr_t>(SamplingData));
    rdtsc();
    shl(rdx, 32);
    or_(rax, rdx);
    mov(qword [rcx + offsetof(BlockSamplingData::BlockData, Start)], rax);
  }

  auto ExitBlock = [&]() {
    if (GetSamplingData) {
      mov(rcx, reinterpret_cast<uintptr_t>(SamplingData));
      // Get time
      rdtsc();
      shl(rdx, 32);
      or_(rax, rdx);

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

  PendingTargetLabel = nullptr;

  for (auto [BlockNode, BlockHeader] : IR->GetBlocks()) {
    using namespace FEXCore::IR;
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
    auto BlockIROp = BlockHeader->CW<IROp_CodeBlock>();
    LOGMAN_THROW_AA_FMT(BlockIROp->Header.Op == IR::OP_CODEBLOCK, "IR type failed to be a code block");
#endif

    auto BlockStartHostCode = getCurr<uint8_t *>();
    {
      const auto Node = IR->GetID(BlockNode);
      const auto IsTarget = JumpTargets.try_emplace(Node).first;

      // if there is a pending branch, and it is not fall-through
      if (PendingTargetLabel && PendingTargetLabel != &IsTarget->second) {
        jmp(*PendingTargetLabel, T_NEAR);
      }
      PendingTargetLabel = nullptr;

      L(IsTarget->second);
    }

    for (auto [CodeNode, IROp] : IR->GetCode(BlockNode)) {
      #ifdef DEBUG_RA
      if (IROp->Op != IR::OP_BEGINBLOCK &&
          IROp->Op != IR::OP_CONDJUMP &&
          IROp->Op != IR::OP_JUMP) {
        fextl::stringstream Inst;
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

        const uint8_t NumArgs = IR::GetArgs(IROp->Op);
        for (uint8_t i = 0; i < NumArgs; ++i) {
          const auto ArgNode = IROp->Args[i].ID();
          const uint64_t PhysReg = RAPass->GetNodeRegister(ArgNode);
          if (PhysReg >= GPRPairBase)
            Inst << "Pair" << GetPhys(ArgNode) << (i + 1 == NumArgs ? "" : ", ");
          else if (PhysReg >= XMMBase)
            Inst << "XMM" << GetPhys(ArgNode) << (i + 1 == NumArgs ? "" : ", ");
          else
            Inst << "Reg" << GetPhys(ArgNode) << (i + 1 == NumArgs ? "" : ", ");
        }

        LogMan::Msg::DFmt("{}", Inst.str());
      }
      #endif
      const auto ID = IR->GetID(CodeNode);

      // Execute handler
      OpHandler Handler = OpHandlers[IROp->Op];
      (this->*Handler)(IROp, ID);
    }

    if (DebugData) {
      DebugData->Subblocks.push_back({
        static_cast<uint32_t>(BlockStartHostCode - CodeData.BlockBegin),
        static_cast<uint32_t>(getCurr<uint8_t *>() - BlockStartHostCode)
      });
    }
  }

  // Make sure last branch is generated. It certainly can't be eliminated here.
  if (PendingTargetLabel)
  {
    jmp(*PendingTargetLabel, T_NEAR);
  }
  PendingTargetLabel = nullptr;

  // Add the JitCodeTail
  auto JITBlockTailLocation = getCurr<uint8_t *>();
  auto JITBlockTail = getCurr<JITCodeTail*>();
  setSize(getSize() + sizeof(JITCodeTail));

  auto JITRIPEntriesLocation = getCurr<uint8_t *>();
  auto JITRIPEntries = getCurr<JITRIPReconstructEntries*>();

  setSize(getSize() + sizeof(JITRIPReconstructEntries) * DebugData->GuestOpcodes.size());

  // Put the block's RIP entry in the tail.
  // This will be used for RIP reconstruction in the future.
  // TODO: This needs to be a data RIP relocation once code caching works.
  //   Current relocation code doesn't support this feature yet.
  JITBlockTail->RIP = Entry;

  {
    // Store the RIP entries.
    JITBlockTail->NumberOfRIPEntries = DebugData->GuestOpcodes.size();
    JITBlockTail->OffsetToRIPEntries = JITRIPEntriesLocation - JITBlockTailLocation;
    uintptr_t CurrentRIPOffset = 0;
    uint64_t CurrentPCOffset = 0;
    for (size_t i = 0; i < DebugData->GuestOpcodes.size(); i++) {
      const auto &GuestOpcode = DebugData->GuestOpcodes[i];
      auto &RIPEntry = JITRIPEntries[i];
      RIPEntry.HostPCOffset = GuestOpcode.HostEntryOffset - CurrentPCOffset;
      RIPEntry.GuestRIPOffset = GuestOpcode.GuestEntryOffset - CurrentRIPOffset;
      CurrentPCOffset = GuestOpcode.HostEntryOffset;
      CurrentRIPOffset = GuestOpcode.GuestEntryOffset;
    }
  }

  CodeHeader->OffsetToBlockTail = JITBlockTailLocation - CodeData.BlockBegin;

  CodeData.Size = getCurr<uint8_t*>() - CodeData.BlockBegin;

  JITBlockTail->Size = CodeData.Size;

  this->IR = nullptr;

  ready();

  if (DebugData) {
    DebugData->HostCodeSize = CodeData.Size;
    DebugData->Relocations = &Relocations;
  }

  return CodeData;
}

fextl::unique_ptr<CPUBackend> CreateX86JITCore(FEXCore::Context::ContextImpl *ctx, FEXCore::Core::InternalThreadState *Thread) {
  return fextl::make_unique<X86JITCore>(ctx, Thread);
}

CPUBackendFeatures GetX86JITBackendFeatures() {
  return CPUBackendFeatures { };
}

}
