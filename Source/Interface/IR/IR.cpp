#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IntrusiveIRList.h>

namespace FEXCore::IR {
#define IROP_GETNAME_IMPL
#include "IRDefines.inc"

static void PrintArg(std::stringstream *out, [[maybe_unused]] IRListView<false> const* IR, uint64_t Arg) {
  *out << "0x" << std::hex << Arg;
}
static void PrintArg(std::stringstream *out, [[maybe_unused]] IRListView<false> const* IR, RegisterClassType Arg) {
  if (Arg == 0)
    *out << "GPR";
  else if (Arg == 1)
    *out << "FPR";
  else
    *out << "Unknown Registerclass " << Arg;
}

static void PrintArg(std::stringstream *out, IRListView<false> const* IR, OrderedNodeWrapper Arg) {
  uintptr_t Data = IR->GetData();
  uintptr_t ListBegin = IR->GetListData();

  OrderedNode *RealNode = Arg.GetNode(ListBegin);
  auto IROp = RealNode->Op(Data);

  *out << "%ssa" << std::to_string(Arg.ID()) << " i" << std::dec << (IROp->Size * 8);
  if (IROp->Elements > 1) {
    *out << "v" << std::dec << IROp->Elements;
  }
}

void Dump(std::stringstream *out, IRListView<false> const* IR) {
  uintptr_t ListBegin = IR->GetListData();
  uintptr_t DataBegin = IR->GetData();

  auto Begin = IR->begin();
  auto Op = Begin();

  OrderedNode *RealNode = Op->GetNode(ListBegin);
  auto HeaderOp = RealNode->Op(DataBegin)->CW<FEXCore::IR::IROp_IRHeader>();
  LogMan::Throw::A(HeaderOp->Header.Op == OP_IRHEADER, "First op wasn't IRHeader");

  OrderedNode *BlockNode = HeaderOp->Blocks.GetNode(ListBegin);
  uint8_t CurrentIndent = 0;
  auto AddIndent = [&out, &CurrentIndent]() {
    for (uint8_t i = 0; i < CurrentIndent; ++i) {
      *out << "\t";
    }
  };

  *out << "(%%ssa" << std::to_string(RealNode->Wrapped(ListBegin).ID()) << ") " << "IRHeader ";
  *out << "0x" << std::hex << HeaderOp->Entry << ", ";
  *out << "%%ssa" << HeaderOp->Blocks.ID() << ", ";
  *out << std::dec << HeaderOp->BlockCount << std::endl;

  while (1) {
    auto BlockIROp = BlockNode->Op(DataBegin)->CW<FEXCore::IR::IROp_CodeBlock>();
    LogMan::Throw::A(BlockIROp->Header.Op == OP_CODEBLOCK, "IR type failed to be a code block");

    // We grab these nodes this way so we can iterate easily
    auto CodeBegin = IR->at(BlockIROp->Begin);
    auto CodeLast = IR->at(BlockIROp->Last);
    *out << "(%%ssa" << std::to_string(BlockNode->Wrapped(ListBegin).ID()) << ") " << "CodeBlock ";

    *out << "%%ssa" << std::to_string(BlockIROp->Begin.ID()) << ", ";
    *out << "%%ssa" << std::to_string(BlockIROp->Last.ID()) << ", ";
    *out << "%%ssa" << std::to_string(BlockIROp->Next.ID()) << std::endl;

    while (1) {
      OrderedNodeWrapper *CodeOp = CodeBegin();
      OrderedNode *CodeNode = CodeOp->GetNode(ListBegin);
      auto IROp = CodeNode->Op(DataBegin);

      auto Name = FEXCore::IR::GetName(IROp->Op);

      AddIndent();
      if (IROp->HasDest) {
        *out << "%ssa" << std::to_string(CodeOp->ID()) << " i" << std::dec << (IROp->Size * 8);
        if (IROp->Elements > 1) {
          *out << "v" << std::dec << IROp->Elements;
        }
        *out << " = ";
      }
      else {
        *out << "(%%ssa" << std::to_string(CodeOp->ID()) << ") ";
      }

      *out << Name;
      switch (IROp->Op) {
        case IR::OP_BEGINBLOCK:
          *out << " %ssa" << std::to_string(CodeOp->ID());
          ++CurrentIndent;
          break;
        case IR::OP_ENDBLOCK:
          --CurrentIndent;
          break;
        default: break;
      }

      #define IROP_ARGPRINTER_HELPER
      #include "IRDefines.inc"
      default: *out << "<Unknown Args>"; break;
      }

      *out << "\n";
      printf("%s", out->str().c_str());
      *out = std::stringstream{};

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
}

}
