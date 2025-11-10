// SPDX-License-Identifier: MIT
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
#include "Interface/Core/Dispatcher/Dispatcher.h"
#include "Interface/Core/Interpreter/InterpreterOps.h"
#include "Interface/Core/JIT/DebugData.h"
#include "Interface/Core/JIT/JITClass.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"

#include "Utils/MemberFunctionToPointer.h"
#include "Utils/variable_length_integer.h"

#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/EnumUtils.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/LongJump.h>
#include <FEXCore/Utils/Profiler.h>
#include <FEXCore/Utils/Telemetry.h>
#include <FEXCore/Utils/TypeDefines.h>
#include <FEXCore/HLE/SyscallHandler.h>

#include <cstdio>
#include <cstring>
#include <unistd.h>

namespace {
struct DivRem {
  uint64_t Quotient;
  uint64_t Remainder;
};

static struct DivRem LUDIV(uint64_t SrcHigh, uint64_t SrcLow, uint64_t Divisor) {
  __uint128_t Source = (static_cast<__uint128_t>(SrcHigh) << 64) | SrcLow;

  return {
    .Quotient = (uint64_t)(Source / Divisor),
    .Remainder = (uint64_t)(Source % Divisor),
  };
}

static struct DivRem
LDIV(uint64_t SrcHigh, uint64_t SrcLow, int64_t Divisor) {
  __int128_t Source = (static_cast<__uint128_t>(SrcHigh) << 64) | SrcLow;

  return {
    .Quotient = (uint64_t)(Source / Divisor),
    .Remainder = (uint64_t)(Source % Divisor),
  };
}

static void
PrintValue(uint64_t Value) {
  LogMan::Msg::DFmt("Value: 0x{:x}", Value);
}

static void PrintVectorValue(uint64_t Value, uint64_t ValueUpper) {
  LogMan::Msg::DFmt("Value: 0x{:016x}'{:016x}", ValueUpper, Value);
}
} // namespace

