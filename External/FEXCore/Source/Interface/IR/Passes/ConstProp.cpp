#include "Interface/IR/PassManager.h"
#include "Interface/Core/OpcodeDispatcher.h"

namespace FEXCore::IR {

class ConstProp final : public FEXCore::IR::Pass {
public:
  bool Run(OpDispatchBuilder *Disp) override;
};

bool ConstProp::Run(OpDispatchBuilder *Disp) {
  bool Changed = false;
  auto CurrentIR = Disp->ViewIR();
  uintptr_t ListBegin = CurrentIR.GetListData();
  uintptr_t DataBegin = CurrentIR.GetData();

  auto Begin = CurrentIR.begin();
  auto Op = Begin();

  OrderedNode *RealNode = Op->GetNode(ListBegin);
  auto HeaderOp = RealNode->Op(DataBegin)->CW<FEXCore::IR::IROp_IRHeader>();
  LogMan::Throw::A(HeaderOp->Header.Op == OP_IRHEADER, "First op wasn't IRHeader");

  OrderedNode *BlockNode = HeaderOp->Blocks.GetNode(ListBegin);

  auto OriginalWriteCursor = Disp->GetWriteCursor();

  while (1) {
    auto BlockIROp = BlockNode->Op(DataBegin)->CW<FEXCore::IR::IROp_CodeBlock>();
    LogMan::Throw::A(BlockIROp->Header.Op == OP_CODEBLOCK, "IR type failed to be a code block");

    // We grab these nodes this way so we can iterate easily
    auto CodeBegin = CurrentIR.at(BlockIROp->Begin);
    auto CodeLast = CurrentIR.at(BlockIROp->Last);
    while (1) {
      auto CodeOp = CodeBegin();
      OrderedNode *CodeNode = CodeOp->GetNode(ListBegin);
      auto IROp = CodeNode->Op(DataBegin);
      switch (IROp->Op) {
/*
      case OP_UMUL:
      case OP_DIV:
      case OP_UDIV:
      case OP_REM:
      case OP_UREM:
      case OP_MULH:
      case OP_UMULH:
      case OP_LSHR:
      case OP_ASHR:
      case OP_ROL:
      case OP_ROR:
      case OP_LDIV:
      case OP_LUDIV:
      case OP_LREM:
      case OP_LUREM:
      case OP_BFI:
      {
        uint64_t Constant1;
        uint64_t Constant2;

        if (Disp->IsValueConstant(IROp->Args[0], &Constant1) &&
            Disp->IsValueConstant(IROp->Args[1], &Constant2)) {
          LogMan::Msg::A("Could const prop op: %s", std::string(IR::GetName(IROp->Op)).c_str());
        }
      break;
      }

      case OP_SEXT:
      case OP_NEG:
      case OP_POPCOUNT:
      case OP_FINDLSB:
      case OP_FINDMSB:
      case OP_REV:
      case OP_SBFE:
      {
        uint64_t Constant1;

        if (Disp->IsValueConstant(IROp->Args[0], &Constant1)) {
          LogMan::Msg::A("Could const prop op: %s", std::string(IR::GetName(IROp->Op)).c_str());
        }
      break;
      }
*/
      case OP_ADD: {
        auto Op = IROp->C<IR::IROp_Add>();
        uint64_t Constant1;
        uint64_t Constant2;

        if (Disp->IsValueConstant(Op->Header.Args[0], &Constant1) &&
            Disp->IsValueConstant(Op->Header.Args[1], &Constant2)) {
          uint64_t NewConstant = Constant1 + Constant2;
          Disp->SetWriteCursor(CodeNode);
          auto ConstantVal = Disp->_Constant(NewConstant);
          Disp->ReplaceAllUsesWithInclusive(CodeNode, ConstantVal, CodeBegin, CodeLast);
          Changed = true;
        }
      break;
      }
      case OP_SUB: {
        auto Op = IROp->C<IR::IROp_Sub>();
        uint64_t Constant1;
        uint64_t Constant2;

        if (Disp->IsValueConstant(Op->Header.Args[0], &Constant1) &&
            Disp->IsValueConstant(Op->Header.Args[1], &Constant2)) {
          uint64_t NewConstant = Constant1 - Constant2;
          Disp->SetWriteCursor(CodeNode);
          auto ConstantVal = Disp->_Constant(NewConstant);
          Disp->ReplaceAllUsesWithInclusive(CodeNode, ConstantVal, CodeBegin, CodeLast);
          Changed = true;
        }
      break;
      }
      case OP_AND: {
        auto Op = IROp->C<IR::IROp_And>();
        uint64_t Constant1;
        uint64_t Constant2;

        if (Disp->IsValueConstant(Op->Header.Args[0], &Constant1) &&
            Disp->IsValueConstant(Op->Header.Args[1], &Constant2)) {
          uint64_t NewConstant = Constant1 & Constant2;
          Disp->SetWriteCursor(CodeNode);
          auto ConstantVal = Disp->_Constant(NewConstant);
          Disp->ReplaceAllUsesWithInclusive(CodeNode, ConstantVal, CodeBegin, CodeLast);
          Changed = true;
        }
      break;
      }
      case OP_OR: {
        auto Op = IROp->C<IR::IROp_Or>();
        uint64_t Constant1;
        uint64_t Constant2;

        if (Disp->IsValueConstant(Op->Header.Args[0], &Constant1) &&
            Disp->IsValueConstant(Op->Header.Args[1], &Constant2)) {
          uint64_t NewConstant = Constant1 | Constant2;
          Disp->SetWriteCursor(CodeNode);
          auto ConstantVal = Disp->_Constant(NewConstant);
          Disp->ReplaceAllUsesWithInclusive(CodeNode, ConstantVal, CodeBegin, CodeLast);
          Changed = true;
        }
      break;
      }
      case OP_XOR: {
        auto Op = IROp->C<IR::IROp_Xor>();
        uint64_t Constant1;
        uint64_t Constant2;

        if (Disp->IsValueConstant(Op->Header.Args[0], &Constant1) &&
            Disp->IsValueConstant(Op->Header.Args[1], &Constant2)) {
          uint64_t NewConstant = Constant1 ^ Constant2;
          Disp->SetWriteCursor(CodeNode);
          auto ConstantVal = Disp->_Constant(NewConstant);
          Disp->ReplaceAllUsesWithInclusive(CodeNode, ConstantVal, CodeBegin, CodeLast);
          Changed = true;
        }
      break;
      }
      case OP_LSHL: {
        auto Op = IROp->C<IR::IROp_Lshl>();
        uint64_t Constant1;
        uint64_t Constant2;

        if (Disp->IsValueConstant(Op->Header.Args[0], &Constant1) &&
            Disp->IsValueConstant(Op->Header.Args[1], &Constant2)) {
          uint64_t NewConstant = Constant1 << Constant2;
          Disp->SetWriteCursor(CodeNode);
          auto ConstantVal = Disp->_Constant(NewConstant);
          Disp->ReplaceAllUsesWithInclusive(CodeNode, ConstantVal, CodeBegin, CodeLast);
          Changed = true;
        }
      break;
      }


      case OP_BFE: {
        auto Op = IROp->C<IR::IROp_Bfe>();
        uint64_t Constant;
        if (IROp->Size <= 8 && Disp->IsValueConstant(Op->Header.Args[0], &Constant)) {
          uint64_t SourceMask = (1ULL << Op->Width) - 1;
          if (Op->Width == 64)
            SourceMask = ~0ULL;
          SourceMask <<= Op->lsb;

          uint64_t NewConstant = (Constant & SourceMask) >> Op->lsb;
          Disp->SetWriteCursor(CodeNode);
          auto ConstantVal = Disp->_Constant(NewConstant);
          Disp->ReplaceAllUsesWithInclusive(CodeNode, ConstantVal, CodeBegin, CodeLast);
          Changed = true;
        }

        break;
      }
      case OP_ZEXT: {
        auto Op = IROp->C<IR::IROp_Zext>();
        uint64_t Constant;
        if (Op->SrcSize != 64 &&
            Disp->IsValueConstant(Op->Header.Args[0], &Constant)) {
          uint64_t NewConstant = Constant & ((1ULL << Op->SrcSize) - 1);
          Disp->SetWriteCursor(CodeNode);
          auto ConstantVal = Disp->_Constant(NewConstant);
          Disp->ReplaceAllUsesWithInclusive(CodeNode, ConstantVal, CodeBegin, CodeLast);
          Changed = true;
        }
      break;
      }
      case OP_MUL: {
        auto Op = IROp->C<IR::IROp_Mul>();
        uint64_t Constant1;
        uint64_t Constant2;

        if (Disp->IsValueConstant(Op->Header.Args[0], &Constant1) &&
            Disp->IsValueConstant(Op->Header.Args[1], &Constant2)) {
          uint64_t NewConstant = Constant1 * Constant2;
          Disp->SetWriteCursor(CodeNode);
          auto ConstantVal = Disp->_Constant(NewConstant);
          Disp->ReplaceAllUsesWithInclusive(CodeNode, ConstantVal, CodeBegin, CodeLast);
          Changed = true;
        }
      }
      default: break;
      }

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

  Disp->SetWriteCursor(OriginalWriteCursor);

  return Changed;
}

FEXCore::IR::Pass* CreateConstProp() {
  return new ConstProp{};
}

}
