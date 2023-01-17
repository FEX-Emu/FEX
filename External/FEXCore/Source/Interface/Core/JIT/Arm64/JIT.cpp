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
#include "Interface/Core/ArchHelpers/CodeEmitter/Emitter.h"
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
#include <FEXCore/Utils/EnumUtils.h>
#include <FEXCore/Utils/Profiler.h>

#include "Interface/Core/Interpreter/InterpreterOps.h"

#include <sys/mman.h>
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
      case FABI_VOID_U16:{
        SpillStaticRegs();

        PushDynamicRegsAndLR(TMP1);

        const auto Src1 = GetReg(IROp->Args[0].ID());
        uxth(ARMEmitter::Size::i32Bit, ARMEmitter::Reg::r0, Src1);
        ldr(ARMEmitter::XReg::x1, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex]));
#ifdef VIXL_SIMULATOR
        GenerateIndirectRuntimeCall<void, uint16_t>(ARMEmitter::Reg::r1);
#else
        blr(ARMEmitter::Reg::r1);
#endif

        PopDynamicRegsAndLR();

        FillStaticRegs();
      }
      break;

      case FABI_F80_F32:{
        SpillStaticRegs();

        PushDynamicRegsAndLR(TMP1);
        const auto Src1 = GetVReg(IROp->Args[0].ID());
        fmov(ARMEmitter::SReg::s0, Src1.S());
        ldr(ARMEmitter::XReg::x0, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex]));
#ifdef VIXL_SIMULATOR
        GenerateIndirectRuntimeCall<__uint128_t, float>(ARMEmitter::Reg::r0);
#else
        blr(ARMEmitter::Reg::r0);
#endif

        PopDynamicRegsAndLR();

        FillStaticRegs();

        const auto Dst = GetVReg(Node);
        eor(Dst.Q(), Dst.Q(), Dst.Q());
        ins(ARMEmitter::SubRegSize::i64Bit, Dst, 0, ARMEmitter::Reg::r0);
        ins(ARMEmitter::SubRegSize::i16Bit, Dst, 4, ARMEmitter::Reg::r1);
      }
      break;

      case FABI_F80_F64:{
        SpillStaticRegs();

        PushDynamicRegsAndLR(TMP1);

        const auto Src1 = GetVReg(IROp->Args[0].ID());
        mov(ARMEmitter::DReg::d0, Src1.D());
        ldr(ARMEmitter::XReg::x0, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex]));
#ifdef VIXL_SIMULATOR
        GenerateIndirectRuntimeCall<__uint128_t, double>(ARMEmitter::Reg::r0);
#else
        blr(ARMEmitter::Reg::r0);
#endif

        PopDynamicRegsAndLR();

        FillStaticRegs();

        const auto Dst = GetVReg(Node);
        eor(Dst.Q(), Dst.Q(), Dst.Q());
        ins(ARMEmitter::SubRegSize::i64Bit, Dst, 0, ARMEmitter::Reg::r0);
        ins(ARMEmitter::SubRegSize::i16Bit, Dst, 4, ARMEmitter::Reg::r1);
      }
      break;

      case FABI_F80_I16:
      case FABI_F80_I32: {
        SpillStaticRegs();

        PushDynamicRegsAndLR(TMP1);

        const auto Src1 = GetReg(IROp->Args[0].ID());
        if (Info.ABI == FABI_F80_I16) {
          sxth(ARMEmitter::Size::i32Bit, ARMEmitter::Reg::r0, Src1);
        }
        else {
          mov(ARMEmitter::Size::i32Bit, ARMEmitter::Reg::r0, Src1);
        }
        ldr(ARMEmitter::XReg::x1, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex]));
#ifdef VIXL_SIMULATOR
        GenerateIndirectRuntimeCall<__uint128_t, uint32_t>(ARMEmitter::Reg::r1);
#else
        blr(ARMEmitter::Reg::r1);