namespace FEXCore::CPU {

void Arm64JITCore::Op_Unhandled(const IR::IROp_Header* IROp, IR::Ref Node) {
  FallbackInfo Info;
  if (!InterpreterOps::GetFallbackHandler(IROp, &Info)) {
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
    LOGMAN_MSG_A_FMT("Unhandled IR Op: {}", FEXCore::IR::GetName(IROp->Op));
#endif
  } else {
    auto FillF80x2Result = [&](auto DstLo, auto DstHi) {
      mov(DstLo.Q(), VTMP1.Q());
      mov(DstHi.Q(), VTMP2.Q());
    };

    auto FillF64x2Result = [&](auto DstLo, auto DstHi) {
      fmov(DstLo.D(), VTMP1.D());
      fmov(DstHi.D(), VTMP2.D());
    };

    auto FillF80Result = [&]() {
      const auto Dst = GetVReg(Node);
      mov(Dst.Q(), VTMP1.Q());
    };

    auto FillF64Result = [&]() {
      const auto Dst = GetVReg(Node);
      fmov(Dst.D(), VTMP1.D());
    };

    auto FillF32Result = [&]() {
      const auto Dst = GetVReg(Node);
      fmov(Dst.S(), VTMP1.S());
    };

    auto FillI64Result = [&]() {
      const auto Dst = GetReg(Node);
      mov(Dst.X(), TMP1);
    };

    auto FillI32Result = [&]() {
      const auto Dst = GetReg(Node);
      mov(Dst.W(), TMP1.W());
    };

    auto FillI16Result = [&]() {
      const auto Dst = GetReg(Node);
      mov(Dst.W(), TMP1.W());
    };

    switch (Info.ABI) {
    case FABI_F80_I16_F32_PTR: {
      // Linux Reg/Win32 Reg:
      // tmp4 (x4/x13): FallbackHandler
      // x30: return
      // vtmp1 (v0/v16): source
      str<ARMEmitter::IndexType::PRE>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, -16);

      const auto Src1 = GetVReg(IROp->Args[0]);
      fmov(VTMP1.S(), Src1.S());

      ldr(TMP1, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].ABIHandler));
      ldr(TMP4, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].Func));
      blr(TMP1);

      ldr<ARMEmitter::IndexType::POST>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, 16);
      FillF80Result();
    } break;

    case FABI_F80_I16_F64_PTR: {
      // Linux Reg/Win32 Reg:
      // tmp4 (x4/x13): FallbackHandler
      // x30: return
      // vtmp1 (v0/v16): source
      str<ARMEmitter::IndexType::PRE>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, -16);

      const auto Src1 = GetVReg(IROp->Args[0]);
      fmov(VTMP1.D(), Src1.D());

      ldr(TMP1, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].ABIHandler));
      ldr(TMP4, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].Func));
      blr(TMP1);

      ldr<ARMEmitter::IndexType::POST>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, 16);
      FillF80Result();
    } break;

    case FABI_F80_I16_I16_PTR:
    case FABI_F80_I16_I32_PTR: {
      // Linux Reg/Win32 Reg:
      // tmp4 (x4/x13): FallbackHandler
      // x30: return
      // tmp2 (x1/x11): source
      str<ARMEmitter::IndexType::PRE>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, -16);

      const auto Src1 = GetReg(IROp->Args[0]);

      // Need to sign or zero extend this for the dispatcher handler.
      if (Info.ABI == FABI_F80_I16_I16_PTR) {
        sxth(ARMEmitter::Size::i32Bit, TMP2, Src1);
      } else {
        mov(ARMEmitter::Size::i32Bit, TMP2, Src1);
      }

      ldr(TMP1, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].ABIHandler));
      ldr(TMP4, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].Func));
      blr(TMP1);

      ldr<ARMEmitter::IndexType::POST>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, 16);
      FillF80Result();
    } break;

    case FABI_F32_I16_F80_PTR: {
      // Linux Reg/Win32 Reg:
      // tmp4 (x4/x13): FallbackHandler
      // x30: return
      // vtmp1 (v0/v16): source
      str<ARMEmitter::IndexType::PRE>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, -16);

      const auto Src1 = GetVReg(IROp->Args[0]);
      mov(VTMP1.Q(), Src1.Q());

      ldr(TMP1, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].ABIHandler));
      ldr(TMP4, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].Func));
      blr(TMP1);

      ldr<ARMEmitter::IndexType::POST>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, 16);
      FillF32Result();
    } break;

    case FABI_F64_I16_F80_PTR: {
      // Linux Reg/Win32 Reg:
      // tmp4 (x4/x13): FallbackHandler
      // x30: return
      // vtmp1 (v0/v16): source
      str<ARMEmitter::IndexType::PRE>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, -16);

      const auto Src1 = GetVReg(IROp->Args[0]);
      mov(VTMP1.Q(), Src1.Q());

      ldr(TMP1, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].ABIHandler));
      ldr(TMP4, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].Func));
      blr(TMP1);

      ldr<ARMEmitter::IndexType::POST>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, 16);
      FillF64Result();
    } break;

    case FABI_F64_F64_PTR: {
      // Linux Reg/Win32 Reg:
      // tmp4 (x4/x13): FallbackHandler
      // x30: return
      // vtmp1 (v0/v16): vector source
      str<ARMEmitter::IndexType::PRE>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, -16);

      const auto Src1 = GetVReg(IROp->Args[0]);
      fmov(VTMP1.D(), Src1.D());

      ldr(TMP1, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].ABIHandler));
      ldr(TMP4, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].Func));
      blr(TMP1);

      ldr<ARMEmitter::IndexType::POST>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, 16);
      FillF64Result();
    } break;
    case FABI_F64x2_F64_PTR: {
      // Linux Reg/Win32 Reg:
      // tmp4 (x4/x13): FallbackHandler
      // x30: return
      // vtmp1 (v0/v16): vector source
      // vtmp2 (v1/v16): vector source
#ifdef VIXL_SIMULATOR
      LOGMAN_THROW_A_FMT(CTX->Config.DisableVixlIndirectCalls, "Vector register pairs unsupported by simulator currently");
#endif
      str<ARMEmitter::IndexType::PRE>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, -16);

      const auto Src1 = GetVReg(IROp->Args[0]);
      const auto DstLo = GetVReg(IROp->Args[1]);
      const auto DstHi = GetVReg(IROp->Args[2]);

      fmov(VTMP1.D(), Src1.D());

      ldr(TMP1, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].ABIHandler));
      ldr(TMP4, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].Func));
      blr(TMP1);

      ldr<ARMEmitter::IndexType::POST>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, 16);
      FillF64x2Result(DstLo, DstHi);
    } break;

    case FABI_F64_F64_F64_PTR: {
      // Linux Reg/Win32 Reg:
      // tmp4 (x4/x13): FallbackHandler
      // x30: return
      // vtmp1 (v0/v16): vector source 1
      // vtmp2 (v1/v17): vector source 2
      str<ARMEmitter::IndexType::PRE>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, -16);

      const auto Src1 = GetVReg(IROp->Args[0]);
      const auto Src2 = GetVReg(IROp->Args[1]);

      fmov(VTMP1.D(), Src1.D());
      fmov(VTMP2.D(), Src2.D());

      ldr(TMP1, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].ABIHandler));
      ldr(TMP4, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].Func));
      blr(TMP1);

      ldr<ARMEmitter::IndexType::POST>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, 16);
      FillF64Result();
    } break;

    case FABI_I16_I16_F80_PTR: {
      // Linux Reg/Win32 Reg:
      // tmp4 (x4/x13): FallbackHandler
      // x30: return
      // vtmp1 (v0/v16): source
      str<ARMEmitter::IndexType::PRE>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, -16);

      const auto Src1 = GetVReg(IROp->Args[0]);
      mov(VTMP1.Q(), Src1.Q());

      ldr(TMP1, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].ABIHandler));
      ldr(TMP4, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].Func));
      blr(TMP1);

      ldr<ARMEmitter::IndexType::POST>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, 16);
      FillI16Result();
    } break;

    case FABI_I32_I16_F80_PTR: {
      // Linux Reg/Win32 Reg:
      // tmp4 (x4/x13): FallbackHandler
      // x30: return
      // vtmp1 (v0/v16): source
      str<ARMEmitter::IndexType::PRE>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, -16);

      const auto Src1 = GetVReg(IROp->Args[0]);
      mov(VTMP1.Q(), Src1.Q());

      ldr(TMP1, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].ABIHandler));
      ldr(TMP4, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].Func));
      blr(TMP1);

      ldr<ARMEmitter::IndexType::POST>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, 16);
      FillI32Result();
    } break;

    case FABI_I64_I16_F80_PTR: {
      // Linux Reg/Win32 Reg:
      // tmp4 (x4/x13): FallbackHandler
      // x30: return
      // vtmp1 (v0/v16): source
      str<ARMEmitter::IndexType::PRE>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, -16);

      const auto Src1 = GetVReg(IROp->Args[0]);
      mov(VTMP1.Q(), Src1.Q());

      ldr(TMP1, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].ABIHandler));
      ldr(TMP4, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].Func));
      blr(TMP1);

      ldr<ARMEmitter::IndexType::POST>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, 16);
      FillI64Result();
    } break;

    case FABI_I64_I16_F80_F80_PTR: {
      // Linux Reg/Win32 Reg:
      // tmp4 (x4/x13): FallbackHandler
      // x30: return
      // vtmp1 (v0/v16): vector source 1
      // vtmp2 (v1/v17): vector source 2
      str<ARMEmitter::IndexType::PRE>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, -16);

      const auto Src1 = GetVReg(IROp->Args[0]);
      const auto Src2 = GetVReg(IROp->Args[1]);
      mov(VTMP1.Q(), Src1.Q());
      mov(VTMP2.Q(), Src2.Q());

      ldr(TMP1, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].ABIHandler));
      ldr(TMP4, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].Func));
      blr(TMP1);

      ldr<ARMEmitter::IndexType::POST>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, 16);
      FillI64Result();
    } break;

    case FABI_F80_I16_F80_PTR: {
      // Linux Reg/Win32 Reg:
      // tmp4 (x4/x13): FallbackHandler
      // x30: return
      // vtmp1 (v0/v16): vector source 1
      str<ARMEmitter::IndexType::PRE>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, -16);

      const auto Src1 = GetVReg(IROp->Args[0]);
      mov(VTMP1.Q(), Src1.Q());

      ldr(TMP1, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].ABIHandler));
      ldr(TMP4, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].Func));
      blr(TMP1);

      ldr<ARMEmitter::IndexType::POST>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, 16);
      FillF80Result();
    } break;

    case FABI_F80x2_I16_F80_PTR: {
      // Linux Reg/Win32 Reg:
      // tmp4 (x4/x13): FallbackHandler
      // x30: return
      // vtmp1 (v0/v16): vector source 1
      // vtmp2 (v1/v16): vector source 2
#ifdef VIXL_SIMULATOR
      LOGMAN_THROW_A_FMT(CTX->Config.DisableVixlIndirectCalls, "Vector register pairs unsupported by simulator currently");
#endif
      str<ARMEmitter::IndexType::PRE>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, -16);

      const auto Src1 = GetVReg(IROp->Args[0]);
      const auto DstLo = GetVReg(IROp->Args[1]);
      const auto DstHi = GetVReg(IROp->Args[2]);

      mov(VTMP1.Q(), Src1.Q());

      ldr(TMP1, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].ABIHandler));
      ldr(TMP4, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].Func));
      blr(TMP1);

      ldr<ARMEmitter::IndexType::POST>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, 16);
      FillF80x2Result(DstLo, DstHi);
    } break;

    case FABI_F80_I16_F80_F80_PTR: {
      // Linux Reg/Win32 Reg:
      // tmp4 (x4/x13): FallbackHandler
      // x30: return
      // vtmp1 (v0/v16): vector source 1
      // vtmp2 (v1/v17): vector source 2
      str<ARMEmitter::IndexType::PRE>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, -16);

      const auto Src1 = GetVReg(IROp->Args[0]);
      const auto Src2 = GetVReg(IROp->Args[1]);

      mov(VTMP1.Q(), Src1.Q());
      mov(VTMP2.Q(), Src2.Q());

      ldr(TMP1, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].ABIHandler));
      ldr(TMP4, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].Func));
      blr(TMP1);

      ldr<ARMEmitter::IndexType::POST>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, 16);
      FillF80Result();
    } break;

    case FABI_I32_I64_I64_V128_V128_I16: {
      // Linux Reg/Win32 Reg:
      // stack: FallbackHandler
      // x30: return
      // vtmp1 (v0/v16): vector source 1
      // vtmp2 (v1/v17): vector source 2
      // tmp1 (x0/x10): source 1
      // tmp2 (x1/x11): source 2
      // tmp3 (x2/x12): source 3
      const auto Op = IROp->C<IR::IROp_VPCMPESTRX>();
      ldr(TMP4, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].ABIHandler));
      ldr(TMP1, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].Func));

      stp<ARMEmitter::IndexType::PRE>(TMP1, ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, -16);

      const auto SrcRAX = GetReg(Op->RAX);
      const auto SrcRDX = GetReg(Op->RDX);
      const auto Control = Op->Control;

      mov(TMP1, SrcRAX.X());
      mov(TMP2, SrcRDX.X());
      movz(ARMEmitter::Size::i32Bit, TMP3, Control);

      const auto Src1 = GetVReg(Op->LHS);
      const auto Src2 = GetVReg(Op->RHS);

      mov(VTMP1.Q(), Src1.Q());
      mov(VTMP2.Q(), Src2.Q());

      blr(TMP4);

      ldp<ARMEmitter::IndexType::POST>(ARMEmitter::XReg::zr, ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, 16);
      FillI32Result();
    } break;
    case FABI_I32_V128_V128_I16: {
      // Linux Reg/Win32 Reg:
      // tmp4 (x4/x13): FallbackHandler
      // x30: return
      // vtmp1 (v0/v16): vector source 1
      // vtmp2 (v1/v17): vector source 2
      // tmp1 (x0/x10): source 1
      const auto Op = IROp->C<IR::IROp_VPCMPISTRX>();
      str<ARMEmitter::IndexType::PRE>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, -16);

      const auto Src1 = GetVReg(Op->LHS);
      const auto Src2 = GetVReg(Op->RHS);
      const auto Control = Op->Control;

      mov(VTMP1.Q(), Src1.Q());
      mov(VTMP2.Q(), Src2.Q());
      movz(ARMEmitter::Size::i32Bit, TMP1, Control);

      ldr(TMP2, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].ABIHandler));
      ldr(TMP4, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex].Func));
      blr(TMP2);

      ldr<ARMEmitter::IndexType::POST>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, 16);
      FillI32Result();
    } break;
    case FABI_UNKNOWN:
    default:
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
      LOGMAN_MSG_A_FMT("Unhandled IR Fallback ABI: {} {}", FEXCore::IR::GetName(IROp->Op), ToUnderlying(Info.ABI));
