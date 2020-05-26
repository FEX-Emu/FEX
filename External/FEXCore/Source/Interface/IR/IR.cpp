#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IntrusiveIRList.h>
#include "LogManager.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"

namespace FEXCore::IR {
#define IROP_GETNAME_IMPL
#define IROP_GETRAARGS_IMPL
#define IROP_REG_CLASSES_IMPL
#define IROP_HASSIDEEFFECTS_IMPL

#include <FEXCore/IR/IRDefines.inc>

static void PrintArg(std::stringstream *out, [[maybe_unused]] IRListView<false> const* IR, uint64_t Arg) {
  *out << "#0x" << std::hex << Arg;
}

static void PrintArg(std::stringstream *out, [[maybe_unused]] IRListView<false> const* IR, CondClassType Arg) {
  std::array<std::string, 14> CondNames = {
    "EQ",
    "NEQ",
    "UGE",
    "ULT",
    "MI",
    "PL",
    "VS",
    "VC",
    "UGT",
    "ULE",
    "SGE",
    "SLT",
    "SGT",
    "SLE",
  };

  *out << CondNames[Arg];
}
static void PrintArg(std::stringstream *out, [[maybe_unused]] IRListView<false> const* IR, RegisterClassType Arg) {
  if (Arg == 0)
    *out << "GPR";
  else if (Arg == 1)
    *out << "FPR";
  else if (Arg == 2)
    *out << "GPRPair";
  else
    *out << "Unknown Registerclass " << Arg;
}

static void PrintArg(std::stringstream *out, IRListView<false> const* IR, OrderedNodeWrapper Arg, IR::RegisterAllocationPass *RAPass) {
  uintptr_t Data = IR->GetData();
  uintptr_t ListBegin = IR->GetListData();

  OrderedNode *RealNode = Arg.GetNode(ListBegin);
  auto IROp = RealNode->Op(Data);

  *out << "%ssa" << std::to_string(Arg.ID());
  if (RAPass) {
    uint64_t RegClass = RAPass->GetNodeRegister(Arg.ID());
    FEXCore::IR::RegisterClassType Class {uint32_t(RegClass >> 32)};
    uint32_t Reg = RegClass;
    switch (Class) {
      case FEXCore::IR::GPRClass.Val: *out << "(GPR"; break;
      case FEXCore::IR::FPRClass.Val: *out << "(FPR"; break;
      case FEXCore::IR::ComplexClass.Val: *out << "(Complex"; break;
    }

    *out << std::dec << Reg << ")";
  }

  if (IROp->HasDest) {
    uint32_t ElementSize = IROp->ElementSize;
    if (!IROp->ElementSize) {
      ElementSize = IROp->Size;
    }
    uint32_t NumElements = IROp->Size / ElementSize;

    *out << " i" << std::dec << (ElementSize * 8);

    if (NumElements > 1) {
      *out << "v" << std::dec << NumElements;
    }

  }
}

static void PrintArg(std::stringstream *out, [[maybe_unused]] IRListView<false> const* IR, IR::TypeDefinition Arg) {
  *out << "i" << std::dec << static_cast<uint32_t>(Arg.Bytes() * 8);

  if (Arg.Elements()) {
    *out << "v" << std::dec << static_cast<uint32_t>(Arg.Elements());
  }
}

void Dump(std::stringstream *out, IRListView<false> const* IR, IR::RegisterAllocationPass *RAPass) {
  uintptr_t ListBegin = IR->GetListData();
  uintptr_t DataBegin = IR->GetData();

  auto Begin = IR->begin();
  auto Op = Begin();

  OrderedNode *RealNode = Op->GetNode(ListBegin);
  auto HeaderOp = RealNode->Op(DataBegin)->CW<FEXCore::IR::IROp_IRHeader>();
  LogMan::Throw::A(HeaderOp->Header.Op == OP_IRHEADER, "First op wasn't IRHeader");

  OrderedNode *BlockNode = HeaderOp->Blocks.GetNode(ListBegin);
  int8_t CurrentIndent = 0;
  auto AddIndent = [&out, &CurrentIndent]() {
    for (uint8_t i = 0; i < CurrentIndent; ++i) {
      *out << "\t";
    }
  };

  ++CurrentIndent;
  AddIndent();
  *out << "(%ssa" << std::to_string(RealNode->Wrapped(ListBegin).ID()) << ") " << "IRHeader ";
  *out << "#0x" << std::hex << HeaderOp->Entry << ", ";
  *out << "%ssa" << HeaderOp->Blocks.ID() << ", ";
  *out << "#" << std::dec << HeaderOp->BlockCount << std::endl;

  while (1) {
    auto BlockIROp = BlockNode->Op(DataBegin)->CW<FEXCore::IR::IROp_CodeBlock>();
    LogMan::Throw::A(BlockIROp->Header.Op == OP_CODEBLOCK, "IR type failed to be a code block");

    // We grab these nodes this way so we can iterate easily
    auto CodeBegin = IR->at(BlockIROp->Begin);
    auto CodeLast = IR->at(BlockIROp->Last);

    AddIndent();
    *out << "(%ssa" << std::to_string(BlockNode->Wrapped(ListBegin).ID()) << ") " << "CodeBlock ";

    *out << "%ssa" << std::to_string(BlockIROp->Begin.ID()) << ", ";
    *out << "%ssa" << std::to_string(BlockIROp->Last.ID()) << ", ";
    *out << "%ssa" << std::to_string(BlockIROp->Next.ID()) << std::endl;

    ++CurrentIndent;
    while (1) {
      OrderedNodeWrapper *CodeOp = CodeBegin();
      OrderedNode *CodeNode = CodeOp->GetNode(ListBegin);
      auto IROp = CodeNode->Op(DataBegin);

      auto Name = FEXCore::IR::GetName(IROp->Op);
      bool Skip{};
      switch (IROp->Op) {
        case IR::OP_PHIVALUE:
          Skip = true;
          break;
        default: break;
      }

      if (!Skip) {
        AddIndent();
        if (IROp->HasDest) {

          uint32_t ElementSize = IROp->ElementSize;
          if (!IROp->ElementSize) {
            ElementSize = IROp->Size;
          }
          uint32_t NumElements = IROp->Size / ElementSize;

          *out << "%ssa" << std::to_string(CodeOp->ID());

          if (RAPass) {
            uint64_t RegClass = RAPass->GetNodeRegister(CodeOp->ID());
            FEXCore::IR::RegisterClassType Class {uint32_t(RegClass >> 32)};
            uint32_t Reg = RegClass;
            switch (Class) {
              case FEXCore::IR::GPRClass.Val: *out << "(GPR"; break;
              case FEXCore::IR::FPRClass.Val: *out << "(FPR"; break;
              case FEXCore::IR::ComplexClass.Val: *out << "(Complex"; break;
            }

            *out << std::dec << Reg << ")";
          }

          *out << " i" << std::dec << (ElementSize * 8);

          if (NumElements > 1) {
            *out << "v" << std::dec << NumElements;
          }

          *out << " = ";
        }
        else {

          uint32_t ElementSize = IROp->ElementSize;
          if (!IROp->ElementSize) {
            ElementSize = IROp->Size;
          }
          uint32_t NumElements = 0;
          if (ElementSize) {
            NumElements = IROp->Size / ElementSize;
          }

          *out << "(%ssa" << std::to_string(CodeOp->ID()) << " ";
          *out << "i" << std::dec << (ElementSize * 8);
          if (NumElements > 1) {
            *out << "v" << std::dec << NumElements;
          }
          *out << ") ";
        }
        *out << Name;
        switch (IROp->Op) {
          case IR::OP_BEGINBLOCK:
            *out << " %ssa" << std::to_string(CodeOp->ID());
            break;
          case IR::OP_ENDBLOCK:
            break;
          default: break;
        }

        #define IROP_ARGPRINTER_HELPER
        #include <FEXCore/IR/IRDefines.inc>
        case IR::OP_PHI: {
          auto Op = IROp->C<IR::IROp_Phi>();
          auto NodeBegin = IR->at(Op->PhiBegin);
          *out << " ";

          while (NodeBegin != NodeBegin.Invalid()) {
            OrderedNodeWrapper *NodeOp = NodeBegin();
            OrderedNode *NodeNode = NodeOp->GetNode(ListBegin);
            auto IRNodeOp  = NodeNode->Op(DataBegin)->C<IR::IROp_PhiValue>();
            *out << "[ ";
            PrintArg(out, IR, IRNodeOp->Value, RAPass);
            *out << ", ";
            PrintArg(out, IR, IRNodeOp->Block, RAPass);
            *out << " ]";

            if (IRNodeOp->Next.ID())
              *out << ", ";

            NodeBegin = IR->at(IRNodeOp->Next);
          }
          break;
        }
        default: *out << "<Unknown Args>"; break;
        }

        //*out << " (" <<  std::dec << CodeNode->GetUses() << ")";

        *out << "\n";
      }

      // CodeLast is inclusive. So we still need to dump the CodeLast op as well
      if (CodeBegin == CodeLast) {
        break;
      }
      ++CodeBegin;
    }

    CurrentIndent = std::max(0, CurrentIndent - 1);

    if (BlockIROp->Next.ID() == 0) {
      break;
    } else {
      BlockNode = BlockIROp->Next.GetNode(ListBegin);
    }
  }
}

}