#endif

        PopDynamicRegsAndLR();

        FillStaticRegs();

        const auto Dst = GetVReg(Node);
        eor(Dst.Q(), Dst.Q(), Dst.Q());
        ins(ARMEmitter::SubRegSize::i64Bit, Dst, 0, ARMEmitter::Reg::r0);
        ins(ARMEmitter::SubRegSize::i16Bit, Dst, 4, ARMEmitter::Reg::r1);
      }
      break;

      case FABI_F32_F80:{
        SpillStaticRegs();

        PushDynamicRegsAndLR(TMP1);

        const auto Src1 = GetVReg(IROp->Args[0].ID());

        umov<ARMEmitter::SubRegSize::i64Bit>(ARMEmitter::Reg::r0, Src1, 0);
        umov<ARMEmitter::SubRegSize::i16Bit>(ARMEmitter::Reg::r1, Src1, 4);

        ldr(ARMEmitter::XReg::x2, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex]));
#ifdef VIXL_SIMULATOR
        GenerateIndirectRuntimeCall<float, uint64_t, uint64_t>(ARMEmitter::Reg::r2);
#else
        blr(ARMEmitter::Reg::r2);
#endif

        PopDynamicRegsAndLR();

        FillStaticRegs();

        const auto Dst = GetVReg(Node);
        fmov(Dst.S(), ARMEmitter::SReg::s0);
      }
      break;

      case FABI_F64_F80:{
        SpillStaticRegs();

        PushDynamicRegsAndLR(TMP1);

        const auto Src1 = GetVReg(IROp->Args[0].ID());

        umov<ARMEmitter::SubRegSize::i64Bit>(ARMEmitter::Reg::r0, Src1, 0);
        umov<ARMEmitter::SubRegSize::i16Bit>(ARMEmitter::Reg::r1, Src1, 4);

        ldr(ARMEmitter::XReg::x2, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex]));
#ifdef VIXL_SIMULATOR
        GenerateIndirectRuntimeCall<double, uint64_t, uint64_t>(ARMEmitter::Reg::r2);
#else
        blr(ARMEmitter::Reg::r2);
#endif

        PopDynamicRegsAndLR();

        FillStaticRegs();

        const auto Dst = GetVReg(Node);
        mov(Dst.D(), ARMEmitter::DReg::d0);
      }
      break;

      case FABI_F64_F64: {
        SpillStaticRegs();

        PushDynamicRegsAndLR(TMP1);

        const auto Src1 = GetVReg(IROp->Args[0].ID());

        mov(ARMEmitter::DReg::d0, Src1.D());
        ldr(ARMEmitter::XReg::x0, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex]));
#ifdef VIXL_SIMULATOR
        GenerateIndirectRuntimeCall<double, double>(ARMEmitter::Reg::r0);
#else
        blr(ARMEmitter::Reg::r0);
#endif

        PopDynamicRegsAndLR();

        FillStaticRegs();

        const auto Dst = GetVReg(Node);
        mov(Dst.D(), ARMEmitter::DReg::d0);
      }
      break;

      case FABI_F64_F64_F64: {
        SpillStaticRegs();

        PushDynamicRegsAndLR(TMP1);

        const auto Src1 = GetVReg(IROp->Args[0].ID());
        const auto Src2 = GetVReg(IROp->Args[1].ID());

        mov(ARMEmitter::DReg::d0, Src1.D());
        mov(ARMEmitter::DReg::d1, Src2.D());
        ldr(ARMEmitter::XReg::x0, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex]));
#ifdef VIXL_SIMULATOR
        GenerateIndirectRuntimeCall<double, double, double>(ARMEmitter::Reg::r0);
#else
        blr(ARMEmitter::Reg::r0);
#endif

        PopDynamicRegsAndLR();

        FillStaticRegs();

        const auto Dst = GetVReg(Node);
        mov(Dst.D(), ARMEmitter::DReg::d0);
      }
      break;

      case FABI_I16_F80:{
        SpillStaticRegs();

        PushDynamicRegsAndLR(TMP1);

        const auto Src1 = GetVReg(IROp->Args[0].ID());

        umov<ARMEmitter::SubRegSize::i64Bit>(ARMEmitter::Reg::r0, Src1, 0);
        umov<ARMEmitter::SubRegSize::i16Bit>(ARMEmitter::Reg::r1, Src1, 4);

        ldr(ARMEmitter::XReg::x2, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex]));