#endif
      break;
    }
  }
}

static void DirectBlockDelinker(FEXCore::Core::CpuStateFrame* Frame, FEXCore::Context::ExitFunctionLinkData* Record, bool Call) {
  uintptr_t JumpThunkStartAddress = reinterpret_cast<uintptr_t>(Record) - 0x10;
  uintptr_t CallerAddress = JumpThunkStartAddress + Record->CallerOffset;
  auto BranchOffset = JumpThunkStartAddress / 4 - CallerAddress / 4;

  // Replace the patched callsite with a branch to the jump thunk.
  uint32_t BranchInst = 0;
  ARMEmitter::Emitter BranchEmit(reinterpret_cast<uint8_t*>(&BranchInst), 4);
  if (Call) {
    BranchEmit.bl(BranchOffset);
  } else {
    BranchEmit.b(BranchOffset);
  }

  std::atomic_ref<uint32_t>(*reinterpret_cast<uint32_t*>(CallerAddress)).store(BranchInst, std::memory_order::relaxed);
  ARMEmitter::Emitter::ClearICache(reinterpret_cast<void*>(CallerAddress), 4);
}

static void IndirectBlockDelinker(FEXCore::Core::CpuStateFrame* Frame, FEXCore::Context::ExitFunctionLinkData* Record) {
  uintptr_t JumpThunkStartAddress = reinterpret_cast<uintptr_t>(Record) - 0x10;
  uint32_t BranchInst = 0;
  ARMEmitter::Emitter BranchEmit(reinterpret_cast<uint8_t*>(&BranchInst), 4);
  BranchEmit.b(0x8);

  std::atomic_ref<uint32_t>(*reinterpret_cast<uint32_t*>(JumpThunkStartAddress)).store(BranchInst, std::memory_order::relaxed);
  ARMEmitter::Emitter::ClearICache(reinterpret_cast<void*>(JumpThunkStartAddress), 4);

  // No need to reset HostCode here as the exit linker pointer is stored separately, and if the block is relinked it will be updated.
}

