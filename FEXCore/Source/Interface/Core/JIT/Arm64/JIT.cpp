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

#include "FEXCore/Utils/Telemetry.h"
#include "Interface/Context/Context.h"
#include "Interface/Core/ArchHelpers/CodeEmitter/Emitter.h"
#include "Interface/Core/LookupCache.h"

#include "Interface/Core/Dispatcher/Arm64Dispatcher.h"
#include "Interface/Core/JIT/Arm64/JITClass.h"
#include "Interface/Core/InternalThreadState.h"

#include "Interface/IR/Passes/RegisterAllocationPass.h"

#include "Utils/MemberFunctionToPointer.h"

#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/EnumUtils.h>
#include <FEXCore/Utils/Profiler.h>

#include "Interface/Core/Interpreter/InterpreterOps.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>

static constexpr size_t INITIAL_CODE_SIZE = 1024 * 1024 * 16;
// We don't want to move above 128MB atm because that means we will have to encode longer jumps
static constexpr size_t MAX_CODE_SIZE = 1024 * 1024 * 128;

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

void Arm64JITCore::Op_Unhandled(IR::IROp_Header const *IROp, IR::NodeID Node) {
  FallbackInfo Info;
  if (!InterpreterOps::GetFallbackHandler(IROp, &Info)) {
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
    LOGMAN_MSG_A_FMT("Unhandled IR Op: {}", FEXCore::IR::GetName(IROp->Op));
#endif
  } else {
    switch(Info.ABI) {
      case FABI_F80_I16_F32:{
        SpillForABICall(Info.SupportsPreserveAllABI, TMP1, true);

        const auto Src1 = GetVReg(IROp->Args[0].ID());
        fmov(ARMEmitter::SReg::s0, Src1.S());
        ldrh(ARMEmitter::WReg::w0, STATE, offsetof(FEXCore::Core::CPUState, FCW));
        ldr(ARMEmitter::XReg::x1, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex]));
#ifdef VIXL_SIMULATOR
        GenerateIndirectRuntimeCall<__uint128_t, uint16_t, float>(ARMEmitter::Reg::r1);
#else
        blr(ARMEmitter::Reg::r1);
#endif

        FillForABICall(Info.SupportsPreserveAllABI, true);

        const auto Dst = GetVReg(Node);
        eor(Dst.Q(), Dst.Q(), Dst.Q());
        ins(ARMEmitter::SubRegSize::i64Bit, Dst, 0, ARMEmitter::Reg::r0);
        ins(ARMEmitter::SubRegSize::i16Bit, Dst, 4, ARMEmitter::Reg::r1);
      }
      break;

      case FABI_F80_I16_F64:{
        SpillForABICall(Info.SupportsPreserveAllABI, TMP1, true);

        const auto Src1 = GetVReg(IROp->Args[0].ID());
        mov(ARMEmitter::DReg::d0, Src1.D());
        ldrh(ARMEmitter::WReg::w0, STATE, offsetof(FEXCore::Core::CPUState, FCW));
        ldr(ARMEmitter::XReg::x1, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex]));
#ifdef VIXL_SIMULATOR
        GenerateIndirectRuntimeCall<__uint128_t, uint16_t, double>(ARMEmitter::Reg::r1);
#else
        blr(ARMEmitter::Reg::r1);
#endif

        FillForABICall(Info.SupportsPreserveAllABI, true);

        const auto Dst = GetVReg(Node);
        eor(Dst.Q(), Dst.Q(), Dst.Q());
        ins(ARMEmitter::SubRegSize::i64Bit, Dst, 0, ARMEmitter::Reg::r0);
        ins(ARMEmitter::SubRegSize::i16Bit, Dst, 4, ARMEmitter::Reg::r1);
      }
      break;

      case FABI_F80_I16_I16:
      case FABI_F80_I16_I32: {
        SpillForABICall(Info.SupportsPreserveAllABI, TMP1, true);

        const auto Src1 = GetReg(IROp->Args[0].ID());
        ldrh(ARMEmitter::WReg::w0, STATE, offsetof(FEXCore::Core::CPUState, FCW));
        if (Info.ABI == FABI_F80_I16_I16) {
          sxth(ARMEmitter::Size::i32Bit, ARMEmitter::Reg::r1, Src1);
        }
        else {
          mov(ARMEmitter::Size::i32Bit, ARMEmitter::Reg::r1, Src1);
        }
        ldr(ARMEmitter::XReg::x2, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex]));
#ifdef VIXL_SIMULATOR
        GenerateIndirectRuntimeCall<__uint128_t, uint16_t, uint32_t>(ARMEmitter::Reg::r2);
#else
        blr(ARMEmitter::Reg::r2);
#endif

        FillForABICall(Info.SupportsPreserveAllABI, true);

        const auto Dst = GetVReg(Node);
        eor(Dst.Q(), Dst.Q(), Dst.Q());
        ins(ARMEmitter::SubRegSize::i64Bit, Dst, 0, ARMEmitter::Reg::r0);
        ins(ARMEmitter::SubRegSize::i16Bit, Dst, 4, ARMEmitter::Reg::r1);
      }
      break;

      case FABI_F32_I16_F80:{
        SpillForABICall(Info.SupportsPreserveAllABI, TMP1, true);

        const auto Src1 = GetVReg(IROp->Args[0].ID());

        ldrh(ARMEmitter::WReg::w0, STATE, offsetof(FEXCore::Core::CPUState, FCW));
        umov<ARMEmitter::SubRegSize::i64Bit>(ARMEmitter::Reg::r1, Src1, 0);
        umov<ARMEmitter::SubRegSize::i16Bit>(ARMEmitter::Reg::r2, Src1, 4);

        ldr(ARMEmitter::XReg::x3, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex]));
#ifdef VIXL_SIMULATOR
        GenerateIndirectRuntimeCall<float, uint16_t, uint64_t, uint64_t>(ARMEmitter::Reg::r3);
#else
        blr(ARMEmitter::Reg::r3);
#endif

        FillForABICall(Info.SupportsPreserveAllABI, true);

        const auto Dst = GetVReg(Node);
        fmov(Dst.S(), ARMEmitter::SReg::s0);
      }
      break;

      case FABI_F64_I16_F80:{
        SpillForABICall(Info.SupportsPreserveAllABI, TMP1, true);

        const auto Src1 = GetVReg(IROp->Args[0].ID());

        ldrh(ARMEmitter::WReg::w0, STATE, offsetof(FEXCore::Core::CPUState, FCW));
        umov<ARMEmitter::SubRegSize::i64Bit>(ARMEmitter::Reg::r1, Src1, 0);
        umov<ARMEmitter::SubRegSize::i16Bit>(ARMEmitter::Reg::r2, Src1, 4);

        ldr(ARMEmitter::XReg::x3, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex]));