#ifdef VIXL_SIMULATOR
        GenerateIndirectRuntimeCall<uint32_t, uint64_t, uint64_t>(ARMEmitter::Reg::r2);
#else
        blr(ARMEmitter::Reg::r2);
#endif

        PopDynamicRegsAndLR();

        FillStaticRegs();

        const auto Dst = GetReg(Node);
        sxth(ARMEmitter::Size::i64Bit, Dst, ARMEmitter::Reg::r0);
      }
      break;
      case FABI_I32_F80:{
        SpillStaticRegs();

        PushDynamicRegsAndLR(TMP1);

        const auto Src1 = GetVReg(IROp->Args[0].ID());

        umov<ARMEmitter::SubRegSize::i64Bit>(ARMEmitter::Reg::r0, Src1, 0);
        umov<ARMEmitter::SubRegSize::i16Bit>(ARMEmitter::Reg::r1, Src1, 4);

        ldr(ARMEmitter::XReg::x2, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex]));
#ifdef VIXL_SIMULATOR
        GenerateIndirectRuntimeCall<uint32_t, uint64_t, uint64_t>(ARMEmitter::Reg::r2);
#else
        blr(ARMEmitter::Reg::r2);
#endif

        PopDynamicRegsAndLR();

        FillStaticRegs();

        const auto Dst = GetReg(Node);
        mov(ARMEmitter::Size::i32Bit, Dst, ARMEmitter::Reg::r0);
      }
      break;
      case FABI_I64_F80:{
        SpillStaticRegs();

        PushDynamicRegsAndLR(TMP1);

        const auto Src1 = GetVReg(IROp->Args[0].ID());

        umov<ARMEmitter::SubRegSize::i64Bit>(ARMEmitter::Reg::r0, Src1, 0);
        umov<ARMEmitter::SubRegSize::i16Bit>(ARMEmitter::Reg::r1, Src1, 4);

        ldr(ARMEmitter::XReg::x2, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex]));
#ifdef VIXL_SIMULATOR
        GenerateIndirectRuntimeCall<uint64_t, uint64_t, uint64_t>(ARMEmitter::Reg::r2);
#else
        blr(ARMEmitter::Reg::r2);
#endif

        PopDynamicRegsAndLR();

        FillStaticRegs();

        const auto Dst = GetReg(Node);
        mov(ARMEmitter::Size::i64Bit, Dst, ARMEmitter::Reg::r0);
      }
      break;
      case FABI_I64_F80_F80:{
        SpillStaticRegs();

        PushDynamicRegsAndLR(TMP1);

        const auto Src1 = GetVReg(IROp->Args[0].ID());
        const auto Src2 = GetVReg(IROp->Args[1].ID());

        umov<ARMEmitter::SubRegSize::i64Bit>(ARMEmitter::Reg::r0, Src1, 0);
        umov<ARMEmitter::SubRegSize::i16Bit>(ARMEmitter::Reg::r1, Src1, 4);

        umov<ARMEmitter::SubRegSize::i64Bit>(ARMEmitter::Reg::r2, Src2, 0);
        umov<ARMEmitter::SubRegSize::i16Bit>(ARMEmitter::Reg::r3, Src2, 4);

        ldr(ARMEmitter::XReg::x4, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex]));
#ifdef VIXL_SIMULATOR
        GenerateIndirectRuntimeCall<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>(ARMEmitter::Reg::r4);
#else
        blr(ARMEmitter::Reg::r4);