uint64_t Arm64JITCore::ExitFunctionLink(FEXCore::Core::CpuStateFrame* Frame, FEXCore::Context::ExitFunctionLinkData* Record) {
  auto Thread = Frame->Thread;
  bool TFSet = Thread->CurrentFrame->State.flags[X86State::RFLAG_TF_RAW_LOC];
  uintptr_t HostCode {};
  auto GuestRip = Record->GuestRIP;

  if (TFSet) {
    // If TF is set, the cache must be skipped as different code needs to be generated.
    Frame->State.rip = GuestRip;
    return Frame->Pointers.Common.DispatcherLoopTop;
  } else {
    {
      // Guard the LookupCache lock with the code invalidation mutex, to avoid issues with forking
      auto lk_inval =
        GuardSignalDeferringSection<std::shared_lock>(static_cast<Context::ContextImpl*>(Thread->CTX)->CodeInvalidationMutex, Thread);
      HostCode = Thread->LookupCache->FindBlock(Thread, GuestRip);
    }
    if (!HostCode) {
      // Hold a reference to the code buffer, to avoid linking unmapped code if compilation triggers a recreation.
      auto CodeBuffer = static_cast<Arm64JITCore*>(Thread->CPUBackend.get())->CurrentCodeBuffer;
      HostCode = static_cast<Context::ContextImpl*>(Thread->CTX)->CompileBlock(Frame, GuestRip, 0);
      if (Thread->LookupCache->Shared != CodeBuffer->LookupCache.get()) {
        return HostCode;
      }
    }
  }

  // See ExitFunction in BranchOps.cpp for an assembly level view of the handled cases.
  uintptr_t JumpThunkStartAddress = reinterpret_cast<uintptr_t>(Record) - 0x10;
  uintptr_t CallerAddress = JumpThunkStartAddress + Record->CallerOffset;
  auto BranchOffset = HostCode / 4 - CallerAddress / 4;

  uint32_t ExpectedKnownCallMarkerInst = 0;
  ARMEmitter::Emitter ExpectedKnownCallMarkerEmit(reinterpret_cast<uint8_t*>(&ExpectedKnownCallMarkerInst), 4);
  ExpectedKnownCallMarkerEmit.adr(TMP1, 0xC);

  // Guard the LookupCache lock with the code invalidation mutex, to avoid issues with forking
  auto lk_inval = GuardSignalDeferringSection<std::shared_lock>(static_cast<Context::ContextImpl*>(Thread->CTX)->CodeInvalidationMutex, Thread);

  // Lock here is necessary to prevent simultaneous linking and delinking
  auto lk = Thread->LookupCache->AcquireWriteLock();

  // For non-calls, this would extend into the block's code, however that's fine as an out-of-range adr would never
  // be generated avoiding any false positives.
  uintptr_t KnownCallMarkerAddr = CallerAddress - 0x8;
  uint32_t KnownCallMarkerInst = *reinterpret_cast<uint32_t*>(KnownCallMarkerAddr);
  if (ARMEmitter::Emitter::IsInt26(BranchOffset)) {
    // Directly patch the callsite with the appropriate branch instruction.
    uint32_t BranchInst = 0;
    ARMEmitter::Emitter BranchEmit(reinterpret_cast<uint8_t*>(&BranchInst), 4);

    if (KnownCallMarkerInst == ExpectedKnownCallMarkerInst) {
      BranchEmit.bl(BranchOffset);
      Thread->LookupCache->AddBlockLink(
        GuestRip, Record,
        [](FEXCore::Core::CpuStateFrame* Frame, FEXCore::Context::ExitFunctionLinkData* Record) { DirectBlockDelinker(Frame, Record, true); }, lk);
    } else {
      BranchEmit.b(BranchOffset);
      Thread->LookupCache->AddBlockLink(
        GuestRip, Record,
        [](FEXCore::Core::CpuStateFrame* Frame, FEXCore::Context::ExitFunctionLinkData* Record) {
          DirectBlockDelinker(Frame, Record, false);
        },
        lk);
    }

    std::atomic_ref<uint32_t>(*reinterpret_cast<uint32_t*>(CallerAddress)).store(BranchInst, std::memory_order::relaxed);
    ARMEmitter::Emitter::ClearICache(reinterpret_cast<void*>(CallerAddress), 4);
  } else {
    // This case is common between calls and jumps as the thunk callsite can be left untouched.
    std::atomic_ref<uint64_t>(Record->HostCode).store(HostCode, std::memory_order::seq_cst);
#ifdef _M_ARM_64
    // Make memory write visible to other threads reading the same location
    asm volatile("dc cvau, %0; dsb ish" : : "r"(Record->HostCode) :);
#endif

    uint32_t LdrInst = 0;
    ARMEmitter::Emitter LdrEmit(reinterpret_cast<uint8_t*>(&LdrInst), 4);
    LdrEmit.ldr(TMP1, reinterpret_cast<uint64_t>(&Record->HostCode) - JumpThunkStartAddress);
    std::atomic_ref<uint32_t>(*reinterpret_cast<uint32_t*>(JumpThunkStartAddress)).store(LdrInst, std::memory_order::relaxed);
    ARMEmitter::Emitter::ClearICache(reinterpret_cast<void*>(JumpThunkStartAddress), 4);

    Thread->LookupCache->AddBlockLink(GuestRip, Record, IndirectBlockDelinker, lk);
  }

  return HostCode;
}

void Arm64JITCore::Op_NoOp(const IR::IROp_Header* IROp, IR::Ref Node) {}

