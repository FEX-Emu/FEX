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
          uxth(ARMEmitter::Size::i32Bit, ARMEmitter::Reg::r0, Src1);
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
        uxth(ARMEmitter::Size::i64Bit, Dst, ARMEmitter::Reg::r0);
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

      // Execute handler
      OpHandler Handler = OpHandlers[IROp->Op];
      (this->*Handler)(IROp, ID);
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