#endif
        PopDynamicRegsAndLR();

        FillStaticRegs();

        const auto Dst = GetReg(Node);
        mov(ARMEmitter::Size::i64Bit, Dst, ARMEmitter::Reg::r0);
      }
      break;
      case FABI_F80_F80:{
        SpillStaticRegs();

        PushDynamicRegsAndLR(TMP1);

        const auto Src1 = GetVReg(IROp->Args[0].ID());

        umov<ARMEmitter::SubRegSize::i64Bit>(ARMEmitter::Reg::r0, Src1, 0);
        umov<ARMEmitter::SubRegSize::i16Bit>(ARMEmitter::Reg::r1, Src1, 4);

        ldr(ARMEmitter::XReg::x2, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex]));
#ifdef VIXL_SIMULATOR
        GenerateIndirectRuntimeCall<__uint128_t, uint64_t, uint64_t>(ARMEmitter::Reg::r2);
#else
        blr(ARMEmitter::Reg::r2);
#endif

        PopDynamicRegsAndLR();

        FillStaticRegs();

        const auto Dst = GetVReg(Node);
        eor(Dst.Q(), Dst.Q(), Dst.Q());
        ins(ARMEmitter::SubRegSize::i64Bit, Dst, 0, ARMEmitter::Reg::r0);
        ins(ARMEmitter::SubRegSize::i16Bit, Dst, 4, ARMEmitter::Reg::r1);
      }
      break;
      case FABI_F80_F80_F80:{
        SpillStaticRegs();

        PushDynamicRegsAndLR(TMP1);

        const auto Src1 = GetVReg(IROp->Args[0].ID());
        const auto Src2 = GetVReg(IROp->Args[1].ID());

        umov<ARMEmitter::SubRegSize::i64Bit>(ARMEmitter::Reg::r0, Src1, 0);
        umov<ARMEmitter::SubRegSize::i16Bit>(ARMEmitter::Reg::r1, Src1, 4);

        umov<ARMEmitter::SubRegSize::i64Bit>(ARMEmitter::Reg::r2, Src2, 0);
        umov<ARMEmitter::SubRegSize::i16Bit>(ARMEmitter::Reg::r3, Src2, 4);

        ldr(ARMEmitter::XReg::x4, STATE_PTR(CpuStateFrame, Pointers.Common.FallbackHandlerPointers[Info.HandlerIndex]));
#ifdef VIXL_SIMULATOR
        GenerateIndirectRuntimeCall<__uint128_t, uint64_t, uint64_t, uint64_t, uint64_t>(ARMEmitter::Reg::r4);
#else
        blr(ARMEmitter::Reg::r4);
#endif

        PopDynamicRegsAndLR();

        FillStaticRegs();

        const auto Dst = GetVReg(Node);
        eor(Dst.Q(), Dst.Q(), Dst.Q());
        ins(ARMEmitter::SubRegSize::i64Bit, Dst, 0, ARMEmitter::Reg::r0);
        ins(ARMEmitter::SubRegSize::i16Bit, Dst, 4, ARMEmitter::Reg::r1);
      }
      break;
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
    Context::Context::ThreadAddBlockLink(Thread, GuestRip, (uintptr_t)record, [branch, LinkerAddress]{
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
    Context::Context::ThreadAddBlockLink(Thread, GuestRip, (uintptr_t)record, [record, LinkerAddress]{
      record[0] = LinkerAddress;
    });
  }

  return HostCode;
}

void Arm64JITCore::Op_NoOp(IR::IROp_Header const *IROp, IR::NodeID Node) {
}

