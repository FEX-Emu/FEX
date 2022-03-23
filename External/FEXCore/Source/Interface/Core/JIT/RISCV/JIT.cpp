#include "FEXCore/IR/IR.h"
#include "Interface/Context/Context.h"
#include "Interface/Core/LookupCache.h"

#include "Interface/Core/ArchHelpers/MContext.h"
#include "Interface/Core/ArchHelpers/RISCV.h"
#include "Interface/Core/Dispatcher/RISCVDispatcher.h"
#include "Interface/Core/JIT/RISCV/JITClass.h"
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
  int64_t Res64 = Res;
  LogMan::Msg::DFmt("0x{:x}'{:x} / 0x{:x} = 0x{:x}", SrcHigh, SrcLow, Divisor, Res64);
  return Res64;
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
void RISCVJITCore::Op_Unhandled(IR::IROp_Header *IROp, IR::NodeID Node) {
  FallbackInfo Info;
  if (!InterpreterOps::GetFallbackHandler(IROp, &Info)) {
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
    LOGMAN_MSG_A_FMT("Unhandled IR Op: {}", FEXCore::IR::GetName(IROp->Op));
#endif
  } else {
    switch(Info.ABI) {
      case FABI_VOID_U16:{
        SpillStaticRegs();

        UXTH(a0, GetReg(IROp->Args[0].ID()));
        ADDI(ra, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.RISCV.FallbackHandlerPointers[Info.HandlerIndex]));
        LD(ra, 0, ra);
        JALR(ra);

        FillStaticRegs();
      }
      break;

      case FABI_F80_F32:{
        SpillStaticRegs();
        auto Src1 = GetVIndex(IROp->Args[0].ID());
        FLW(fa0, Src1 * 16 + 0, FPRSTATE);

        ADDI(ra, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.RISCV.FallbackHandlerPointers[Info.HandlerIndex]));
        LD(ra, 0, ra);
        JALR(ra);

        // return in a0 and a1
        auto Dst  = GetVIndex(Node);
        SD(zero, Dst * 16 + 0, FPRSTATE);
        SD(zero, Dst * 16 + 8, FPRSTATE);

        SD(a0, Dst * 16 + 0, FPRSTATE);
        SH(a1, Dst * 16 + 8, FPRSTATE);

        FillStaticRegs();
      }
      break;

      case FABI_F80_F64:{
        SpillStaticRegs();

        auto Src1 = GetVIndex(IROp->Args[0].ID());
        FLD(fa0, Src1 * 16 + 0, FPRSTATE);

        ADDI(ra, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.RISCV.FallbackHandlerPointers[Info.HandlerIndex]));
        LD(ra, 0, ra);
        JALR(ra);

        // return in a0 and a1
        auto Dst  = GetVIndex(Node);
        SD(zero, Dst * 16 + 0, FPRSTATE);
        SD(zero, Dst * 16 + 8, FPRSTATE);

        SD(a0, Dst * 16 + 0, FPRSTATE);
        SH(a1, Dst * 16 + 8, FPRSTATE);

        FillStaticRegs();
      }
      break;

      case FABI_F80_I16:
      case FABI_F80_I32: {
        SpillStaticRegs();

        auto Phys = GetPhys(IROp->Args[0].ID());
        if (Phys.Class == IR::GPRFixedClass.Val ||
            Phys.Class == IR::GPRClass) {
          if (Info.ABI == FABI_F80_I16) {
            UXTH(a0, GetReg(IROp->Args[0].ID()));
          }
          else {
            MV(a0, GetReg(IROp->Args[0].ID()));
          }
        }
        else {
          auto Src1 = GetVIndex(IROp->Args[0].ID());
          if (Info.ABI == FABI_F80_I16) {
            LHU(a0, Src1 * 16 + 0, FPRSTATE);
          }
          else {
            LWU(a0, Src1 * 16 + 0, FPRSTATE);
          }
        }

        ADDI(ra, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.RISCV.FallbackHandlerPointers[Info.HandlerIndex]));
        LD(ra, 0, ra);
        JALR(ra);

        // return in a0 and a1
        auto Dst  = GetVIndex(Node);
        SD(zero, Dst * 16 + 0, FPRSTATE);
        SD(zero, Dst * 16 + 8, FPRSTATE);

        SD(a0, Dst * 16 + 0, FPRSTATE);
        SH(a1, Dst * 16 + 8, FPRSTATE);

        FillStaticRegs();
      }
      break;

      case FABI_F32_F80:{
        SpillStaticRegs();
        auto Src1 = GetVIndex(IROp->Args[0].ID());
        LD(a0, Src1 * 16 + 0, FPRSTATE);
        LHU(a1, Src1 * 16 + 8, FPRSTATE);

        ADDI(ra, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.RISCV.FallbackHandlerPointers[Info.HandlerIndex]));
        LD(ra, 0, ra);
        JALR(ra);

        // return in fa0
        auto Dst  = GetVIndex(Node);
        SD(zero, Dst * 16 + 0, FPRSTATE);
        SD(zero, Dst * 16 + 8, FPRSTATE);

        FSW(fa0, Dst * 16 + 0, FPRSTATE);

        FillStaticRegs();
      }
      break;
      case FABI_F64_F80:{
        SpillStaticRegs();
        auto Src1 = GetVIndex(IROp->Args[0].ID());
        LD(a0, Src1 * 16 + 0, FPRSTATE);
        LHU(a1, Src1 * 16 + 8, FPRSTATE);

        ADDI(ra, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.RISCV.FallbackHandlerPointers[Info.HandlerIndex]));
        LD(ra, 0, ra);
        JALR(ra);

        // return in fa0
        auto Dst  = GetVIndex(Node);
        SD(zero, Dst * 16 + 0, FPRSTATE);
        SD(zero, Dst * 16 + 8, FPRSTATE);

        FSD(fa0, Dst * 16 + 0, FPRSTATE);

        FillStaticRegs();
      }
      break;

      case FABI_I16_F80:{
        SpillStaticRegs();

        auto Src1 = GetVIndex(IROp->Args[0].ID());
        LD(a0, Src1 * 16 + 0, FPRSTATE);
        LHU(a1, Src1 * 16 + 8, FPRSTATE);

        ADDI(ra, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.RISCV.FallbackHandlerPointers[Info.HandlerIndex]));
        LD(ra, 0, ra);
        JALR(ra);

        // return in a0
        auto Dst  = GetReg(Node);
        UXTH(TMP1, a0);

        FillStaticRegs();
        MV(Dst, TMP1);
      }
      break;
      case FABI_I32_F80:{
        SpillStaticRegs();

        auto Src1 = GetVIndex(IROp->Args[0].ID());
        LD(a0, Src1 * 16 + 0, FPRSTATE);
        LHU(a1, Src1 * 16 + 8, FPRSTATE);

        ADDI(ra, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.RISCV.FallbackHandlerPointers[Info.HandlerIndex]));
        LD(ra, 0, ra);
        JALR(ra);

        // return in a0
        auto Dst  = GetReg(Node);
        UXTW(TMP1, a0);

        FillStaticRegs();
        MV(Dst, TMP1);
      }
      break;
      case FABI_I64_F80:{
        SpillStaticRegs();

        auto Src1 = GetVIndex(IROp->Args[0].ID());
        LD(a0, Src1 * 16 + 0, FPRSTATE);
        LHU(a1, Src1 * 16 + 8, FPRSTATE);

        ADDI(ra, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.RISCV.FallbackHandlerPointers[Info.HandlerIndex]));
        LD(ra, 0, ra);
        JALR(ra);

        // return in a0
        auto Dst  = GetReg(Node);
        MV(TMP1, a0);

        FillStaticRegs();
        MV(Dst, TMP1);
      }
      break;
      case FABI_I64_F80_F80:{
        SpillStaticRegs();

        auto Src1 = GetVIndex(IROp->Args[0].ID());
        auto Src2 = GetVIndex(IROp->Args[1].ID());

        LD(a0, Src1 * 16 + 0, FPRSTATE);
        LHU(a1, Src1 * 16 + 8, FPRSTATE);

        LD(a2, Src2 * 16 + 0, FPRSTATE);
        LHU(a3, Src2 * 16 + 8, FPRSTATE);

        ADDI(ra, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.RISCV.FallbackHandlerPointers[Info.HandlerIndex]));
        LD(ra, 0, ra);
        JALR(ra);

        // return in a0
        auto Dst  = GetReg(Node);
        MV(TMP1, a0);

        FillStaticRegs();
        MV(Dst, TMP1);
      }
      break;
      case FABI_F80_F80:{
        SpillStaticRegs();

        auto Src1 = GetVIndex(IROp->Args[0].ID());

        LD(a0, Src1 * 16 + 0, FPRSTATE);
        LHU(a1, Src1 * 16 + 8, FPRSTATE);

        ADDI(ra, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.RISCV.FallbackHandlerPointers[Info.HandlerIndex]));
        LD(ra, 0, ra);
        JALR(ra);

        // return in a0 and a1
        auto Dst  = GetVIndex(Node);
        SD(a0, Dst * 16 + 0, FPRSTATE);
        SH(a1, Dst * 16 + 8, FPRSTATE);

        FillStaticRegs();
      }
      break;
      case FABI_F80_F80_F80:{
        SpillStaticRegs();

        auto Src1 = GetVIndex(IROp->Args[0].ID());
        auto Src2 = GetVIndex(IROp->Args[1].ID());

        LD(a0, Src1 * 16 + 0, FPRSTATE);
        LHU(a1, Src1 * 16 + 8, FPRSTATE);

        LD(a2, Src2 * 16 + 0, FPRSTATE);
        LHU(a3, Src2 * 16 + 8, FPRSTATE);

        ADDI(ra, STATE, offsetof(FEXCore::Core::CpuStateFrame, Pointers.RISCV.FallbackHandlerPointers[Info.HandlerIndex]));
        LD(ra, 0, ra);
        JALR(ra);

        // return in a0 and a1
        auto Dst  = GetVIndex(Node);
        SD(a0, Dst * 16 + 0, FPRSTATE);
        SH(a1, Dst * 16 + 8, FPRSTATE);

        FillStaticRegs();
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

void RISCVJITCore::Op_NoOp(IR::IROp_Header *IROp, IR::NodeID Node) {
}

IR::PhysicalRegister RISCVJITCore::GetPhys(IR::NodeID Node) const {
  auto PhyReg = RAData->GetNodeRegister(Node);

  LOGMAN_THROW_A_FMT(!PhyReg.IsInvalid(), "Couldn't Allocate register for node: ssa{}. Class: {}", Node, PhyReg.Class);

  return PhyReg;
}

biscuit::GPR RISCVJITCore::GetReg(IR::NodeID Node) const {
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

std::pair<biscuit::GPR, biscuit::GPR> RISCVJITCore::GetSrcPair(IR::NodeID Node) const {
  uint32_t Reg = GetPhys(Node).Reg;
  return RA64Pair[Reg];
}

biscuit::Vec RISCVJITCore::GetVReg(IR::NodeID Node) const {
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

uint32_t RISCVJITCore::GetVIndex(IR::NodeID Node) const {
  auto Reg = GetPhys(Node);
  if (Reg.Class == IR::FPRFixedClass.Val) {
    return Reg.Reg;
  } else if (Reg.Class == IR::FPRClass.Val) {
    return 32 + Reg.Reg;
  } else {
    LOGMAN_THROW_A_FMT(false, "Unexpected Class: {}", Reg.Class);
  }

  FEX_UNREACHABLE;
}

bool RISCVJITCore::IsInlineConstant(const IR::OrderedNodeWrapper& WNode, uint64_t* Value) const {
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

bool RISCVJITCore::IsInlineEntrypointOffset(const IR::OrderedNodeWrapper& WNode, uint64_t* Value) const {
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

RISCVJITCore::CodeBuffer RISCVJITCore::AllocateNewCodeBuffer(size_t Size) {
  auto Ptr = static_cast<uint8_t*>(
               FEXCore::Allocator::mmap(nullptr,
                    Size,
                    PROT_READ | PROT_WRITE | PROT_EXEC,
                    MAP_PRIVATE | MAP_ANONYMOUS,
                    -1, 0));
  CodeBuffer Buffer {
    .Ptr = Ptr,
    .Size = Size,
  };
  LOGMAN_THROW_A_FMT(!!Buffer.Ptr, "Couldn't allocate code buffer");
  Dispatcher->RegisterCodeBuffer(Buffer.Ptr, Buffer.Size);
  if (CTX->Config.GlobalJITNaming()) {
    CTX->Symbols.RegisterJITSpace(Buffer.Ptr, Buffer.Size);
  }
  return Buffer;
}

void RISCVJITCore::FreeCodeBuffer(CodeBuffer Buffer) {
  FEXCore::Allocator::munmap(Buffer.Ptr, Buffer.Size);
  Dispatcher->RemoveCodeBuffer(Buffer.Ptr);
}


RISCVJITCore::RISCVJITCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread, bool CompileThread)
  : RISCVEmitter(ctx, (uint8_t*)1, 0)
  , CTX {ctx}
  , ThreadState {Thread} {
  RAPass = Thread->PassManager->GetPass<IR::RegisterAllocationPass>("RA");

#if DEBUG
  Decoder.AppendVisitor(&Disasm)
#endif

  RAPass->AllocateRegisterSet(RA64.size() + RAFPR.size() + RA64Pair.size(), RegisterClasses);

  RAPass->AddRegisters(FEXCore::IR::GPRClass, RA64.size());
  RAPass->AddRegisters(FEXCore::IR::GPRFixedClass, SRA64.size());
  RAPass->AddRegisters(FEXCore::IR::FPRClass, RAFPR.size());
  RAPass->AddRegisters(FEXCore::IR::FPRFixedClass, SRAFPR.size());
  RAPass->AddRegisters(FEXCore::IR::GPRPairClass, RA64Pair.size());
  RAPass->AddRegisters(FEXCore::IR::ComplexClass, 1);

  for (uint32_t i = 0; i < RA64Pair.size(); ++i) {
    RAPass->AddRegisterConflict(FEXCore::IR::GPRClass, i * 2,     FEXCore::IR::GPRPairClass, i);
    RAPass->AddRegisterConflict(FEXCore::IR::GPRClass, i * 2 + 1, FEXCore::IR::GPRPairClass, i);
  }

  for (uint32_t i = 0; i < FEXCore::IR::IROps::OP_LAST + 1; ++i) {
    OpHandlers[i] = &RISCVJITCore::Op_Unhandled;
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
  //RegisterEncryptionHandlers();

  {
    DispatcherConfig config;
    config.ExitFunctionLinkThis = reinterpret_cast<uintptr_t>(this);
    config.StaticRegisterAssignment = ctx->Config.StaticRegisterAllocation;

    uint8_t *Buffer = (uint8_t*)FEXCore::Allocator::mmap(nullptr, 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    Dispatcher = std::make_unique<RISCVDispatcher>(CTX, ThreadState, Buffer);
    DispatchPtr = Dispatcher->DispatchPtr;
    CallbackPtr = Dispatcher->CallbackPtr;
  }

  {
    // Set up pointers that the JIT needs to load
    auto &Pointers = ThreadState->CurrentFrame->Pointers.RISCV;
    // Process specific
    Pointers.LUDIV = reinterpret_cast<uint64_t>(LUDIV);
    Pointers.LDIV = reinterpret_cast<uint64_t>(LDIV);
    Pointers.LUREM = reinterpret_cast<uint64_t>(LUREM);
    Pointers.LREM = reinterpret_cast<uint64_t>(LREM);
    Pointers.PrintValue = reinterpret_cast<uint64_t>(PrintValue);
    Pointers.PrintVectorValue = reinterpret_cast<uint64_t>(PrintVectorValue);
    Pointers.RemoveCodeEntryFromJIT = reinterpret_cast<uintptr_t>(&Context::Context::RemoveCodeEntryFromJit);
    Pointers.CPUIDObj = reinterpret_cast<uint64_t>(&CTX->CPUID);

    {
      FEXCore::Utils::MemberFunctionToPointerCast PMF(&FEXCore::CPUIDEmu::RunFunction);
      Pointers.CPUIDFunction = PMF.GetConvertedPointer();
    }

    Pointers.SyscallHandlerObj = reinterpret_cast<uint64_t>(CTX->SyscallHandler);
    Pointers.SyscallHandlerFunc = reinterpret_cast<uint64_t>(FEXCore::Context::HandleSyscall);

    // Fill in the fallback handlers
    InterpreterOps::FillFallbackIndexPointers(Pointers.FallbackHandlerPointers);

    // Thread Specific
    Pointers.SignalHandlerRefCountPointer = reinterpret_cast<uint64_t>(&Dispatcher->SignalHandlerRefCounter);
  }

  // Can't allocate a code buffer until after dispatcher is created
  InitialCodeBuffer = AllocateNewCodeBuffer(RISCVJITCore::INITIAL_CODE_SIZE);

  // We don't have a code buffer right now, so throw away what is returned to us
  SwapCodeBuffer(biscuit::CodeBuffer(InitialCodeBuffer.Ptr, InitialCodeBuffer.Size));
  EmitDetectionString();

  CurrentCodeBuffer = &InitialCodeBuffer;
}

void RISCVJITCore::InitializeSignalHandlers(FEXCore::Context::Context *CTX) {
  CTX->SignalDelegation->RegisterHostSignalHandler(SIGBUS, [](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) -> bool {
    RISCVJITCore *Core = reinterpret_cast<RISCVJITCore*>(Thread->CPUBackend.get());

    if (!Core->Dispatcher->IsAddressInJITCode(ArchHelpers::Context::GetPc(ucontext))) {
      // Wasn't a sigbus in JIT code
      return false;
    }

    return FEXCore::ArchHelpers::RISCV::HandleSIGBUS(Core->CTX->Config.ParanoidTSO(), Signal, info, ucontext);
  }, true);

}

void RISCVJITCore::EmitDetectionString() {
  [[maybe_unused]] const char JITString[] = "FEXJIT::RISCVJITCore::";
  // XXX: Not yet
}

RISCVJITCore::~RISCVJITCore() {
}
void RISCVJITCore::CopyNecessaryDataForCompileThread(CPUBackend *Original) {
  [[maybe_unused]] RISCVJITCore *Core = reinterpret_cast<RISCVJITCore*>(Original);
  // XXX: Not yet
}

void RISCVJITCore::ClearCache() {
  // XXX: Not yet
}

void *RISCVJITCore::CompileCode(uint64_t Entry, [[maybe_unused]] FEXCore::IR::IRListView const *IR, [[maybe_unused]] FEXCore::Core::DebugData *DebugData, FEXCore::IR::RegisterAllocationData *RAData) {
  JumpTargets.clear();
  uint32_t SSACount = IR->GetSSACount();

  this->Entry = Entry;
  this->RAData = RAData;
  this->IR = IR;

  #ifndef NDEBUG
  LoadConstant(TMP1, Entry);
  #endif

  auto GuestEntry = GetCursorPointer();

  // XXX: Buffer range checking

  // XXX: GDBServer bits
  SpillSlots = RAData->SpillSlots();

  if (SpillSlots) {
    ADDI(sp, sp, SpillSlots * -16);
  }

  PendingTargetLabel = nullptr;

  for (auto [BlockNode, BlockHeader] : IR->GetBlocks()) {
    using namespace FEXCore::IR;
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
    auto BlockIROp = BlockHeader->CW<FEXCore::IR::IROp_CodeBlock>();
    LOGMAN_THROW_A_FMT(BlockIROp->Header.Op == IR::OP_CODEBLOCK, "IR type failed to be a code block");
#endif

    uintptr_t BlockStartHostCode = (uintptr_t)GetCursorPointer();
    {
      const auto Node = IR->GetID(BlockNode);
      const auto IsTarget = JumpTargets.try_emplace(Node).first;

      // if there's a pending branch, and it is not fall-through
      if (PendingTargetLabel && PendingTargetLabel != &IsTarget->second)
      {
        J(PendingTargetLabel);
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
      DebugData->Subblocks.push_back({BlockStartHostCode, static_cast<uint32_t>((uintptr_t)GetCursorPointer() - BlockStartHostCode)});
    }
  }

  // Make sure last branch is generated. It certainly can't be eliminated here.
  if (PendingTargetLabel)
  {
    J(PendingTargetLabel);
  }
  PendingTargetLabel = nullptr;

  auto CodeEnd = GetCursorPointer();

  // XXX: icache clearing

  if (DebugData) {
    DebugData->HostCodeSize = reinterpret_cast<uintptr_t>(CodeEnd) - reinterpret_cast<uintptr_t>(GuestEntry);
  }

  this->IR = nullptr;

  LogMan::Msg::DFmt("Entry: 0x{:x} disas /r 0x{:x},+{}", Entry, (uintptr_t)GuestEntry, DebugData->HostCodeSize);
  return reinterpret_cast<void*>(GuestEntry);
}

std::unique_ptr<CPUBackend> CreateRISCVJITCore(FEXCore::Context::Context *ctx, FEXCore::Core::InternalThreadState *Thread, bool CompileThread) {
  return std::make_unique<RISCVJITCore>(ctx, Thread, CompileThread);
}

void InitializeRISCVJITSignalHandlers(FEXCore::Context::Context *CTX) {
  RISCVJITCore::InitializeSignalHandlers(CTX);
}

}