#ifdef VIXL_SIMULATOR
        GenerateIndirectRuntimeCall<double, uint16_t, uint64_t, uint64_t>(ARMEmitter::Reg::r3);
#else
        blr(ARMEmitter::Reg::r3);
#endif

        FillForABICall(Info.SupportsPreserveAllABI, true);

        const auto Dst = GetVReg(Node);
        mov(Dst.D(), ARMEmitter::DReg::d0);
      }
      break;

      case FABI_F64_I16_F64: {
        SpillForABICall(Info.SupportsPreserveAllABI, TMP1, true);

        const auto Src1 = GetVReg(IROp->Args[0].ID());

        mov(ARMEmitter::DReg::d0, Src1.D());
        ldrh(ARMEmitter::WReg::w0, STATE, offsetof(FEXCore::Core::CPUState, FCW));
        ldr(ARMEmitter::XReg::x1, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex]));
#ifdef VIXL_SIMULATOR
        GenerateIndirectRuntimeCall<double, uint16_t, double>(ARMEmitter::Reg::r1);
#else
        blr(ARMEmitter::Reg::r1);
#endif

        FillForABICall(Info.SupportsPreserveAllABI, true);

        const auto Dst = GetVReg(Node);
        mov(Dst.D(), ARMEmitter::DReg::d0);
      }
      break;

      case FABI_F64_I16_F64_F64: {
        SpillForABICall(Info.SupportsPreserveAllABI, TMP1, true);

        const auto Src1 = GetVReg(IROp->Args[0].ID());
        const auto Src2 = GetVReg(IROp->Args[1].ID());

        mov(ARMEmitter::DReg::d0, Src1.D());
        mov(ARMEmitter::DReg::d1, Src2.D());
        ldrh(ARMEmitter::WReg::w0, STATE, offsetof(FEXCore::Core::CPUState, FCW));
        ldr(ARMEmitter::XReg::x1, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex]));
#ifdef VIXL_SIMULATOR
        GenerateIndirectRuntimeCall<double, uint16_t, double, double>(ARMEmitter::Reg::r1);
#else
        blr(ARMEmitter::Reg::r1);
#endif

        FillForABICall(Info.SupportsPreserveAllABI, true);

        const auto Dst = GetVReg(Node);
        mov(Dst.D(), ARMEmitter::DReg::d0);
      }
      break;

      case FABI_I16_I16_F80:{
        SpillForABICall(Info.SupportsPreserveAllABI, TMP1, true);

        const auto Src1 = GetVReg(IROp->Args[0].ID());

        ldrh(ARMEmitter::WReg::w0, STATE, offsetof(FEXCore::Core::CPUState, FCW));
        umov<ARMEmitter::SubRegSize::i64Bit>(ARMEmitter::Reg::r1, Src1, 0);
        umov<ARMEmitter::SubRegSize::i16Bit>(ARMEmitter::Reg::r2, Src1, 4);

        ldr(ARMEmitter::XReg::x3, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex]));
#ifdef VIXL_SIMULATOR
        GenerateIndirectRuntimeCall<uint32_t, uint16_t, uint64_t, uint64_t>(ARMEmitter::Reg::r3);
#else
        blr(ARMEmitter::Reg::r3);
#endif

        FillForABICall(Info.SupportsPreserveAllABI, true);

        const auto Dst = GetReg(Node);
        sxth(ARMEmitter::Size::i64Bit, Dst, ARMEmitter::Reg::r0);
      }
      break;
      case FABI_I32_I16_F80:{
        SpillForABICall(Info.SupportsPreserveAllABI, TMP1, true);

        const auto Src1 = GetVReg(IROp->Args[0].ID());

        ldrh(ARMEmitter::WReg::w0, STATE, offsetof(FEXCore::Core::CPUState, FCW));
        umov<ARMEmitter::SubRegSize::i64Bit>(ARMEmitter::Reg::r1, Src1, 0);
        umov<ARMEmitter::SubRegSize::i16Bit>(ARMEmitter::Reg::r2, Src1, 4);

        ldr(ARMEmitter::XReg::x3, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex]));
#ifdef VIXL_SIMULATOR
        GenerateIndirectRuntimeCall<uint32_t, uint16_t, uint64_t, uint64_t>(ARMEmitter::Reg::r3);
#else
        blr(ARMEmitter::Reg::r3);
#endif

        FillForABICall(Info.SupportsPreserveAllABI, true);

        const auto Dst = GetReg(Node);
        mov(ARMEmitter::Size::i32Bit, Dst, ARMEmitter::Reg::r0);
      }
      break;
      case FABI_I64_I16_F80:{
        SpillForABICall(Info.SupportsPreserveAllABI, TMP1, true);

        const auto Src1 = GetVReg(IROp->Args[0].ID());

        ldrh(ARMEmitter::WReg::w0, STATE, offsetof(FEXCore::Core::CPUState, FCW));
        umov<ARMEmitter::SubRegSize::i64Bit>(ARMEmitter::Reg::r1, Src1, 0);
        umov<ARMEmitter::SubRegSize::i16Bit>(ARMEmitter::Reg::r2, Src1, 4);

        ldr(ARMEmitter::XReg::x3, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex]));
#ifdef VIXL_SIMULATOR
        GenerateIndirectRuntimeCall<uint64_t, uint16_t, uint64_t, uint64_t>(ARMEmitter::Reg::r3);
#else
        blr(ARMEmitter::Reg::r3);
#endif
        FillForABICall(Info.SupportsPreserveAllABI, true);

        const auto Dst = GetReg(Node);
        mov(ARMEmitter::Size::i64Bit, Dst, ARMEmitter::Reg::r0);
      }
      break;
      case FABI_I64_I16_F80_F80:{
        SpillForABICall(Info.SupportsPreserveAllABI, TMP1, true);

        const auto Src1 = GetVReg(IROp->Args[0].ID());
        const auto Src2 = GetVReg(IROp->Args[1].ID());

        ldrh(ARMEmitter::WReg::w0, STATE, offsetof(FEXCore::Core::CPUState, FCW));
        umov<ARMEmitter::SubRegSize::i64Bit>(ARMEmitter::Reg::r1, Src1, 0);
        umov<ARMEmitter::SubRegSize::i16Bit>(ARMEmitter::Reg::r2, Src1, 4);

        umov<ARMEmitter::SubRegSize::i64Bit>(ARMEmitter::Reg::r3, Src2, 0);
        umov<ARMEmitter::SubRegSize::i16Bit>(ARMEmitter::Reg::r4, Src2, 4);

        ldr(ARMEmitter::XReg::x5, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex]));