Arm64JITCore::Arm64JITCore(FEXCore::Context::ContextImpl* ctx, FEXCore::Core::InternalThreadState* Thread)
  : CPUBackend(*ctx, Thread)
  , Arm64Emitter(ctx)
  , HostSupportsSVE128 {ctx->HostFeatures.SupportsSVE128}
  , HostSupportsSVE256 {ctx->HostFeatures.SupportsSVE256}
  , HostSupportsAVX256 {ctx->HostFeatures.SupportsAVX && ctx->HostFeatures.SupportsSVE256}
  , HostSupportsRPRES {ctx->HostFeatures.SupportsRPRES}
  , HostSupportsAFP {ctx->HostFeatures.SupportsAFP}
  , CTX {ctx}
  , TempAllocator(ctx->CPUBackendAllocator, 0) {

  RAPass = Thread->PassManager->GetPass<IR::RegisterAllocationPass>("RA");

  RAPass->AddRegisters(IR::RegClass::GPR, GeneralRegisters.size());
  RAPass->AddRegisters(IR::RegClass::GPRFixed, StaticRegisters.size());
  RAPass->AddRegisters(IR::RegClass::FPR, GeneralFPRegisters.size());
  RAPass->AddRegisters(IR::RegClass::FPRFixed, StaticFPRegisters.size());
  RAPass->PairRegs = PairRegisters;

  {
    // Set up pointers that the JIT needs to load

    // Common
    auto& Common = ThreadState->CurrentFrame->Pointers.Common;

    Common.PrintValue = reinterpret_cast<uint64_t>(PrintValue);
    Common.PrintVectorValue = reinterpret_cast<uint64_t>(PrintVectorValue);
    Common.ThreadRemoveCodeEntryFromJIT = reinterpret_cast<uintptr_t>(&Context::ContextImpl::ThreadRemoveCodeEntryFromJit);
    Common.MonoBackpatcherWrite = reinterpret_cast<uint64_t>(&Context::ContextImpl::MonoBackpatcherWrite);
    Common.CPUIDObj = reinterpret_cast<uint64_t>(&CTX->CPUID);

    {
      FEXCore::Utils::MemberFunctionToPointerCast PMF(&FEXCore::CPUIDEmu::RunFunction);
      Common.CPUIDFunction = PMF.GetConvertedPointer();
    }

    {
      FEXCore::Utils::MemberFunctionToPointerCast PMF(&FEXCore::CPUIDEmu::RunXCRFunction);
      Common.XCRFunction = PMF.GetConvertedPointer();
    }

    {
      FEXCore::Utils::MemberFunctionToPointerCast PMF(&FEXCore::HLE::SyscallHandler::HandleSyscall);
      Common.SyscallHandlerObj = reinterpret_cast<uint64_t>(CTX->SyscallHandler);
      Common.SyscallHandlerFunc = PMF.GetVTableEntry(CTX->SyscallHandler);
    }
    Common.ExitFunctionLink = reinterpret_cast<uintptr_t>(&Arm64JITCore::ExitFunctionLink);

    // Platform Specific
    auto& AArch64 = ThreadState->CurrentFrame->Pointers.AArch64;

    AArch64.LUDIV = reinterpret_cast<uint64_t>(LUDIV);
    AArch64.LDIV = reinterpret_cast<uint64_t>(LDIV);
  }

  CurrentCodeBuffer = CodeBuffers.GetLatest();
  ThreadState->LookupCache->Shared = CurrentCodeBuffer->LookupCache.get();
}

void Arm64JITCore::EmitDetectionString() {
  const char JITString[] = "FEXJIT::Arm64JITCore::";
  EmitString(JITString);
  Align();
}

void Arm64JITCore::ClearCache() {
  // NOTE: Holding on to the reference here is required to ensure validity of the WriteLock mutex
  auto PrevCodeBuffer = CurrentCodeBuffer;
  auto lk = PrevCodeBuffer->LookupCache->AcquireWriteLock();

  auto CodeBuffer = GetEmptyCodeBuffer();
  SetBuffer(CodeBuffer->Ptr, CodeBuffer->Size);
  EmitDetectionString();

  ThreadState->LookupCache->ChangeGuestToHostMapping(*PrevCodeBuffer, *CurrentCodeBuffer->LookupCache, lk);
}

Arm64JITCore::~Arm64JITCore() {}

bool Arm64JITCore::IsInlineConstant(const IR::OrderedNodeWrapper& WNode, uint64_t* Value) const {
  if (WNode.IsImmediate()) {
    return false;
  }

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
  if (WNode.IsImmediate()) {
    return false;
  }

  auto OpHeader = IR->GetOp<IR::IROp_Header>(WNode);

  if (OpHeader->Op == IR::IROps::OP_INLINEENTRYPOINTOFFSET) {
    auto Op = OpHeader->C<IR::IROp_InlineEntrypointOffset>();
    if (Value) {
      uint64_t Mask = ~0ULL;
      const auto Size = OpHeader->Size;
      if (Size == IR::OpSize::i32Bit) {
        Mask = 0xFFFF'FFFFULL;
      }
      *Value = (Entry + Op->Offset) & Mask;
    }
    return true;
  } else {
    return false;
  }
}

void Arm64JITCore::EmitTFCheck() {
  ARMEmitter::ForwardLabel l_TFUnset;
  ARMEmitter::ForwardLabel l_TFBlocked;

  // Note that this needs to be before the below suspend checks, as X86 checks this flag immediately after executing an instruction.
  ldrb(TMP1, STATE_PTR(CpuStateFrame, State.flags[X86State::RFLAG_TF_RAW_LOC]));

  (void)cbz(ARMEmitter::Size::i32Bit, TMP1, &l_TFUnset);

  // X86 semantically checks TF after executing each instruction, so e.g. setting a context with TF set will execute a single instruction
  // and then raise an exception. However on the FEX side this is simpler to implement by checking at the start of each instruction, handle this by having bit 1 being unset in the flag state indicate that TF is blocked for a single instruction.
  (void)tbz(TMP1, 1, &l_TFBlocked);

  // Block TF for a single instruction when the frontend jumps to a new context by unsetting bit 1.
  ldrb(TMP1, STATE_PTR(CpuStateFrame, State.flags[X86State::RFLAG_TF_RAW_LOC]));
  and_(ARMEmitter::Size::i32Bit, TMP1, TMP1, ~(1 << 1));
  strb(TMP1, STATE_PTR(CpuStateFrame, State.flags[X86State::RFLAG_TF_RAW_LOC]));

  Core::CpuStateFrame::SynchronousFaultDataStruct State = {
    .FaultToTopAndGeneratedException = 1,
    .Signal = Core::FAULT_SIGTRAP,
    .TrapNo = X86State::X86_TRAPNO_DB,
    .si_code = 2,
    .err_code = 0,
  };

  uint64_t Constant {};
  memcpy(&Constant, &State, sizeof(State));

  LoadConstant(ARMEmitter::Size::i64Bit, TMP1, Constant);
  str(TMP1, STATE, offsetof(FEXCore::Core::CpuStateFrame, SynchronousFaultData));
  ldr(TMP1, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.Common.GuestSignal_SIGTRAP));
  br(TMP1);

  (void)Bind(&l_TFBlocked);
  // If TF was blocked for this instruction, unblock it for the next.
  LoadConstant(ARMEmitter::Size::i32Bit, TMP1, 0b11);
  strb(TMP1, STATE_PTR(CpuStateFrame, State.flags[X86State::RFLAG_TF_RAW_LOC]));
  (void)Bind(&l_TFUnset);
}

