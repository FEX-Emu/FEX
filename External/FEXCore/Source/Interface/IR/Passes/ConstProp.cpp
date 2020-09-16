#include "Interface/IR/PassManager.h"
#include "Interface/Core/OpcodeDispatcher.h"

namespace FEXCore::IR {

class ConstProp final : public FEXCore::IR::Pass {
public:
  bool Run(IREmitter *IREmit) override;
};

template<typename T>
static uint64_t getMask(T Op) {
  uint64_t NumBits = Op->Header.Size * 8;
  return (~0ULL) >> (64 - NumBits);
}

bool ConstProp::Run(IREmitter *IREmit) {
  bool Changed = false;
  auto CurrentIR = IREmit->ViewIR();

  auto OriginalWriteCursor = IREmit->GetWriteCursor();

  for (auto [CodeNode, IROp] : CurrentIR.GetAllCode()) {

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

      if (IREmit->IsValueConstant(IROp->Args[0], &Constant1) &&
          IREmit->IsValueConstant(IROp->Args[1], &Constant2)) {
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

      if (IREmit->IsValueConstant(IROp->Args[0], &Constant1)) {
        LogMan::Msg::A("Could const prop op: %s", std::string(IR::GetName(IROp->Op)).c_str());
      }
    break;
    }
*/
    case OP_ADD: {
      auto Op = IROp->C<IR::IROp_Add>();
      uint64_t Constant1;
      uint64_t Constant2;

      if (IREmit->IsValueConstant(Op->Header.Args[0], &Constant1) &&
          IREmit->IsValueConstant(Op->Header.Args[1], &Constant2)) {
        uint64_t NewConstant = (Constant1 + Constant2) & getMask(Op) ;
        IREmit->ReplaceWithConstant(CodeNode, NewConstant);
        Changed = true;
      }
    break;
    }
    case OP_SUB: {
      auto Op = IROp->C<IR::IROp_Sub>();
      uint64_t Constant1;
      uint64_t Constant2;

      if (IREmit->IsValueConstant(Op->Header.Args[0], &Constant1) &&
          IREmit->IsValueConstant(Op->Header.Args[1], &Constant2)) {
        uint64_t NewConstant = (Constant1 - Constant2) & getMask(Op) ;
        IREmit->ReplaceWithConstant(CodeNode, NewConstant);
        Changed = true;
      }
    break;
    }
    case OP_AND: {
      auto Op = IROp->CW<IR::IROp_And>();
      uint64_t Constant1;
      uint64_t Constant2;

      if (IREmit->IsValueConstant(Op->Header.Args[0], &Constant1) &&
          IREmit->IsValueConstant(Op->Header.Args[1], &Constant2)) {
        uint64_t NewConstant = (Constant1 & Constant2) & getMask(Op) ;
        IREmit->ReplaceWithConstant(CodeNode, NewConstant);
        Changed = true;
      } else if (Op->Header.Args[0].ID() == Op->Header.Args[1].ID()) {
        // AND with same value results in original value
        IREmit->ReplaceAllUsesWith(CodeNode, CurrentIR.GetNode(Op->Header.Args[0]));
        Changed = true;
      }
    break;
    }
    case OP_OR: {
      auto Op = IROp->CW<IR::IROp_Or>();
      uint64_t Constant1;
      uint64_t Constant2;

      if (IREmit->IsValueConstant(Op->Header.Args[0], &Constant1) &&
          IREmit->IsValueConstant(Op->Header.Args[1], &Constant2)) {
        uint64_t NewConstant = Constant1 | Constant2;
        IREmit->ReplaceWithConstant(CodeNode, NewConstant);
        Changed = true;
      } else if (Op->Header.Args[0].ID() == Op->Header.Args[1].ID()) {
        // OR with same value results in original value
        IREmit->ReplaceAllUsesWith(CodeNode, CurrentIR.GetNode(Op->Header.Args[0]));
        Changed = true;
      }
    break;
    }
    case OP_XOR: {
      auto Op = IROp->C<IR::IROp_Xor>();
      uint64_t Constant1;
      uint64_t Constant2;

      if (IREmit->IsValueConstant(Op->Header.Args[0], &Constant1) &&
          IREmit->IsValueConstant(Op->Header.Args[1], &Constant2)) {
        uint64_t NewConstant = Constant1 ^ Constant2;
        IREmit->ReplaceWithConstant(CodeNode, NewConstant);
        Changed = true;
      } else if (Op->Header.Args[0].ID() == Op->Header.Args[1].ID()) {
        // XOR with same value results to zero
        IREmit->SetWriteCursor(CodeNode);
        IREmit->ReplaceAllUsesWith(CodeNode, IREmit->_Constant(0));
        Changed = true;
      }
    break;
    }
    case OP_LSHL: {
      auto Op = IROp->CW<IR::IROp_Lshl>();
      uint64_t Constant1;
      uint64_t Constant2;

      if (IREmit->IsValueConstant(Op->Header.Args[0], &Constant1) &&
          IREmit->IsValueConstant(Op->Header.Args[1], &Constant2)) {
        uint64_t NewConstant = (Constant1 << Constant2) & getMask(Op);
        IREmit->ReplaceWithConstant(CodeNode, NewConstant);
        Changed = true;
      }
      else if (IREmit->IsValueConstant(Op->Header.Args[1], &Constant2) &&
                Constant2 == 0) {
        IREmit->SetWriteCursor(CodeNode);
        OrderedNode *Arg = CurrentIR.GetNode(Op->Header.Args[0]);
        IREmit->ReplaceAllUsesWith(CodeNode, Arg);
        Changed = true;
      }
    break;
    }
    case OP_BFE: {
      auto Op = IROp->C<IR::IROp_Bfe>();
      uint64_t Constant;
      if (IROp->Size <= 8 && IREmit->IsValueConstant(Op->Header.Args[0], &Constant)) {
        uint64_t SourceMask = (1ULL << Op->Width) - 1;
        if (Op->Width == 64)
          SourceMask = ~0ULL;
        SourceMask <<= Op->lsb;

        uint64_t NewConstant = (Constant & SourceMask) >> Op->lsb;
        IREmit->ReplaceWithConstant(CodeNode, NewConstant);
        Changed = true;
      } else if (IROp->Size == CurrentIR.GetOp<IROp_Header>(Op->Header.Args[0])->Size && Op->Width == (IROp->Size * 8) && Op->lsb == 0 ) {
        // A BFE that extracts all bits results in original value
  // XXX - This is broken for now - see https://github.com/FEX-Emu/FEX/issues/351
        // IREmit->ReplaceAllUsesWith(CodeNode, CurrentIR.GetNode(Op->Header.Args[0]));
        // Changed = true;
      }

    break;
    }
    case OP_MUL: {
      auto Op = IROp->C<IR::IROp_Mul>();
      uint64_t Constant1;
      uint64_t Constant2;

      if (IREmit->IsValueConstant(Op->Header.Args[0], &Constant1) &&
          IREmit->IsValueConstant(Op->Header.Args[1], &Constant2)) {
        uint64_t NewConstant = (Constant1 * Constant2) & getMask(Op);
        IREmit->ReplaceWithConstant(CodeNode, NewConstant);
        Changed = true;
      }
    }
    default: break;
    }
  }

  IREmit->SetWriteCursor(OriginalWriteCursor);

  return Changed;
}

FEXCore::IR::Pass* CreateConstProp() {
  return new ConstProp{};
}

}