Arm64JITCore::Arm64JITCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread)
  : CPUBackend(Thread, INITIAL_CODE_SIZE, MAX_CODE_SIZE)
  , Arm64Emitter(ctx, 0)
  , HostSupportsSVE{ctx->HostFeatures.SupportsAVX}
  , CTX {ctx} {

  RAPass = Thread->PassManager->GetPass<IR::RegisterAllocationPass>("RA");

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

  {
    // Set up pointers that the JIT needs to load

    // Common
    auto &Common = ThreadState->CurrentFrame->Pointers.Common;

    Common.PrintValue = reinterpret_cast<uint64_t>(PrintValue);
    Common.PrintVectorValue = reinterpret_cast<uint64_t>(PrintVectorValue);
    Common.ThreadRemoveCodeEntryFromJIT = reinterpret_cast<uintptr_t>(&Context::Context::ThreadRemoveCodeEntryFromJit);
    Common.CPUIDObj = reinterpret_cast<uint64_t>(&CTX->CPUID);

    {
      FEXCore::Utils::MemberFunctionToPointerCast PMF(&FEXCore::CPUIDEmu::RunFunction);
      Common.CPUIDFunction = PMF.GetConvertedPointer();
    }

    Common.SyscallHandlerObj = reinterpret_cast<uint64_t>(CTX->SyscallHandler);
    Common.SyscallHandlerFunc = reinterpret_cast<uint64_t>(FEXCore::Context::HandleSyscall);
    Common.ExitFunctionLink = reinterpret_cast<uintptr_t>(&Context::Context::ThreadExitFunctionLink<Arm64JITCore_ExitFunctionLink>);


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
}

void Arm64JITCore::InitializeSignalHandlers(FEXCore::Context::Context *CTX) {
  CTX->SignalDelegation->RegisterHostSignalHandler(SIGILL, [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) -> bool {
    return Thread->CTX->Dispatcher->HandleSIGILL(Thread, Signal, info, ucontext);
  }, true);

#ifdef _M_ARM_64
  CTX->SignalDelegation->RegisterHostSignalHandler(SIGBUS, [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) -> bool {
    if (!Thread->CPUBackend->IsAddressInCodeBuffer(ArchHelpers::Context::GetPc(ucontext))) {
      // Wasn't a sigbus in JIT code
      return false;
    }

    return FEXCore::ArchHelpers::Arm64::HandleSIGBUS(Thread->CTX->Config.ParanoidTSO(), Signal, info, ucontext);
  }, true);
#endif
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

void *Arm64JITCore::CompileCode(uint64_t Entry,
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

#ifdef VIXL_DISASSEMBLER
  const auto DisasmBegin = GetCursorAddress<const vixl::aarch64::Instruction*>();
#endif

#ifndef NDEBUG
  LoadConstant(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r0, Entry);
#endif

  this->IR = IR;

  // Fairly excessive buffer range to make sure we don't overflow
  uint32_t BufferRange = SSACount * 16 + GDBEnabled * Dispatcher::MaxGDBPauseCheckSize;
  if ((GetCursorOffset() + BufferRange) > CurrentCodeBuffer->Size) {
    CTX->ClearCodeCache(ThreadState);
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

  GuestEntry = GetCursorAddress<uint8_t *>();

  if (GDBEnabled) {
    auto GDBSize = CTX->Dispatcher->GenerateGDBPauseCheck(GuestEntry, Entry);
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
#define REGISTER_OP(op, x) case FEXCore::IR::IROps::OP_##op: Op_##x(IROp, ID); break
        // ALU ops
        REGISTER_OP(TRUNCELEMENTPAIR,  TruncElementPair);
        REGISTER_OP(CONSTANT,          Constant);
        REGISTER_OP(ENTRYPOINTOFFSET,  EntrypointOffset);
        REGISTER_OP(INLINECONSTANT,    InlineConstant);
        REGISTER_OP(INLINEENTRYPOINTOFFSET,  InlineEntrypointOffset);
        REGISTER_OP(CYCLECOUNTER,      CycleCounter);
        REGISTER_OP(ADD,               Add);
        REGISTER_OP(SUB,               Sub);
        REGISTER_OP(NEG,               Neg);
        REGISTER_OP(MUL,               Mul);
        REGISTER_OP(UMUL,              UMul);
        REGISTER_OP(DIV,               Div);
        REGISTER_OP(UDIV,              UDiv);
        REGISTER_OP(REM,               Rem);
        REGISTER_OP(UREM,              URem);
        REGISTER_OP(MULH,              MulH);
        REGISTER_OP(UMULH,             UMulH);
        REGISTER_OP(OR,                Or);
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
        REGISTER_OP(FINDTRAILINGZEROS, FindTrailingZeros);
        REGISTER_OP(COUNTLEADINGZEROES, CountLeadingZeroes);
        REGISTER_OP(REV,               Rev);
        REGISTER_OP(BFI,               Bfi);
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
        REGISTER_OP(ATOMICFETCHOR,  AtomicFetchOr);
        REGISTER_OP(ATOMICFETCHXOR, AtomicFetchXor);
        REGISTER_OP(ATOMICFETCHNEG, AtomicFetchNeg);

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

        // Conversion ops
        REGISTER_OP(VINSGPR,         VInsGPR);
        REGISTER_OP(VCASTFROMGPR,    VCastFromGPR);
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
        REGISTER_OP(LOADREGISTER,        LoadRegister);
        REGISTER_OP(STOREREGISTER,       StoreRegister);
        REGISTER_OP(LOADCONTEXTINDEXED,  LoadContextIndexed);
        REGISTER_OP(STORECONTEXTINDEXED, StoreContextIndexed);
        REGISTER_OP(SPILLREGISTER,       SpillRegister);
        REGISTER_OP(FILLREGISTER,        FillRegister);
        REGISTER_OP(LOADFLAG,            LoadFlag);
        REGISTER_OP(STOREFLAG,           StoreFlag);
        REGISTER_OP(LOADMEM,             LoadMem);
        REGISTER_OP(STOREMEM,            StoreMem);
        case FEXCore::IR::IROps::OP_LOADMEMTSO:
          if (ParanoidTSO()) {
            Op_ParanoidLoadMemTSO(IROp, ID);
          }
          else {
            Op_LoadMemTSO(IROp, ID);
          }
          break;
        case FEXCore::IR::IROps::OP_STOREMEMTSO:
          if (ParanoidTSO()) {
            Op_ParanoidStoreMemTSO(IROp, ID);
          }
          else {
            Op_StoreMemTSO(IROp, ID);
          }
          break;
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
        REGISTER_OP(PHI,        NoOp);
        REGISTER_OP(PHIVALUE,   NoOp);
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
        REGISTER_OP(VSQXTUN,           VSQXTUN);
        REGISTER_OP(VSQXTUN2,          VSQXTUN2);
        REGISTER_OP(VUMUL,             VMul);
        REGISTER_OP(VSMUL,             VMul);
        REGISTER_OP(VUMULL,            VUMull);
        REGISTER_OP(VSMULL,            VSMull);
        REGISTER_OP(VUMULL2,           VUMull2);
        REGISTER_OP(VSMULL2,           VSMull2);
        REGISTER_OP(VUABDL,            VUABDL);
        REGISTER_OP(VTBL1,             VTBL1);
        REGISTER_OP(VREV64,            VRev64);
#undef REGISTER_OP

        default:
          Op_Unhandled(IROp, ID);
          break;
      }
    }

    if (DebugData) {
      DebugData->Subblocks.push_back({
        static_cast<uint32_t>(BlockStartHostCode - GuestEntry),
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

  auto CodeEnd = GetCursorAddress<uint8_t *>();
  ClearICache(GuestEntry, CodeEnd - GuestEntry);

#ifdef VIXL_DISASSEMBLER
  const auto DisasmEnd = GetCursorAddress<const vixl::aarch64::Instruction*>();
  Disasm.DisassembleBuffer(DisasmBegin, DisasmEnd);
#endif

  if (DebugData) {
    DebugData->HostCodeSize = CodeEnd - GuestEntry;
    DebugData->Relocations = &Relocations;
  }

  this->IR = nullptr;

  return GuestEntry;
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

std::unique_ptr<CPUBackend> CreateArm64JITCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread) {
  return std::make_unique<Arm64JITCore>(ctx, Thread);
}

void InitializeArm64JITSignalHandlers(FEXCore::Context::Context *CTX) {
  Arm64JITCore::InitializeSignalHandlers(CTX);
}

CPUBackendFeatures GetArm64JITBackendFeatures() {
  return CPUBackendFeatures {
    .SupportsStaticRegisterAllocation = true
  };
}

}