void Arm64JITCore::EmitSuspendInterruptCheck() {
  if (CTX->Config.NeedsPendingInterruptFaultCheck) {
    // Trigger a fault if there are any pending interrupts
    // Used only for suspend on WIN32 at the moment
    strb(ARMEmitter::XReg::zr, STATE,
         offsetof(FEXCore::Core::InternalThreadState, InterruptFaultPage) - offsetof(FEXCore::Core::InternalThreadState, BaseFrameState));
  }

#ifdef _M_ARM_64EC
  static constexpr uint16_t SuspendMagic {0xCAFE};

  ldr(TMP2.W(), STATE_PTR(CpuStateFrame, SuspendDoorbell));
  ARMEmitter::ForwardLabel l_NoSuspend;
  (void)cbz(ARMEmitter::Size::i32Bit, TMP2, &l_NoSuspend);
  brk(SuspendMagic);
  (void)Bind(&l_NoSuspend);
#endif
}

void Arm64JITCore::EmitEntryPoint(ARMEmitter::BackwardLabel& HeaderLabel, bool CheckTF) {
  // Get the address of the JITCodeHeader and store in to the core state.
  // Two instruction cost, each 1 cycle.
  adr_OrRestart(TMP1, &HeaderLabel);
  str(TMP1, STATE, offsetof(FEXCore::Core::CPUState, InlineJITBlockHeader));

  if (CheckTF) {
    EmitTFCheck();
  }

  if (SpillSlots) {
    const auto TotalSpillSlotsSize = SpillSlots * MaxSpillSlotSize;

    if (ARMEmitter::IsImmAddSub(TotalSpillSlotsSize)) {
      sub(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::rsp, ARMEmitter::Reg::rsp, TotalSpillSlotsSize);
    } else {
      LoadConstant(ARMEmitter::Size::i64Bit, TMP1, TotalSpillSlotsSize);
      sub(ARMEmitter::Size::i64Bit, ARMEmitter::XReg::rsp, ARMEmitter::XReg::rsp, TMP1, ARMEmitter::ExtendedType::LSL_64, 0);
    }
  }

  EmitSuspendInterruptCheck();
}