#ifdef VIXL_SIMULATOR
        GenerateIndirectRuntimeCall<uint64_t, uint16_t, uint64_t, uint64_t, uint64_t, uint64_t>(ARMEmitter::Reg::r5);
#else
        blr(ARMEmitter::Reg::r5);
#endif
        FillForABICall(Info.SupportsPreserveAllABI, true);

        const auto Dst = GetReg(Node);
        mov(ARMEmitter::Size::i64Bit, Dst, ARMEmitter::Reg::r0);
      }
      break;
      case FABI_F80_I16_F80:{
        SpillForABICall(Info.SupportsPreserveAllABI, TMP1, true);

        const auto Src1 = GetVReg(IROp->Args[0].ID());

        ldrh(ARMEmitter::WReg::w0, STATE, offsetof(FEXCore::Core::CPUState, FCW));
        umov<ARMEmitter::SubRegSize::i64Bit>(ARMEmitter::Reg::r1, Src1, 0);
        umov<ARMEmitter::SubRegSize::i16Bit>(ARMEmitter::Reg::r2, Src1, 4);

        ldr(ARMEmitter::XReg::x3, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex]));
#ifdef VIXL_SIMULATOR
        GenerateIndirectRuntimeCall<__uint128_t, uint16_t, uint64_t, uint64_t>(ARMEmitter::Reg::r3);
#else
        blr(ARMEmitter::Reg::r3);
#endif

        FillForABICall(Info.SupportsPreserveAllABI, true);

        const auto Dst = GetVReg(Node);
        eor(Dst.Q(), Dst.Q(), Dst.Q());
        ins(ARMEmitter::SubRegSize::i64Bit, Dst, 0, ARMEmitter::Reg::r0);
        ins(ARMEmitter::SubRegSize::i16Bit, Dst, 4, ARMEmitter::Reg::r1);
      }
      break;
      case FABI_F80_I16_F80_F80:{
        SpillForABICall(Info.SupportsPreserveAllABI, TMP1, true);

        const auto Src1 = GetVReg(IROp->Args[0].ID());
        const auto Src2 = GetVReg(IROp->Args[1].ID());

        ldrh(ARMEmitter::WReg::w0, STATE, offsetof(FEXCore::Core::CPUState, FCW));
        umov<ARMEmitter::SubRegSize::i64Bit>(ARMEmitter::Reg::r1, Src1, 0);
        umov<ARMEmitter::SubRegSize::i16Bit>(ARMEmitter::Reg::r2, Src1, 4);

        umov<ARMEmitter::SubRegSize::i64Bit>(ARMEmitter::Reg::r3, Src2, 0);
        umov<ARMEmitter::SubRegSize::i16Bit>(ARMEmitter::Reg::r4, Src2, 4);

        ldr(ARMEmitter::XReg::x5, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex]));
#ifdef VIXL_SIMULATOR
        GenerateIndirectRuntimeCall<__uint128_t, uint16_t, uint64_t, uint64_t, uint64_t, uint64_t>(ARMEmitter::Reg::r5);
#else
        blr(ARMEmitter::Reg::r5);
#endif

        FillForABICall(Info.SupportsPreserveAllABI, true);

        const auto Dst = GetVReg(Node);
        eor(Dst.Q(), Dst.Q(), Dst.Q());
        ins(ARMEmitter::SubRegSize::i64Bit, Dst, 0, ARMEmitter::Reg::r0);
        ins(ARMEmitter::SubRegSize::i16Bit, Dst, 4, ARMEmitter::Reg::r1);
      }
      break;
      case FABI_I32_I64_I64_I128_I128_I16: {
        SpillForABICall(Info.SupportsPreserveAllABI, TMP1, true);

        const auto Op = IROp->C<IR::IROp_VPCMPESTRX>();
        const auto Control = Op->Control;

        const auto Src1 = GetVReg(Op->LHS.ID());
        const auto Src2 = GetVReg(Op->RHS.ID());
        const auto SrcRAX = GetReg(Op->RAX.ID());
        const auto SrcRDX = GetReg(Op->RDX.ID());

        mov(ARMEmitter::XReg::x0, SrcRAX.X());
        mov(ARMEmitter::XReg::x1, SrcRDX.X());

        umov<ARMEmitter::SubRegSize::i64Bit>(ARMEmitter::Reg::r2, Src1, 0);
        umov<ARMEmitter::SubRegSize::i64Bit>(ARMEmitter::Reg::r3, Src1, 1);

        umov<ARMEmitter::SubRegSize::i64Bit>(ARMEmitter::Reg::r4, Src2, 0);
        umov<ARMEmitter::SubRegSize::i64Bit>(ARMEmitter::Reg::r5, Src2, 1);

        movz(ARMEmitter::Size::i32Bit, ARMEmitter::Reg::r6, Control);

        ldr(ARMEmitter::XReg::x7, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex]));
#ifdef VIXL_SIMULATOR
        GenerateIndirectRuntimeCall<uint32_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint16_t>(ARMEmitter::Reg::r7);
#else
        blr(ARMEmitter::Reg::r7);
#endif

        FillForABICall(Info.SupportsPreserveAllABI, true);

        const auto Dst = GetReg(Node);
        mov(Dst.W(), ARMEmitter::WReg::w0);
        break;
      }
      case FABI_I32_I128_I128_I16: {
        SpillForABICall(Info.SupportsPreserveAllABI, TMP1, true);

        const auto Op = IROp->C<IR::IROp_VPCMPISTRX>();

        const auto Src1 = GetVReg(Op->LHS.ID());
        const auto Src2 = GetVReg(Op->RHS.ID());
        const auto Control = Op->Control;

        umov<ARMEmitter::SubRegSize::i64Bit>(ARMEmitter::Reg::r0, Src1, 0);
        umov<ARMEmitter::SubRegSize::i64Bit>(ARMEmitter::Reg::r1, Src1, 1);

        umov<ARMEmitter::SubRegSize::i64Bit>(ARMEmitter::Reg::r2, Src2, 0);
        umov<ARMEmitter::SubRegSize::i64Bit>(ARMEmitter::Reg::r3, Src2, 1);

        movz(ARMEmitter::Size::i32Bit, ARMEmitter::Reg::r4, Control);

        ldr(ARMEmitter::XReg::x5, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex]));
