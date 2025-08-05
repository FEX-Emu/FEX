// SPDX-License-Identifier: MIT
/*
$info$
tags: ir|opts
desc: ConstProp, ZExt elim, const pooling, fcmp reduction, const inlining
$end_info$
*/
#include <CodeEmitter/Emitter.h>

#include "Interface/IR/IREmitter.h"
#include "Interface/IR/PassManager.h"

#include <FEXCore/IR/IR.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/Profiler.h>
#include <FEXCore/fextl/map.h>
#include <FEXCore/fextl/unordered_map.h>

#include <cstdint>
#include <string.h>

namespace FEXCore::IR {

// aarch64 heuristics
static bool IsImmLogical(uint64_t imm, unsigned width) {
  if (width < 32) {
    width = 32;
  }
  return ARMEmitter::Emitter::IsImmLogical(imm, width);
}

class ConstProp final : public FEXCore::IR::Pass {
public:
  explicit ConstProp(bool SupportsTSOImm9)
    : SupportsTSOImm9 {SupportsTSOImm9} {}

  void Run(IREmitter* IREmit) override;

private:
  void ConstantPropagation(IREmitter* IREmit, const IRListView& CurrentIR, Ref CodeNode, IROp_Header* IROp);

  bool SupportsTSOImm9 {};

  template<class F>
  bool InlineIf(IREmitter* IREmit, const IRListView& CurrentIR, Ref CodeNode, IROp_Header* IROp, unsigned Index, F Filter) {
    uint64_t Constant;
    if (!IREmit->IsValueConstant(IROp->Args[Index], &Constant) || !Filter(Constant)) {
      return false;
    }

    IREmit->SetWriteCursor(CurrentIR.GetNode(IROp->Args[Index]));
    IREmit->ReplaceNodeArgument(CodeNode, Index, IREmit->_InlineConstant(Constant));
    return true;
  }

  bool Inline(IREmitter* IREmit, const IRListView& CurrentIR, Ref CodeNode, IROp_Header* IROp, unsigned Index) {
    return InlineIf(IREmit, CurrentIR, CodeNode, IROp, Index, [](uint64_t _) { return true; });
  }

  bool InlineIfZero(IREmitter* IREmit, const IRListView& CurrentIR, Ref CodeNode, IROp_Header* IROp, unsigned Index) {
    return InlineIf(IREmit, CurrentIR, CodeNode, IROp, Index, [](uint64_t X) { return X == 0; });
  }

  bool InlineIfLargeAddSub(IREmitter* IREmit, const IRListView& CurrentIR, Ref CodeNode, IROp_Header* IROp, unsigned Index) {
    // We don't allow 8/16-bit operations to have constants, since no
    // constant would be in bounds after the JIT's 24/16 shift.
    auto Filter = [&IROp](uint64_t X) {
      return ARMEmitter::IsImmAddSub(X) && IROp->Size >= OpSize::i32Bit;
    };

    return InlineIf(IREmit, CurrentIR, CodeNode, IROp, Index, Filter);
  }