CPUBackend::CompiledCode Arm64JITCore::CompileCode(uint64_t Entry, uint64_t Size, bool SingleInst, const FEXCore::IR::IRListView* IR,
                                                   FEXCore::Core::DebugData* DebugData, bool CheckTF) {
  FEXCORE_PROFILE_SCOPED("Arm64::CompileCode");
  this->Entry = Entry;
  this->DebugData = DebugData;
  this->IR = IR;
  RequiresFarARM64Jumps = false;
  SSANodeMultiplier = 24;

  // Prepare restart via long jump in case branch encoding fails.
  // This uses UncheckedLongJump since we don't implement std::longjmp in WoA setups
  switch (static_cast<RestartOptions::Control>(FEXCore::UncheckedLongJump::SetJump(ThreadState->RestartJump))) {
  case RestartOptions::Control::Incoming:
    // Nothing
    break;
  case RestartOptions::Control::EnableFarARM64Jumps: RequiresFarARM64Jumps = true; break;
  case RestartOptions::Control::NeedsLargerJITSpace:
    // Get rid of the claimed buffer immediately, we can't fit in it at all.
    TempAllocator.UnclaimBuffer();
    SSANodeMultiplier *= 2;
    break;
  default: LOGMAN_MSG_A_FMT("Unhandled Arm64 restart condition!");
  }

  uint32_t SSACount = IR->GetSSACount();
  JumpTargets.clear();
  CallReturnTargets.clear();
  PendingJumpThunks.clear();
  JumpTargets.resize(IR->GetHeader()->BlockCount, {});

  CodeData.EntryPoints.clear();

  // Fairly excessive buffer range to make sure we don't overflow
  // One page baseline, plus SSANodeMultipler bytes, plus another page for guard page.
  const uint32_t BufferRange = AlignUp(FEXCore::Utils::FEX_PAGE_SIZE * 2 + SSACount * SSANodeMultiplier, FEXCore::Utils::FEX_PAGE_SIZE);
  const uint32_t UsableBufferRange = BufferRange - FEXCore::Utils::FEX_PAGE_SIZE;

  // JIT output is first written to a temporary buffer and later relocated to the CodeBuffer.
  // This minimizes lock contention of CodeBufferWriteMutex.
  auto TempCodeBuffer = TempAllocator.ReownOrClaimBuffer(BufferRange);
  SetBuffer(TempCodeBuffer, UsableBufferRange);

  ThreadState->JITGuardPage = reinterpret_cast<uintptr_t>(TempCodeBuffer) + UsableBufferRange;
  ThreadState->JITGuardOverflowArgument = FEXCore::ToUnderlying(RestartOptions::Control::NeedsLargerJITSpace);

  CodeData.BlockBegin = GetCursorAddress<uint8_t*>();

  // Put the code header at the start of the data block.
  ARMEmitter::BackwardLabel JITCodeHeaderLabel {};
  (void)Bind(&JITCodeHeaderLabel);
  JITCodeHeader* CodeHeader = GetCursorAddress<JITCodeHeader*>();
  CursorIncrement(sizeof(JITCodeHeader));

  auto CodeBegin = GetCursorAddress<uint8_t*>();

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

  SpillSlots = IR->SpillSlots();

  PendingTargetLabel = nullptr;
  PendingCallReturnTargetLabel = nullptr;

  for (auto [BlockNode, BlockHeader] : IR->GetBlocks()) {
    using namespace FEXCore::IR;
    auto BlockIROp = BlockHeader->CW<FEXCore::IR::IROp_CodeBlock>();
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
    LOGMAN_THROW_A_FMT(BlockIROp->Header.Op == IR::OP_CODEBLOCK, "IR type failed to be a code block");
#endif

    auto BlockStartHostCode = GetCursorAddress<uint8_t*>();
    {
      const auto Node = IR->GetID(BlockNode);
      const auto Target = &JumpTargets[BlockIROp->ID];

      // if there's a pending branch, and it is not fall-through
      if (PendingTargetLabel && PendingTargetLabel != Target) {
        if (PendingTargetLabel->Backward.Location) {
          EmitSuspendInterruptCheck();
        }
        b_OrRestart(PendingTargetLabel);
        PendingTargetLabel = nullptr;
      }

      if (BlockIROp->EntryPoint) {
        uint64_t BlockStartRIP = Entry + BlockIROp->GuestEntryOffset;

        const auto IsReturnTarget = CallReturnTargets.try_emplace(Node).first;
        if (PendingTargetLabel) {
          // If there is a fallthrough branch to this block, skip over the entrypoint code.
          b_OrRestart(Target);
        } else if (PendingCallReturnTargetLabel && PendingCallReturnTargetLabel != &IsReturnTarget->second) {
          // If we just emitted a call, but the block we're now emitting is not the return block so don't fallthrough.
          b_OrRestart(PendingCallReturnTargetLabel);
        }
        PendingCallReturnTargetLabel = nullptr;

        BindOrRestart(&IsReturnTarget->second);
        CodeData.EntryPoints.emplace(BlockStartRIP, GetCursorAddress<uint8_t*>());
        DebugData->GuestOpcodes.push_back({BlockIROp->GuestEntryOffset, GetCursorAddress<uint8_t*>() - CodeData.BlockBegin});

        EmitEntryPoint(JITCodeHeaderLabel, CheckTF);
      }

      if (PendingCallReturnTargetLabel) {
        // If there is still a pending call return target, then the block we're emitting is not the return block so don't fallthrough.
        b_OrRestart(PendingCallReturnTargetLabel);
        PendingCallReturnTargetLabel = nullptr;
      }
      PendingTargetLabel = nullptr;

      BindOrRestart(Target);
    }

    for (auto [CodeNode, IROp] : IR->GetCode(BlockNode)) {
      switch (IROp->Op) {
#define REGISTER_OP(op, x) \
  case FEXCore::IR::IROps::OP_##op: Op_##x(IROp, CodeNode); break

#define IROP_DISPATCH_DISPATCH
#include <FEXCore/IR/IRDefines_Dispatch.inc>
#undef REGISTER_OP

      default: Op_Unhandled(IROp, CodeNode); break;
      }
    }

    DebugData->Subblocks.push_back({static_cast<uint32_t>(BlockStartHostCode - CodeData.BlockBegin),
                                    static_cast<uint32_t>(GetCursorAddress<uint8_t*>() - BlockStartHostCode)});
  }

  // Make sure last branch is generated. It certainly can't be eliminated here.
  if (PendingTargetLabel) {
    if (PendingTargetLabel->Backward.Location) {
      EmitSuspendInterruptCheck();
    }
    b_OrRestart(PendingTargetLabel);
  }
  PendingTargetLabel = nullptr;

  ARMEmitter::ForwardLabel l_ExitLink;
  for (auto& PendingJumpThunk : PendingJumpThunks) {
    // Align as 64-bit atomics are used on the HostCode field.
    Align(8);

    ARMEmitter::ForwardLabel l_DoLink;
    uint64_t ThunkAddress = GetCursorAddress<uint64_t>();
    BindOrRestart(&PendingJumpThunk.Label);
    b_OrRestart(&l_DoLink);
    br(TMP1);
    BindOrRestart(&l_DoLink);
    ldr(TMP1, &l_ExitLink);
    blr(TMP1);

    // This is a ExitFunctionLinkData struct
    BindOrRestart(&l_ExitLink);
    dc64(0);                                             // HostCode
    dc64(PendingJumpThunk.GuestRIP);                     // GuestRIP
    dc64(PendingJumpThunk.CallerAddress - ThunkAddress); // CallerOffset
  }

  BindOrRestart(&l_ExitLink);
  dc64(ThreadState->CurrentFrame->Pointers.Common.ExitFunctionLinker);

  // CodeSize not including the header or tail data.
  const uint64_t CodeOnlySize = GetCursorAddress<uint8_t*>() - CodeBegin;

  // Add the JitCodeTail
  Align(alignof(JITCodeTail));
  auto JITBlockTailLocation = GetCursorAddress<uint8_t*>();
  auto JITBlockTail = GetCursorAddress<JITCodeTail*>();
  CursorIncrement(sizeof(JITCodeTail));

  // Entries that live after the JITCodeTail.
  // These entries correlate JIT code regions with guest RIP regions.
  // Using these entries FEX is able to reconstruct the guest RIP accurately when an instruction cause a signal fault.
  // Packed using two variable length integer entries to ensure the size isn't too large.
  // These smaller sizes means that each entry is relative to each other instead of absolute offset from the start of the JIT block.
  // When reconstructing the RIP, each entry must be walked linearly and accumulated with the previous entries.
  // This is a trade-off between compression inside the JIT code space and execution time when reconstruction the RIP.
  // RIP reconstruction when faulting is less likely so we are requiring the accumulation.
  //
  // struct {
  //   // The Host PC offset from the previous entry.
  //   FEXCore::Utils::vl64 HostPCOffset;
  //   // How much to offset the RIP from the previous entry.
  //   FEXCore::Utils::vl64 GuestRIPOffset;
  // };

  auto JITRIPEntriesBegin = GetCursorAddress<uint8_t*>();

  // Put the block's RIP entry in the tail.
  // This will be used for RIP reconstruction in the future.
  // TODO: This needs to be a data RIP relocation once code caching works.
  //   Current relocation code doesn't support this feature yet.
  JITBlockTail->RIP = Entry;
  JITBlockTail->GuestSize = Size;
  JITBlockTail->SingleInst = SingleInst;
  JITBlockTail->SpinLockFutex = 0;

  auto JITRIPEntriesLocation = JITRIPEntriesBegin;

  {
    // Store the RIP entries.
    JITBlockTail->NumberOfRIPEntries = DebugData->GuestOpcodes.size();
    JITBlockTail->OffsetToRIPEntries = JITRIPEntriesBegin - JITBlockTailLocation;
    uintptr_t CurrentRIPOffset = 0;
    uint64_t CurrentPCOffset = 0;

    for (size_t i = 0; i < DebugData->GuestOpcodes.size(); i++) {
      const auto& GuestOpcode = DebugData->GuestOpcodes[i];
      int64_t HostPCOffset = GuestOpcode.HostEntryOffset - CurrentPCOffset;
      int64_t GuestRIPOffset = GuestOpcode.GuestEntryOffset - CurrentRIPOffset;

      JITRIPEntriesLocation += FEXCore::Utils::vl64pair::Encode(JITRIPEntriesLocation, HostPCOffset, GuestRIPOffset);

      CurrentPCOffset = GuestOpcode.HostEntryOffset;
      CurrentRIPOffset = GuestOpcode.GuestEntryOffset;
    }
  }

  CursorIncrement(JITRIPEntriesLocation - JITRIPEntriesBegin);
  Align();

  CodeHeader->OffsetToBlockTail = JITBlockTailLocation - CodeData.BlockBegin;

  CodeData.Size = GetCursorAddress<uint8_t*>() - CodeData.BlockBegin;

  JITBlockTail->Size = CodeData.Size;

  // Migrate the compile output from temporary storage to the actual CodeBuffer.
  // This can block progress in other compiling threads, so the duration of the lock should be as small as possible.
  {
    auto CodeBufferLock = std::unique_lock {CodeBuffers.CodeBufferWriteMutex};

    // Query size of generated code
    const auto TempSize = GetCursorOffset();

    // Bring CodeBuffer up to date
    {
      LOGMAN_THROW_A_FMT(CurrentCodeBuffer->LookupCache.get() == ThreadState->LookupCache->Shared, "INVARIANT VIOLATED: SharedLookupCache "
                                                                                                   "doesn't match up!\n");
      if (auto Prev = CheckCodeBufferUpdate()) {
        Allocator::VirtualDontNeed(ThreadState->CallRetStackBase, FEXCore::Core::InternalThreadState::CALLRET_STACK_SIZE);
        auto lk = ThreadState->LookupCache->AcquireWriteLock();
        ThreadState->LookupCache->ChangeGuestToHostMapping(*Prev, *CurrentCodeBuffer->LookupCache, lk);
      }

      // NOTE: 16-byte alignment of the new cursor offset must be preserved for block linking records
      SetBuffer(CurrentCodeBuffer->Ptr, CurrentCodeBuffer->Size);
      SetCursorOffset(AlignUp(CodeBuffers.LatestOffset, 16));
      if ((GetCursorOffset() + TempSize) > (CurrentCodeBuffer->Size - Utils::FEX_PAGE_SIZE)) {
        CTX->ClearCodeCache(ThreadState);
      }

      Align16B();

      CodeBuffers.LatestOffset = GetCursorOffset();
    }

    // Adjust host addresses
    const auto Delta = GetCursorAddress<uint8_t*>() - CodeData.BlockBegin;
    CodeData.BlockBegin += Delta;
    for (auto& EntryPoint : CodeData.EntryPoints) {
      EntryPoint.second += Delta;
    }
    CodeBegin += Delta;

    // Copy over CodeBuffer contents
    memcpy(GetCursorAddress<uint8_t*>(), TempCodeBuffer, TempSize);
    SetCursorOffset(CodeBuffers.LatestOffset + TempSize);

    CodeBuffers.LatestOffset = GetCursorOffset();
  }

  TempAllocator.DelayedDisownBuffer();

  ClearICache(CodeBegin, CodeOnlySize);

#ifdef VIXL_DISASSEMBLER
  if (Disassemble() & FEXCore::Config::Disassemble::STATS) {
    auto HeaderOp = IR->GetHeader();
    LOGMAN_THROW_A_FMT(HeaderOp->Header.Op == IR::OP_IRHEADER, "First op wasn't IRHeader");

    LogMan::Msg::IFmt("RIP: 0x{:x}", Entry);
    LogMan::Msg::IFmt("Guest Code instructions: {}", HeaderOp->NumHostInstructions);
    LogMan::Msg::IFmt("Host Code instructions: {}", CodeOnlySize >> 2);
    LogMan::Msg::IFmt("Blow-up Amt: {}x", double(CodeOnlySize >> 2) / double(HeaderOp->NumHostInstructions));
  }

  if (Disassemble() & FEXCore::Config::Disassemble::BLOCKS) {
    const auto DisasmBegin = reinterpret_cast<const vixl::aarch64::Instruction*>(CodeBegin);
    const auto DisasmEnd = reinterpret_cast<const vixl::aarch64::Instruction*>(CodeBegin + CodeOnlySize);
    LogMan::Msg::IFmt("Disassemble Begin");
    for (auto PCToDecode = DisasmBegin; PCToDecode < DisasmEnd; PCToDecode += 4) {
      DisasmDecoder->Decode(PCToDecode);
      auto Output = Disasm->GetOutput();
      LogMan::Msg::IFmt("{}", Output);
    }
    LogMan::Msg::IFmt("Disassemble End");
  }
#endif

  DebugData->HostCodeSize = CodeData.Size;
  DebugData->Relocations = &Relocations;

  this->IR = nullptr;

  return std::move(CodeData);
}

void Arm64JITCore::ResetStack() {
  if (SpillSlots == 0) {
    return;
  }

  const auto TotalSpillSlotsSize = SpillSlots * MaxSpillSlotSize;

  if (ARMEmitter::IsImmAddSub(TotalSpillSlotsSize)) {
    add(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::rsp, ARMEmitter::Reg::rsp, TotalSpillSlotsSize);
  } else {
    // Too big to fit in a 12bit immediate
    LoadConstant(ARMEmitter::Size::i64Bit, TMP1, TotalSpillSlotsSize);
    add(ARMEmitter::Size::i64Bit, ARMEmitter::XReg::rsp, ARMEmitter::XReg::rsp, TMP1, ARMEmitter::ExtendedType::LSL_64, 0);
  }
}

fextl::unique_ptr<CPUBackend> CreateArm64JITCore(FEXCore::Context::ContextImpl* ctx, FEXCore::Core::InternalThreadState* Thread) {
  return fextl::make_unique<Arm64JITCore>(ctx, Thread);
}

} // namespace FEXCore::CPU