#ifdef VIXL_SIMULATOR
        GenerateIndirectRuntimeCall<uint32_t, uint64_t, uint64_t, uint64_t, uint64_t, uint16_t>(ARMEmitter::Reg::r5);
#else
        blr(ARMEmitter::Reg::r5);
#endif

        FillForABICall(Info.SupportsPreserveAllABI, true);

        const auto Dst = GetReg(Node);
        mov(Dst.W(), ARMEmitter::WReg::w0);
        break;
      }
      case FABI_UNKNOWN:
      default:
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
        LOGMAN_MSG_A_FMT("Unhandled IR Fallback ABI: {} {}",
                         FEXCore::IR::GetName(IROp->Op), ToUnderlying(Info.ABI));
#endif
      break;
    }
  }
}


static uint64_t Arm64JITCore_ExitFunctionLink(FEXCore::Core::CpuStateFrame *Frame, uint64_t *record) {
  auto Thread = Frame->Thread;
  auto GuestRip = record[1];

  auto HostCode = Thread->LookupCache->FindBlock(GuestRip);

  if (!HostCode) {
    Frame->State.rip = GuestRip;
    return Frame->Pointers.Common.DispatcherLoopTop;
  }

  uintptr_t branch = (uintptr_t)(record) - 8;
  auto LinkerAddress = Frame->Pointers.Common.ExitFunctionLinker;

  auto offset = HostCode/4 - branch/4;
  if (vixl::IsInt26(offset)) {
    // optimal case - can branch directly
    // patch the code
    FEXCore::ARMEmitter::Emitter emit((uint8_t*)(branch), 24);
    emit.b(offset);
    FEXCore::ARMEmitter::Emitter::ClearICache((void*)branch, 24);

    // Add de-linking handler
    Thread->LookupCache->AddBlockLink(GuestRip, (uintptr_t)record, [branch, LinkerAddress]{
      FEXCore::ARMEmitter::Emitter emit((uint8_t*)(branch), 24);
      FEXCore::ARMEmitter::ForwardLabel l_BranchHost;
      emit.ldr(FEXCore::ARMEmitter::XReg::x0, &l_BranchHost);
      emit.blr(FEXCore::ARMEmitter::Reg::r0);
      emit.Bind(&l_BranchHost);
      emit.dc64(LinkerAddress);
      FEXCore::ARMEmitter::Emitter::ClearICache((void*)branch, 24);
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

void Arm64JITCore::Op_NoOp(IR::IROp_Header const *IROp, IR::NodeID Node) {
}

Arm64JITCore::Arm64JITCore(FEXCore::Context::ContextImpl *ctx, FEXCore::Core::InternalThreadState *Thread)
  : CPUBackend(Thread, INITIAL_CODE_SIZE, MAX_CODE_SIZE)
  , Arm64Emitter(ctx, 0)
  , HostSupportsSVE128{ctx->HostFeatures.SupportsSVE}
  , HostSupportsSVE256{ctx->HostFeatures.SupportsAVX}
  , CTX {ctx} {

  RAPass = Thread->PassManager->GetPass<IR::RegisterAllocationPass>("RA");

  RAPass->AllocateRegisterSet(RegisterClasses);

  RAPass->AddRegisters(FEXCore::IR::GPRClass, GeneralRegisters.size());
  RAPass->AddRegisters(FEXCore::IR::GPRFixedClass, StaticRegisters.size());
  RAPass->AddRegisters(FEXCore::IR::FPRClass, GeneralFPRegisters.size());
  RAPass->AddRegisters(FEXCore::IR::FPRFixedClass, StaticFPRegisters.size());
  RAPass->AddRegisters(FEXCore::IR::GPRPairClass, GeneralPairRegisters.size());
  RAPass->AddRegisters(FEXCore::IR::ComplexClass, 1);

  for (uint32_t i = 0; i < GeneralPairRegisters.size(); ++i) {
    RAPass->AddRegisterConflict(FEXCore::IR::GPRClass, i * 2,     FEXCore::IR::GPRPairClass, i);
    RAPass->AddRegisterConflict(FEXCore::IR::GPRClass, i * 2 + 1, FEXCore::IR::GPRPairClass, i);
  }

  {
    // Set up pointers that the JIT needs to load

    // Common
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
    Common.ExitFunctionLink = reinterpret_cast<uintptr_t>(&Context::ContextImpl::ThreadExitFunctionLink<Arm64JITCore_ExitFunctionLink>);

    // Fill in the fallback handlers
    InterpreterOps::FillFallbackIndexPointers(Common.FallbackHandlerPointers);

    // Platform Specific
    auto &AArch64 = ThreadState->CurrentFrame->Pointers.AArch64;

    AArch64.LUDIV = reinterpret_cast<uint64_t>(LUDIV);
    AArch64.LDIV = reinterpret_cast<uint64_t>(LDIV);
    AArch64.LUREM = reinterpret_cast<uint64_t>(LUREM);
    AArch64.LREM = reinterpret_cast<uint64_t>(LREM);
  }

  // Must be done after Dispatcher init
  ClearCache();

  // Setup dynamic dispatch.
  if (CTX->Dispatcher->GetConfig().StaticRegisterAllocation) {
    RT_LoadRegister = &Arm64JITCore::Op_LoadRegisterSRA;
    RT_StoreRegister = &Arm64JITCore::Op_StoreRegisterSRA;
  }
  else {
    RT_LoadRegister = &Arm64JITCore::Op_LoadRegister;
    RT_StoreRegister = &Arm64JITCore::Op_StoreRegister;
  }

  if (ParanoidTSO()) {
    RT_LoadMemTSO = &Arm64JITCore::Op_ParanoidLoadMemTSO;
    RT_StoreMemTSO = &Arm64JITCore::Op_ParanoidStoreMemTSO;
  }
  else {
    RT_LoadMemTSO = &Arm64JITCore::Op_LoadMemTSO;
    RT_StoreMemTSO = &Arm64JITCore::Op_StoreMemTSO;
  }
}

void Arm64JITCore::EmitDetectionString() {
  const char JITString[] = "FEXJIT::Arm64JITCore::";
  EmitString(JITString);
  Align();
}

void Arm64JITCore::ClearCache() {
  // Get the backing code buffer

  auto CodeBuffer = GetEmptyCodeBuffer();
  SetBuffer(CodeBuffer->Ptr, CodeBuffer->Size);
  EmitDetectionString();
}

Arm64JITCore::~Arm64JITCore() {

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

bool Arm64JITCore::IsGPRPair(IR::NodeID Node) const {
  auto Class = GetRegClass(Node);

  return Class == IR::GPRPairClass;
}

CPUBackend::CompiledCode Arm64JITCore::CompileCode(uint64_t Entry,
                                FEXCore::IR::IRListView const *IR,
                                FEXCore::Core::DebugData *DebugData,
                                FEXCore::IR::RegisterAllocationData *RAData,
                                bool GDBEnabled) {
  FEXCORE_PROFILE_SCOPED("Arm64::CompileCode");

  JumpTargets.clear();
  uint32_t SSACount = IR->GetSSACount();

  this->Entry = Entry;
  this->RAData = RAData;
  this->DebugData = DebugData;
  this->IR = IR;

  // Fairly excessive buffer range to make sure we don't overflow
  uint32_t BufferRange = SSACount * 16 + GDBEnabled * Dispatcher::MaxGDBPauseCheckSize;
  if ((GetCursorOffset() + BufferRange) > CurrentCodeBuffer->Size) {
    CTX->ClearCodeCache(ThreadState);
  }

  CodeData.BlockBegin = GetCursorAddress<uint8_t*>();

  // Put the code header at the start of the data block.
  ARMEmitter::BackwardLabel JITCodeHeaderLabel{};
  Bind(&JITCodeHeaderLabel);
  JITCodeHeader *CodeHeader = GetCursorAddress<JITCodeHeader *>();
  CursorIncrement(sizeof(JITCodeHeader));

#ifdef VIXL_DISASSEMBLER
  const auto DisasmBegin = GetCursorAddress<const vixl::aarch64::Instruction*>();
#endif

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

  CodeData.BlockEntry = GetCursorAddress<uint8_t*>();

  // Get the address of the JITCodeHeader and store in to the core state.
  // Two instruction cost, each 1 cycle.
  adr(TMP1, &JITCodeHeaderLabel);
  str(TMP1, STATE, offsetof(FEXCore::Core::CPUState, InlineJITBlockHeader));

#ifdef _WIN32
  // Trigger a fault if there are any pending interrupts
  // Used only for suspend on WIN32 at the moment
  strb(ARMEmitter::XReg::zr, STATE, offsetof(FEXCore::Core::InternalThreadState, InterruptFaultPage) -
                                    offsetof(FEXCore::Core::InternalThreadState, BaseFrameState));
#endif

  if (GDBEnabled) {
    auto GDBSize = CTX->Dispatcher->GenerateGDBPauseCheck(CodeData.BlockEntry, Entry);
    CursorIncrement(GDBSize);
  }

  //LOGMAN_THROW_A_FMT(RAData->HasFullRA(), "Arm64 JIT only works with RA");

  SpillSlots = RAData->SpillSlots();

  if (SpillSlots) {
    const auto TotalSpillSlotsSize = SpillSlots * MaxSpillSlotSize;

    if (vixl::aarch64::Assembler::IsImmAddSub(TotalSpillSlotsSize)) {
      sub(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::rsp, ARMEmitter::Reg::rsp, TotalSpillSlotsSize);
    } else {
      LoadConstant(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r0, TotalSpillSlotsSize);
      sub(ARMEmitter::Size::i64Bit, ARMEmitter::XReg::rsp, ARMEmitter::XReg::rsp, ARMEmitter::XReg::x0, ARMEmitter::ExtendedType::LSL_64, 0);
    }
  }

  PendingTargetLabel = nullptr;

  for (auto [BlockNode, BlockHeader] : IR->GetBlocks()) {
    using namespace FEXCore::IR;
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
    auto BlockIROp = BlockHeader->CW<FEXCore::IR::IROp_CodeBlock>();
    LOGMAN_THROW_AA_FMT(BlockIROp->Header.Op == IR::OP_CODEBLOCK, "IR type failed to be a code block");
#endif

    auto BlockStartHostCode = GetCursorAddress<uint8_t *>();
    {
      const auto Node = IR->GetID(BlockNode);
      const auto IsTarget = JumpTargets.try_emplace(Node).first;

      // if there's a pending branch, and it is not fall-through
      if (PendingTargetLabel && PendingTargetLabel != &IsTarget->second)
      {
        b(PendingTargetLabel);
      }
      PendingTargetLabel = nullptr;

      Bind(&IsTarget->second);
    }

    for (auto [CodeNode, IROp] : IR->GetCode(BlockNode)) {
      const auto ID = IR->GetID(CodeNode);
      switch (IROp->Op) {
#define REGISTER_OP_RT(op, x) case FEXCore::IR::IROps::OP_##op: std::invoke(RT_##x, this, IROp, ID); break
#define REGISTER_OP(op, x) case FEXCore::IR::IROps::OP_##op: Op_##x(IROp, ID); break
        // ALU ops
        REGISTER_OP(TRUNCELEMENTPAIR,  TruncElementPair);
        REGISTER_OP(CONSTANT,          Constant);
        REGISTER_OP(ENTRYPOINTOFFSET,  EntrypointOffset);
        REGISTER_OP(INLINECONSTANT,    InlineConstant);
        REGISTER_OP(INLINEENTRYPOINTOFFSET,  InlineEntrypointOffset);
        REGISTER_OP(CYCLECOUNTER,      CycleCounter);
        REGISTER_OP(ADD,               Add);
        REGISTER_OP(ADDNZCV,           AddNZCV);
        REGISTER_OP(TESTNZ,            TestNZ);
        REGISTER_OP(SUB,               Sub);
        REGISTER_OP(SUBNZCV,           SubNZCV);
        REGISTER_OP(NEG,               Neg);
        REGISTER_OP(ABS,               Abs);
        REGISTER_OP(MUL,               Mul);
        REGISTER_OP(UMUL,              UMul);
        REGISTER_OP(DIV,               Div);
        REGISTER_OP(UDIV,              UDiv);
        REGISTER_OP(REM,               Rem);
        REGISTER_OP(UREM,              URem);
        REGISTER_OP(MULH,              MulH);
        REGISTER_OP(UMULH,             UMulH);
        REGISTER_OP(OR,                Or);
        REGISTER_OP(ORLSHL,            Orlshl);
        REGISTER_OP(ORLSHR,            Orlshr);
        REGISTER_OP(ORNROR,            Ornror);
        REGISTER_OP(AND,               And);
        REGISTER_OP(ANDN,              Andn);
        REGISTER_OP(XOR,               Xor);
        REGISTER_OP(LSHL,              Lshl);
        REGISTER_OP(LSHR,              Lshr);
        REGISTER_OP(ASHR,              Ashr);
        REGISTER_OP(ROR,               Ror);
        REGISTER_OP(EXTR,              Extr);
        REGISTER_OP(PDEP,              PDep);
        REGISTER_OP(PEXT,              PExt);
        REGISTER_OP(LDIV,              LDiv);
        REGISTER_OP(LUDIV,             LUDiv);
        REGISTER_OP(LREM,              LRem);
        REGISTER_OP(LUREM,             LURem);
        REGISTER_OP(NOT,               Not);
        REGISTER_OP(POPCOUNT,          Popcount);
        REGISTER_OP(FINDLSB,           FindLSB);
        REGISTER_OP(FINDMSB,           FindMSB);
        REGISTER_OP(FINDTRAILINGZEROES, FindTrailingZeroes);
        REGISTER_OP(COUNTLEADINGZEROES, CountLeadingZeroes);
        REGISTER_OP(REV,               Rev);
        REGISTER_OP(BFI,               Bfi);
        REGISTER_OP(BFXIL,             Bfxil);
        REGISTER_OP(BFE,               Bfe);
        REGISTER_OP(SBFE,              Sbfe);
        REGISTER_OP(SELECT,            Select);
        REGISTER_OP(VEXTRACTTOGPR,     VExtractToGPR);
        REGISTER_OP(FLOAT_TOGPR_ZS,    Float_ToGPR_ZS);
        REGISTER_OP(FLOAT_TOGPR_S,     Float_ToGPR_S);
        REGISTER_OP(FCMP,              FCmp);

        // Atomic ops
        REGISTER_OP(CASPAIR,        CASPair);
        REGISTER_OP(CAS,            CAS);
        REGISTER_OP(ATOMICADD,      AtomicAdd);
        REGISTER_OP(ATOMICSUB,      AtomicSub);
        REGISTER_OP(ATOMICAND,      AtomicAnd);
        REGISTER_OP(ATOMICOR,       AtomicOr);
        REGISTER_OP(ATOMICXOR,      AtomicXor);
        REGISTER_OP(ATOMICSWAP,     AtomicSwap);
        REGISTER_OP(ATOMICFETCHADD, AtomicFetchAdd);
        REGISTER_OP(ATOMICFETCHSUB, AtomicFetchSub);
        REGISTER_OP(ATOMICFETCHAND, AtomicFetchAnd);
        REGISTER_OP(ATOMICFETCHCLR, AtomicFetchCLR);
        REGISTER_OP(ATOMICFETCHOR,  AtomicFetchOr);
        REGISTER_OP(ATOMICFETCHXOR, AtomicFetchXor);
        REGISTER_OP(ATOMICFETCHNEG, AtomicFetchNeg);
        REGISTER_OP(TELEMETRYSETVALUE, TelemetrySetValue);

        // Branch ops
        REGISTER_OP(CALLBACKRETURN,    CallbackReturn);
        REGISTER_OP(EXITFUNCTION,      ExitFunction);
        REGISTER_OP(JUMP,              Jump);
        REGISTER_OP(CONDJUMP,          CondJump);
        REGISTER_OP(SYSCALL,           Syscall);
        REGISTER_OP(INLINESYSCALL,     InlineSyscall);
        REGISTER_OP(THUNK,             Thunk);
        REGISTER_OP(VALIDATECODE,      ValidateCode);
        REGISTER_OP(THREADREMOVECODEENTRY,   ThreadRemoveCodeEntry);
        REGISTER_OP(CPUID,             CPUID);
        REGISTER_OP(XGETBV,            XGETBV);

        // Conversion ops
        REGISTER_OP(VINSGPR,         VInsGPR);
        REGISTER_OP(VCASTFROMGPR,    VCastFromGPR);
        REGISTER_OP(VDUPFROMGPR,     VDupFromGPR);
        REGISTER_OP(FLOAT_FROMGPR_S, Float_FromGPR_S);
        REGISTER_OP(FLOAT_FTOF,      Float_FToF);
        REGISTER_OP(VECTOR_STOF,     Vector_SToF);
        REGISTER_OP(VECTOR_FTOZS,    Vector_FToZS);
        REGISTER_OP(VECTOR_FTOS,     Vector_FToS);
        REGISTER_OP(VECTOR_FTOF,     Vector_FToF);
        REGISTER_OP(VECTOR_FTOI,     Vector_FToI);

        // Encryption ops
        REGISTER_OP(VAESIMC,           AESImc);
        REGISTER_OP(VAESENC,           AESEnc);
        REGISTER_OP(VAESENCLAST,       AESEncLast);
        REGISTER_OP(VAESDEC,           AESDec);
        REGISTER_OP(VAESDECLAST,       AESDecLast);
        REGISTER_OP(VAESKEYGENASSIST,  AESKeyGenAssist);
        REGISTER_OP(CRC32,             CRC32);
        REGISTER_OP(PCLMUL,            PCLMUL);

        // Flag ops
        REGISTER_OP(GETHOSTFLAG, GetHostFlag);

        // Memory ops
        REGISTER_OP(LOADCONTEXT,         LoadContext);
        REGISTER_OP(STORECONTEXT,        StoreContext);
        REGISTER_OP_RT(LOADREGISTER,     LoadRegister);
        REGISTER_OP_RT(STOREREGISTER,    StoreRegister);
        REGISTER_OP(LOADCONTEXTINDEXED,  LoadContextIndexed);
        REGISTER_OP(STORECONTEXTINDEXED, StoreContextIndexed);
        REGISTER_OP(SPILLREGISTER,       SpillRegister);
        REGISTER_OP(FILLREGISTER,        FillRegister);
        REGISTER_OP(LOADFLAG,            LoadFlag);
        REGISTER_OP(STOREFLAG,           StoreFlag);
        REGISTER_OP(LOADMEM,             LoadMem);
        REGISTER_OP(STOREMEM,            StoreMem);
        REGISTER_OP_RT(LOADMEMTSO,       LoadMemTSO);
        REGISTER_OP_RT(STOREMEMTSO,      StoreMemTSO);
        REGISTER_OP(VLOADVECTORMASKED,   VLoadVectorMasked);
        REGISTER_OP(VSTOREVECTORMASKED,  VStoreVectorMasked);
        REGISTER_OP(VLOADVECTORELEMENT,  VLoadVectorElement);
        REGISTER_OP(VSTOREVECTORELEMENT, VStoreVectorElement);
        REGISTER_OP(VBROADCASTFROMMEM,   VBroadcastFromMem);
        REGISTER_OP(PUSH,                Push);
        REGISTER_OP(MEMSET,              MemSet);
        REGISTER_OP(MEMCPY,              MemCpy);
        REGISTER_OP(CACHELINECLEAR,      CacheLineClear);
        REGISTER_OP(CACHELINECLEAN,      CacheLineClean);
        REGISTER_OP(CACHELINEZERO,       CacheLineZero);

        // Misc ops
        REGISTER_OP(DUMMY,      NoOp);
        REGISTER_OP(IRHEADER,   NoOp);
        REGISTER_OP(CODEBLOCK,  NoOp);
        REGISTER_OP(BEGINBLOCK, NoOp);
        REGISTER_OP(ENDBLOCK,   NoOp);
        REGISTER_OP(GUESTOPCODE, GuestOpcode);
        REGISTER_OP(FENCE,      Fence);
        REGISTER_OP(BREAK,      Break);
        REGISTER_OP(PRINT,      Print);
        REGISTER_OP(GETROUNDINGMODE, GetRoundingMode);
        REGISTER_OP(SETROUNDINGMODE, SetRoundingMode);
        REGISTER_OP(INVALIDATEFLAGS,   NoOp);
        REGISTER_OP(PROCESSORID,   ProcessorID);
        REGISTER_OP(RDRAND, RDRAND);
        REGISTER_OP(YIELD, Yield);

        // Move ops
        REGISTER_OP(EXTRACTELEMENTPAIR, ExtractElementPair);
        REGISTER_OP(CREATEELEMENTPAIR,  CreateElementPair);

        // Vector ops
        REGISTER_OP(VECTORZERO,        VectorZero);
        REGISTER_OP(VECTORIMM,         VectorImm);
        REGISTER_OP(LOADNAMEDVECTORCONSTANT, LoadNamedVectorConstant);
        REGISTER_OP(LOADNAMEDVECTORINDEXEDCONSTANT, LoadNamedVectorIndexedConstant);
        REGISTER_OP(VMOV,              VMov);
        REGISTER_OP(VAND,              VAnd);
        REGISTER_OP(VBIC,              VBic);
        REGISTER_OP(VOR,               VOr);
        REGISTER_OP(VXOR,              VXor);
        REGISTER_OP(VADD,              VAdd);
        REGISTER_OP(VSUB,              VSub);
        REGISTER_OP(VUQADD,            VUQAdd);
        REGISTER_OP(VUQSUB,            VUQSub);
        REGISTER_OP(VSQADD,            VSQAdd);
        REGISTER_OP(VSQSUB,            VSQSub);
        REGISTER_OP(VADDP,             VAddP);
        REGISTER_OP(VADDV,             VAddV);
        REGISTER_OP(VUMINV,            VUMinV);
        REGISTER_OP(VURAVG,            VURAvg);
        REGISTER_OP(VABS,              VAbs);
        REGISTER_OP(VPOPCOUNT,         VPopcount);
        REGISTER_OP(VFADD,             VFAdd);
        REGISTER_OP(VFADDP,            VFAddP);
        REGISTER_OP(VFSUB,             VFSub);
        REGISTER_OP(VFMUL,             VFMul);
        REGISTER_OP(VFDIV,             VFDiv);
        REGISTER_OP(VFMIN,             VFMin);
        REGISTER_OP(VFMAX,             VFMax);
        REGISTER_OP(VFRECP,            VFRecp);
        REGISTER_OP(VFSQRT,            VFSqrt);
        REGISTER_OP(VFRSQRT,           VFRSqrt);
        REGISTER_OP(VNEG,              VNeg);
        REGISTER_OP(VFNEG,             VFNeg);
        REGISTER_OP(VNOT,              VNot);
        REGISTER_OP(VUMIN,             VUMin);
        REGISTER_OP(VSMIN,             VSMin);
        REGISTER_OP(VUMAX,             VUMax);
        REGISTER_OP(VSMAX,             VSMax);
        REGISTER_OP(VZIP,              VZip);
        REGISTER_OP(VZIP2,             VZip2);
        REGISTER_OP(VUNZIP,            VUnZip);
        REGISTER_OP(VUNZIP2,           VUnZip2);
        REGISTER_OP(VTRN,              VTrn);
        REGISTER_OP(VTRN2,             VTrn2);
        REGISTER_OP(VBSL,              VBSL);
        REGISTER_OP(VCMPEQ,            VCMPEQ);
        REGISTER_OP(VCMPEQZ,           VCMPEQZ);
        REGISTER_OP(VCMPGT,            VCMPGT);
        REGISTER_OP(VCMPGTZ,           VCMPGTZ);
        REGISTER_OP(VCMPLTZ,           VCMPLTZ);
        REGISTER_OP(VFCMPEQ,           VFCMPEQ);
        REGISTER_OP(VFCMPNEQ,          VFCMPNEQ);
        REGISTER_OP(VFCMPLT,           VFCMPLT);
        REGISTER_OP(VFCMPGT,           VFCMPGT);
        REGISTER_OP(VFCMPLE,           VFCMPLE);
        REGISTER_OP(VFCMPORD,          VFCMPORD);
        REGISTER_OP(VFCMPUNO,          VFCMPUNO);
        REGISTER_OP(VUSHL,             VUShl);
        REGISTER_OP(VUSHR,             VUShr);
        REGISTER_OP(VSSHR,             VSShr);
        REGISTER_OP(VUSHLS,            VUShlS);
        REGISTER_OP(VUSHRS,            VUShrS);
        REGISTER_OP(VUSHRSWIDE,        VUShrSWide);
        REGISTER_OP(VSSHRSWIDE,        VSShrSWide);
        REGISTER_OP(VUSHLSWIDE,        VUShlSWide);
        REGISTER_OP(VSSHRS,            VSShrS);
        REGISTER_OP(VINSELEMENT,       VInsElement);
        REGISTER_OP(VDUPELEMENT,       VDupElement);
        REGISTER_OP(VEXTR,             VExtr);
        REGISTER_OP(VUSHRI,            VUShrI);
        REGISTER_OP(VSSHRI,            VSShrI);
        REGISTER_OP(VSHLI,             VShlI);
        REGISTER_OP(VUSHRNI,           VUShrNI);
        REGISTER_OP(VUSHRNI2,          VUShrNI2);
        REGISTER_OP(VSXTL,             VSXTL);
        REGISTER_OP(VSXTL2,            VSXTL2);
        REGISTER_OP(VUXTL,             VUXTL);
        REGISTER_OP(VUXTL2,            VUXTL2);
        REGISTER_OP(VSQXTN,            VSQXTN);
        REGISTER_OP(VSQXTN2,           VSQXTN2);
        REGISTER_OP(VSQXTNPAIR,        VSQXTNPair);
        REGISTER_OP(VSQXTUN,           VSQXTUN);
        REGISTER_OP(VSQXTUN2,          VSQXTUN2);
        REGISTER_OP(VSQXTUNPAIR,       VSQXTUNPair);
        REGISTER_OP(VSRSHR,            VSRSHR);
        REGISTER_OP(VSQSHL,            VSQSHL);
        REGISTER_OP(VUMUL,             VMul);
        REGISTER_OP(VSMUL,             VMul);
        REGISTER_OP(VUMULL,            VUMull);
        REGISTER_OP(VSMULL,            VSMull);
        REGISTER_OP(VUMULL2,           VUMull2);
        REGISTER_OP(VSMULL2,           VSMull2);
        REGISTER_OP(VUMULH,            VUMulH);
        REGISTER_OP(VSMULH,            VSMulH);
        REGISTER_OP(VUABDL,            VUABDL);
        REGISTER_OP(VUABDL2,           VUABDL2);
        REGISTER_OP(VTBL1,             VTBL1);
        REGISTER_OP(VTBL2,             VTBL2);
        REGISTER_OP(VREV32,            VRev32);
        REGISTER_OP(VREV64,            VRev64);
        REGISTER_OP(VFCADD,            VFCADD);
#undef REGISTER_OP

        default:
          Op_Unhandled(IROp, ID);
          break;
      }
    }

    if (DebugData) {
      DebugData->Subblocks.push_back({
        static_cast<uint32_t>(BlockStartHostCode - CodeData.BlockEntry),
        static_cast<uint32_t>(GetCursorAddress<uint8_t *>() - BlockStartHostCode)
      });
    }
  }

  // Make sure last branch is generated. It certainly can't be eliminated here.
  if (PendingTargetLabel)
  {
    b(PendingTargetLabel);
  }
  PendingTargetLabel = nullptr;

  // CodeSize not including the tail data.
  const uint64_t CodeOnlySize = GetCursorAddress<uint8_t *>() - CodeData.BlockBegin;

  // Add the JitCodeTail
  auto JITBlockTailLocation = GetCursorAddress<uint8_t *>();
  auto JITBlockTail = GetCursorAddress<JITCodeTail*>();
  CursorIncrement(sizeof(JITCodeTail));

  auto JITRIPEntriesLocation = GetCursorAddress<uint8_t *>();
  auto JITRIPEntries = GetCursorAddress<JITRIPReconstructEntries*>();

  CursorIncrement(sizeof(JITRIPReconstructEntries) * DebugData->GuestOpcodes.size());

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

  CodeData.Size = GetCursorAddress<uint8_t *>() - CodeData.BlockBegin;

  JITBlockTail->Size = CodeData.Size;

  ClearICache(CodeData.BlockBegin, CodeOnlySize);

#ifdef VIXL_DISASSEMBLER
  if (Disassemble() & FEXCore::Config::Disassemble::STATS) {
    auto HeaderOp = IR->GetHeader();
    LOGMAN_THROW_AA_FMT(HeaderOp->Header.Op == IR::OP_IRHEADER, "First op wasn't IRHeader");

    LogMan::Msg::IFmt("RIP: 0x{:x}", Entry);
    LogMan::Msg::IFmt("Guest Code instructions: {}", HeaderOp->NumHostInstructions);
    LogMan::Msg::IFmt("Host Code instructions: {}", CodeOnlySize >> 2);
    LogMan::Msg::IFmt("Blow-up Amt: {}x", double(CodeOnlySize >> 2) / double(HeaderOp->NumHostInstructions));
  }

  if (Disassemble() & FEXCore::Config::Disassemble::BLOCKS) {
    const auto DisasmEnd = reinterpret_cast<const vixl::aarch64::Instruction*>(JITBlockTailLocation);
    LogMan::Msg::IFmt("Disassemble Begin");
    for (auto PCToDecode = DisasmBegin; PCToDecode < DisasmEnd; PCToDecode += 4) {
      DisasmDecoder->Decode(PCToDecode);
      auto Output = Disasm.GetOutput();
      LogMan::Msg::IFmt("{}", Output);
    }
    LogMan::Msg::IFmt("Disassemble End");
  }
#endif

  if (DebugData) {
    DebugData->HostCodeSize = CodeData.Size;
    DebugData->Relocations = &Relocations;
  }

  this->IR = nullptr;

  return CodeData;
}

void Arm64JITCore::ResetStack() {
  if (SpillSlots == 0) {
    return;
  }

  const auto TotalSpillSlotsSize = SpillSlots * MaxSpillSlotSize;

  if (vixl::aarch64::Assembler::IsImmAddSub(TotalSpillSlotsSize)) {
    add(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::rsp, ARMEmitter::Reg::rsp, TotalSpillSlotsSize);
  } else {
    // Too big to fit in a 12bit immediate
    LoadConstant(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r0, TotalSpillSlotsSize);
    add(ARMEmitter::Size::i64Bit, ARMEmitter::XReg::rsp, ARMEmitter::XReg::rsp, ARMEmitter::XReg::x0, ARMEmitter::ExtendedType::LSL_64, 0);
  }
}

fextl::unique_ptr<CPUBackend> CreateArm64JITCore(FEXCore::Context::ContextImpl *ctx, FEXCore::Core::InternalThreadState *Thread) {
  return fextl::make_unique<Arm64JITCore>(ctx, Thread);
}

CPUBackendFeatures GetArm64JITBackendFeatures() {
  return CPUBackendFeatures {
    .SupportsStaticRegisterAllocation = true,
    .SupportsFlags = true,
    .SupportsSaturatingRoundingShifts = true,
    .SupportsVTBL2 = true,
  };
}

}