  void InlineMemImmediate(IREmitter* IREmit, const IRListView& IR, Ref CodeNode, IR::RegisterClassType RegisterClass, IROp_Header* IROp,
                          OrderedNodeWrapper Offset, MemOffsetType OffsetType, const size_t Offset_Index, uint8_t& OffsetScale, bool TSO) {
    uint64_t Imm {};
    if (OffsetType != MEM_OFFSET_SXTX || !IREmit->IsValueConstant(Offset, &Imm)) {
      return;
    }

    // The immediate may be scaled in the IR, we need to correct for that.
    Imm *= OffsetScale;

    // Signed immediate unscaled 9-bit range for both regular and LRCPC2 ops.
    bool IsSIMM9 = ((int64_t)Imm >= -256) && ((int64_t)Imm <= 255);
    IsSIMM9 &= (SupportsTSOImm9 || !TSO);

    // Extended offsets for regular loadstore only.
    LOGMAN_THROW_A_FMT(IROp->Size >= IR::OpSize::i8Bit && IROp->Size <= (RegisterClass == GPRClass ? IR::OpSize::i64Bit : IR::OpSize::i256Bit),
                       "Invalid "
                       "size");
    bool IsExtended = (Imm & (IR::OpSizeToSize(IROp->Size) - 1)) == 0 && Imm / IR::OpSizeToSize(IROp->Size) <= 4095;
    IsExtended &= !TSO;

    if (IsSIMM9 || IsExtended) {
      IREmit->SetWriteCursor(IR.GetNode(Offset));
      IREmit->ReplaceNodeArgument(CodeNode, Offset_Index, IREmit->_InlineConstant(Imm));
      OffsetScale = 1;
    }
  }
};

// constprop + some more per instruction logic
void ConstProp::ConstantPropagation(IREmitter* IREmit, const IRListView& CurrentIR, Ref CodeNode, IROp_Header* IROp) {
  switch (IROp->Op) {
  case OP_ADD:
  case OP_SUB:
  case OP_ADDWITHFLAGS:
  case OP_SUBWITHFLAGS: {
    auto Op = IROp->C<IR::IROp_Add>();
    uint64_t Constant1 {};
    uint64_t Constant2 {};
    bool IsConstant2 = IREmit->IsValueConstant(IROp->Args[1], &Constant2);

    /* IsImmAddSub assumes the constants are sign-extended, take care of that
     * here so we get the optimization for 32-bit adds too.
     */
    if (Op->Header.Size == OpSize::i32Bit) {
      Constant1 = (int64_t)(int32_t)Constant1;
      Constant2 = (int64_t)(int32_t)Constant2;
    }

    if (IsConstant2 && !ARMEmitter::IsImmAddSub(Constant2) && ARMEmitter::IsImmAddSub(-Constant2)) {
      // If the second argument is constant, the immediate is not ImmAddSub, but when negated is.
      // So, negate the operation to negate (and inline) the constant.
      if (IROp->Op == OP_ADD) {
        IROp->Op = OP_SUB;
      } else if (IROp->Op == OP_SUB) {
        IROp->Op = OP_ADD;
      } else if (IROp->Op == OP_ADDWITHFLAGS) {
        IROp->Op = OP_SUBWITHFLAGS;
      } else if (IROp->Op == OP_SUBWITHFLAGS) {
        IROp->Op = OP_ADDWITHFLAGS;
      }

      IREmit->SetWriteCursorBefore(CodeNode);

      // Negate the constant.
      auto NegConstant = IREmit->_Constant(-Constant2);

      // Replace the second source with the negated constant.
      IREmit->ReplaceNodeArgument(CodeNode, Op->Src2_Index, NegConstant);
    }

    if (!InlineIfLargeAddSub(IREmit, CurrentIR, CodeNode, IROp, 1) && (IROp->Op == OP_SUB || IROp->Op == OP_SUBWITHFLAGS)) {
      // TODO: Generalize this
      InlineIfZero(IREmit, CurrentIR, CodeNode, IROp, 0);
    }

    break;
  }
  case OP_ADDNZCV: {
    InlineIfLargeAddSub(IREmit, CurrentIR, CodeNode, IROp, 1);
    break;
  }
  case OP_SUBNZCV: {
    if (!InlineIfLargeAddSub(IREmit, CurrentIR, CodeNode, IROp, 1)) {
      // TODO: Generalize this
      InlineIfZero(IREmit, CurrentIR, CodeNode, IROp, 0);
    }
    break;
  }
  case OP_AND:
  case OP_OR:
  case OP_XOR: {
    InlineIf(IREmit, CurrentIR, CodeNode, IROp, 1, [&IROp](uint64_t X) { return IsImmLogical(X, IR::OpSizeAsBits(IROp->Size)); });
    break;
  }
  case OP_ANDWITHFLAGS:
  case OP_ANDN:
  case OP_TESTNZ: {
    InlineIf(IREmit, CurrentIR, CodeNode, IROp, 1, [&IROp](uint64_t X) { return IsImmLogical(X, IR::OpSizeAsBits(IROp->Size)); });
    break;
  }
  case OP_ASHR:
  case OP_ROR: {
    Inline(IREmit, CurrentIR, CodeNode, IROp, 1);
    break;
  }
  case OP_LSHL: {
    Inline(IREmit, CurrentIR, CodeNode, IROp, 1);
    break;
  }
  case OP_LSHR: {
    Inline(IREmit, CurrentIR, CodeNode, IROp, 1);
    break;
  }
  case OP_ADC:
  case OP_ADCWITHFLAGS:
  case OP_RMIFNZCV: {
    InlineIfZero(IREmit, CurrentIR, CodeNode, IROp, 0);
    break;
  }
  case OP_STORECONTEXT: {
    // For i128Bit, we won't see a normal Constant to inline, but as a special
    // case we can replace with a 2x64-bit store which can use inline zeroes.
    if (IROp->Size == OpSize::i128Bit) {
      auto Op = IROp->C<IR::IROp_StoreContext>();
      auto Header = IREmit->GetOpHeader(IROp->Args[0]);
      const auto MAX_STP_OFFSET = (252 * 4);

      if (Op->Offset <= MAX_STP_OFFSET && Header->Op == OP_LOADNAMEDVECTORCONSTANT) {
        auto Const = Header->C<IR::IROp_LoadNamedVectorConstant>();

        if (Const->Constant == IR::NamedVectorConstant::NAMED_VECTOR_ZERO) {
          IREmit->SetWriteCursor(CodeNode);
          Ref Zero = IREmit->_Constant(0);
          Ref STP = IREmit->_StoreContextPair(IR::OpSize::i64Bit, GPRClass, Zero, Zero, Op->Offset);
          IREmit->Remove(CodeNode);

          // XXX: This works around InlineConstant not having an associated
          // register class, else we'd just do InlineConstant above.
          Ref InlineZero = IREmit->_InlineConstant(0);
          IREmit->ReplaceNodeArgument(STP, 0, InlineZero);
          IREmit->ReplaceNodeArgument(STP, 1, InlineZero);
        }
      }
    } else {
      InlineIfZero(IREmit, CurrentIR, CodeNode, IROp, 0);
    }

    break;
  }

  case OP_CONDADDNZCV:
  case OP_CONDSUBNZCV: {
    InlineIfZero(IREmit, CurrentIR, CodeNode, IROp, 0);
    InlineIf(IREmit, CurrentIR, CodeNode, IROp, 1, ARMEmitter::IsImmAddSub);
    break;
  }
  case OP_SELECT:
  case OP_CONDJUMP: {
    InlineIf(IREmit, CurrentIR, CodeNode, IROp, 1, ARMEmitter::IsImmAddSub);
    break;
  }
  case OP_EXITFUNCTION: {
    auto Op = IROp->C<IR::IROp_ExitFunction>();

    if (!Inline(IREmit, CurrentIR, CodeNode, IROp, Op->NewRIP_Index)) {
      auto NewRIP = IREmit->GetOpHeader(Op->NewRIP);
      if (NewRIP->Op == OP_ENTRYPOINTOFFSET) {
        auto EO = NewRIP->C<IR::IROp_EntrypointOffset>();
        IREmit->SetWriteCursor(CurrentIR.GetNode(Op->NewRIP));

        IREmit->ReplaceNodeArgument(CodeNode, 0, IREmit->_InlineEntrypointOffset(EO->Header.Size, EO->Offset));
      }
    }
    break;
  }

  case OP_LOADMEM: {
    auto Op = IROp->CW<IR::IROp_LoadMem>();
    InlineMemImmediate(IREmit, CurrentIR, CodeNode, Op->Class, IROp, Op->Offset, Op->OffsetType, Op->Offset_Index, Op->OffsetScale, false);
    break;
  }
  case OP_STOREMEM: {
    auto Op = IROp->CW<IR::IROp_StoreMem>();
    InlineMemImmediate(IREmit, CurrentIR, CodeNode, Op->Class, IROp, Op->Offset, Op->OffsetType, Op->Offset_Index, Op->OffsetScale, false);
    InlineIfZero(IREmit, CurrentIR, CodeNode, IROp, Op->Value_Index);
    break;
  }
  case OP_PREFETCH: {
    auto Op = IROp->CW<IR::IROp_Prefetch>();
    InlineMemImmediate(IREmit, CurrentIR, CodeNode, GPRClass, IROp, Op->Offset, Op->OffsetType, Op->Offset_Index, Op->OffsetScale, false);
    break;
  }
  case OP_LOADMEMTSO: {
    auto Op = IROp->CW<IR::IROp_LoadMemTSO>();
    InlineMemImmediate(IREmit, CurrentIR, CodeNode, Op->Class, IROp, Op->Offset, Op->OffsetType, Op->Offset_Index, Op->OffsetScale, true);
    break;
  }
  case OP_STOREMEMTSO: {
    auto Op = IROp->CW<IR::IROp_StoreMemTSO>();
    InlineMemImmediate(IREmit, CurrentIR, CodeNode, Op->Class, IROp, Op->Offset, Op->OffsetType, Op->Offset_Index, Op->OffsetScale, true);
    InlineIfZero(IREmit, CurrentIR, CodeNode, IROp, Op->Value_Index);
    break;
  }
  case OP_STOREMEMPAIR: {
    auto Op = IROp->CW<IR::IROp_StoreMemPair>();
    InlineIfZero(IREmit, CurrentIR, CodeNode, IROp, Op->Value1_Index);
    InlineIfZero(IREmit, CurrentIR, CodeNode, IROp, Op->Value2_Index);
    break;
  }
  case OP_MEMCPY: {
    auto Op = IROp->CW<IR::IROp_MemCpy>();
    Inline(IREmit, CurrentIR, CodeNode, IROp, Op->Direction_Index);
    break;
  }
  case OP_MEMSET: {
    auto Op = IROp->CW<IR::IROp_MemSet>();
    Inline(IREmit, CurrentIR, CodeNode, IROp, Op->Direction_Index);
    InlineIfZero(IREmit, CurrentIR, CodeNode, IROp, Op->Value_Index);
    break;
  }

  default: break;
  }
}

void ConstProp::Run(IREmitter* IREmit) {
  FEXCORE_PROFILE_SCOPED("PassManager::ConstProp");

  auto CurrentIR = IREmit->ViewIR();

  for (auto [BlockNode, BlockIROp] : CurrentIR.GetBlocks()) {
    for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {
      ConstantPropagation(IREmit, CurrentIR, CodeNode, IROp);
    }
  }
}

fextl::unique_ptr<FEXCore::IR::Pass> CreateConstProp(bool SupportsTSOImm9) {
  return fextl::make_unique<ConstProp>(SupportsTSOImm9);
}
} // namespace FEXCore::IR
