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

static void PrintArg(std::stringstream *out, [[maybe_unused]] IRListView<false> const* IR, const char* Arg) {
  *out <<  Arg;
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

static void PrintArg(std::stringstream *out, [[maybe_unused]] IRListView<false> const* IR, MemOffsetType Arg) {
  std::array<std::string, 3> Names = {
    "SXTX",
    "UXTW",
    "SXTW",
  };

  *out << Names[Arg];
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
  auto [CodeNode, IROp] = IR->at(Arg)();

  *out << "%ssa" << std::to_string(Arg.ID());
  if (RAPass) {
    uint64_t RegClass = RAPass->GetNodeRegister(Arg.ID());
    FEXCore::IR::RegisterClassType Class {uint32_t(RegClass >> 32)};
    uint32_t Reg = RegClass;
    switch (Class) {
      case FEXCore::IR::GPRClass.Val: *out << "(GPR"; break;
      case FEXCore::IR::FPRClass.Val: *out << "(FPR"; break;
      case FEXCore::IR::GPRPairClass.Val: *out << "(GPRPair"; break;
      case FEXCore::IR::ComplexClass.Val: *out << "(Complex"; break;
      case FEXCore::IR::InvalidClass.Val: *out << "(Invalid"; break;
      default: *out << "(Unknown"; break;
    }

    *out << std::dec << Reg << ")";
  }

  if (IROp->HasDest) {
    uint32_t ElementSize = IROp->ElementSize;
    uint32_t NumElements = IROp->Size;
    if (!IROp->ElementSize) {
      ElementSize = IROp->Size;
    }

    if (ElementSize) {
      NumElements /= ElementSize;
    }

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

static void PrintArg(std::stringstream *out, [[maybe_unused]] IRListView<false> const* IR, FEXCore::IR::FenceType Arg) {
  if (Arg == IR::Fence_Load) {
    *out << "Loads";
  }
  else if (Arg == IR::Fence_Store) {
    *out << "Stores";
  }
  else if (Arg == IR::Fence_LoadStore) {
    *out << "LoadStores";
  }
  else {
    *out << "<Unknown Fence Type>";
  }
}

void Dump(std::stringstream *out, IRListView<false> const* IR, IR::RegisterAllocationPass *RAPass) {
  auto HeaderOp = IR->GetHeader();

  int8_t CurrentIndent = 0;
  auto AddIndent = [&out, &CurrentIndent]() {
    for (uint8_t i = 0; i < CurrentIndent; ++i) {
      *out << "\t";
    }
  };

  ++CurrentIndent;
  AddIndent();
  *out << "(%ssa0) " << "IRHeader ";
  *out << "#0x" << std::hex << HeaderOp->Entry << ", ";
  *out << "%ssa" << HeaderOp->Blocks.ID() << ", ";
  *out << "#" << std::dec << HeaderOp->BlockCount << std::endl;

  for (auto [BlockNode, BlockHeader] : IR->GetBlocks()) {
    {
      auto BlockIROp = BlockHeader->C<FEXCore::IR::IROp_CodeBlock>();

      AddIndent();
      *out << "(%ssa" << std::to_string(IR->GetID(BlockNode)) << ") " << "CodeBlock ";

      *out << "%ssa" << std::to_string(BlockIROp->Begin.ID()) << ", ";
      *out << "%ssa" << std::to_string(BlockIROp->Last.ID()) << std::endl;
    }

    ++CurrentIndent;
    for (auto [CodeNode, IROp] : IR->GetCode(BlockNode)) {
      uint32_t ID = IR->GetID(CodeNode);

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
          uint32_t NumElements = IROp->Size;
          if (!IROp->ElementSize) {
            ElementSize = IROp->Size;
          }

          if (ElementSize) {
            NumElements /= ElementSize;
          }

          *out << "%ssa" << std::to_string(ID);

          if (RAPass) {
            uint64_t RegClass = RAPass->GetNodeRegister(ID);
            FEXCore::IR::RegisterClassType Class {uint32_t(RegClass >> 32)};
            uint32_t Reg = RegClass;
            switch (Class) {
              case FEXCore::IR::GPRClass.Val: *out << "(GPR"; break;
              case FEXCore::IR::FPRClass.Val: *out << "(FPR"; break;
              case FEXCore::IR::GPRPairClass.Val: *out << "(GPRPair"; break;
              case FEXCore::IR::ComplexClass.Val: *out << "(Complex"; break;
              case FEXCore::IR::InvalidClass.Val: *out << "(Invalid"; break;
              default: *out << "(Unknown"; break;
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

          *out << "(%ssa" << std::to_string(ID) << " ";
          *out << "i" << std::dec << (ElementSize * 8);
          if (NumElements > 1) {
            *out << "v" << std::dec << NumElements;
          }
          *out << ") ";
        }
        *out << Name;

        #define IROP_ARGPRINTER_HELPER
        #include <FEXCore/IR/IRDefines.inc>
        case IR::OP_PHI: {
          auto Op = IROp->C<IR::IROp_Phi>();
          auto NodeBegin = IR->at(Op->PhiBegin);
          *out << " ";

          while (NodeBegin != NodeBegin.Invalid()) {
            auto [NodeNode, IROp] = NodeBegin();
            auto PhiOp  = IROp->C<IR::IROp_PhiValue>();
            *out << "[ ";
            PrintArg(out, IR, PhiOp->Value, RAPass);
            *out << ", ";
            PrintArg(out, IR, PhiOp->Block, RAPass);
            *out << " ]";

            if (PhiOp->Next.ID())
              *out << ", ";

            NodeBegin = IR->at(PhiOp->Next);
          }
          break;
        }
        default: *out << "<Unknown Args>"; break;
        }

        //*out << " (" <<  std::dec << CodeNode->GetUses() << ")";

        *out << "\n";
      }
    }

    CurrentIndent = std::max(0, CurrentIndent - 1);
  }
}

}
