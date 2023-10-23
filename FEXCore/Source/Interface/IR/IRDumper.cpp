// SPDX-License-Identifier: MIT
/*
$info$
meta: ir|dumper ~ IR -> Text
tags: ir|dumper
$end_info$
*/

#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/IR/RegisterAllocationData.h>
#include <FEXCore/fextl/sstream.h>

#include <algorithm>
#include <array>
#include <ostream>
#include <stdint.h>
#include <string_view>
#include  <iomanip>

namespace FEXCore::IR {
#define IROP_GETNAME_IMPL
#define IROP_GETRAARGS_IMPL
#define IROP_REG_CLASSES_IMPL
#define IROP_HASSIDEEFFECTS_IMPL
#define IROP_SIZES_IMPL
#define IROP_GETHASDEST_IMPL

#include <FEXCore/IR/IRDefines.inc>

static void PrintArg(fextl::stringstream *out, [[maybe_unused]] IRListView const* IR, const SHA256Sum &Arg) {
  *out << "sha256:";
  for(auto byte: Arg.data)
    *out << std::hex << std::setfill('0') << std::setw(2) << (unsigned int)byte;
}

static void PrintArg(fextl::stringstream *out, [[maybe_unused]] IRListView const* IR, uint64_t Arg) {
  *out << "#0x" << std::hex << Arg;
}

[[maybe_unused]]
static void PrintArg(fextl::stringstream *out, [[maybe_unused]] IRListView const* IR, const char* Arg) {
  *out <<  Arg;
}

static void PrintArg(fextl::stringstream *out, [[maybe_unused]] IRListView const* IR, CondClassType Arg) {
  static constexpr std::array<std::string_view, 22> CondNames = {
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
    "ANDZ",
    "ANDNZ",
    "FLU",
    "FGE",
    "FLEU",
    "FGT",
    "FU",
    "FNU"
  };

  *out << CondNames[Arg];
}

static void PrintArg(fextl::stringstream *out, [[maybe_unused]] IRListView const* IR, MemOffsetType Arg) {
  static constexpr std::array<std::string_view, 3> Names = {
    "SXTX",
    "UXTW",
    "SXTW",
  };

  *out << Names[Arg];
}

static void PrintArg(fextl::stringstream *out, [[maybe_unused]] IRListView const* IR, RegisterClassType Arg) {
  if (Arg == GPRClass.Val)
    *out << "GPR";
  else if (Arg == GPRFixedClass.Val)
    *out << "GPRFixed";
  else if (Arg == FPRClass.Val)
    *out << "FPR";
  else if (Arg == FPRFixedClass.Val)
    *out << "FPRFixed";
  else if (Arg == GPRPairClass.Val)
    *out << "GPRPair";
  else
    *out << "Unknown Registerclass " << Arg;
}

static void PrintArg(fextl::stringstream *out, IRListView const* IR, OrderedNodeWrapper Arg, IR::RegisterAllocationData *RAData) {
  auto [CodeNode, IROp] = IR->at(Arg)();
  const auto ArgID = Arg.ID();

  if (ArgID.IsInvalid()) {
    *out << "%Invalid";
  } else {
    *out << "%" << std::dec << ArgID;
    if (RAData) {
      auto PhyReg = RAData->GetNodeRegister(ArgID);

      switch (PhyReg.Class) {
        case FEXCore::IR::GPRClass.Val: *out << "(GPR"; break;
        case FEXCore::IR::GPRFixedClass.Val: *out << "(GPRFixed"; break;
        case FEXCore::IR::FPRClass.Val: *out << "(FPR"; break;
        case FEXCore::IR::FPRFixedClass.Val: *out << "(FPRFixed"; break;
        case FEXCore::IR::GPRPairClass.Val: *out << "(GPRPair"; break;
        case FEXCore::IR::ComplexClass.Val: *out << "(Complex"; break;
        case FEXCore::IR::InvalidClass.Val: *out << "(Invalid"; break;
        default: *out << "(Unknown"; break;
      }

      if (PhyReg.Class != FEXCore::IR::InvalidClass.Val) {
        *out << std::dec << (uint32_t)PhyReg.Reg << ")";
      } else {
        *out << ")";
      }
    }
  }

  if (GetHasDest(IROp->Op)) {
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

static void PrintArg(fextl::stringstream *out, [[maybe_unused]] IRListView const* IR, FEXCore::IR::FenceType Arg) {
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

static void PrintArg(fextl::stringstream *out, [[maybe_unused]] IRListView const* IR, FEXCore::IR::RoundType Arg) {
  switch (Arg) {
    case FEXCore::IR::Round_Nearest: *out << "Nearest"; break;
    case FEXCore::IR::Round_Negative_Infinity: *out << "-Inf"; break;
    case FEXCore::IR::Round_Positive_Infinity: *out << "+Inf"; break;
    case FEXCore::IR::Round_Towards_Zero: *out << "Towards Zero"; break;
    case FEXCore::IR::Round_Host: *out << "Host"; break;
    default: *out << "<Unknown Round Type>"; break;
  }
}

static void PrintArg(fextl::stringstream *out, [[maybe_unused]] IRListView const* IR, FEXCore::IR::SyscallFlags Arg) {
  switch (Arg) {
    case FEXCore::IR::SyscallFlags::DEFAULT: *out << "Default"; break;
    case FEXCore::IR::SyscallFlags::OPTIMIZETHROUGH: *out << "Optimize Through"; break;
    case FEXCore::IR::SyscallFlags::NOSYNCSTATEONENTRY: *out << "No Sync State on Entry"; break;
    case FEXCore::IR::SyscallFlags::NORETURN: *out << "No Return"; break;
    case FEXCore::IR::SyscallFlags::NOSIDEEFFECTS: *out << "No Side Effects"; break;
    default: *out << "<Unknown Round Type>"; break;
  }
}

static void PrintArg(fextl::stringstream *out, [[maybe_unused]] IRListView const* IR, FEXCore::IR::NamedVectorConstant Arg) {
  switch (Arg) {
    case FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_INCREMENTAL_U16_INDEX: {
      *out << "u16_incremental_index";
      break;
    }
    case FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_INCREMENTAL_U16_INDEX_UPPER: {
      *out << "u16_incremental_index_upper";
      break;
    }
    case FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_PADDSUBPS_INVERT: {
      *out << "addsubps_invert";
      break;
    }
    case FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_PADDSUBPS_INVERT_UPPER: {
      *out << "addsubps_invert_upper";
      break;
    }
    case FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_PADDSUBPD_INVERT: {
      *out << "addsubpd_invert";
      break;
    }
    case FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_PADDSUBPD_INVERT_UPPER: {
      *out << "addsubpd_invert_upper";
      break;
    }
    case FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_MOVMSKPS_SHIFT: {
      *out << "movmskps_shift";
      break;
    }
    case FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_AESKEYGENASSIST_SWIZZLE: {
      *out << "aeskeygenassist_swizzle";
      break;
    }
    case FEXCore::IR::NamedVectorConstant::NAMED_VECTOR_ZERO: {
      *out << "vectorzero";
      break;
    }
    default: *out << "<Unknown Named Vector Constant>"; break;
  }
}

static void PrintArg(fextl::stringstream *out, [[maybe_unused]] IRListView const* IR, FEXCore::IR::OpSize Arg) {
  switch (Arg) {
    case OpSize::i8Bit: *out << "i8"; break;
    case OpSize::i16Bit: *out << "i16"; break;
    case OpSize::i32Bit: *out << "i32"; break;
    case OpSize::i64Bit: *out << "i64"; break;
    case OpSize::i128Bit: *out << "i128"; break;
    case OpSize::i256Bit: *out << "i256"; break;
    default: *out << "<Unknown OpSize Type>"; break;
  }
}

static void PrintArg(fextl::stringstream *out, [[maybe_unused]] IRListView const* IR, FEXCore::IR::FloatCompareOp Arg) {
  switch (Arg) {
    case FloatCompareOp::EQ:  *out << "FEQ"; break;
    case FloatCompareOp::LT:  *out << "FLT"; break;
    case FloatCompareOp::LE:  *out << "FLE"; break;
    case FloatCompareOp::UNO: *out << "UNO"; break;
    case FloatCompareOp::NEQ: *out << "NEQ"; break;
    case FloatCompareOp::ORD: *out << "ORD"; break;
    default: *out << "<Unknown OpSize Type>"; break;
  }
}

static void PrintArg(fextl::stringstream *out, [[maybe_unused]] IRListView const* IR, FEXCore::IR::BreakDefinition Arg) {
  *out << "{" << Arg.ErrorRegister << ".";
  *out << static_cast<uint32_t>(Arg.Signal) << ".";
  *out << static_cast<uint32_t>(Arg.TrapNumber) << ".";
  *out << static_cast<uint32_t>(Arg.si_code) << "}";
}

static void PrintArg(fextl::stringstream *out, [[maybe_unused]] IRListView const* IR, FEXCore::IR::ShiftType Arg) {
  switch (Arg) {
    case ShiftType::LSL:  *out << "LSL"; break;
    case ShiftType::LSR:  *out << "LSR"; break;
    case ShiftType::ASR:  *out << "ASR"; break;
    case ShiftType::ROR:  *out << "ROR"; break;
    default: *out << "<Unknown Shift Type>"; break;
  }
}

void Dump(fextl::stringstream *out, IRListView const* IR, IR::RegisterAllocationData *RAData) {
  auto HeaderOp = IR->GetHeader();

  int8_t CurrentIndent = 0;
  auto AddIndent = [&out, &CurrentIndent]() {
    for (uint8_t i = 0; i < CurrentIndent; ++i) {
      *out << "\t";
    }
  };

  ++CurrentIndent;
  AddIndent();
  *out << "(%0) " << "IRHeader ";
  *out << "%" << HeaderOp->Blocks.ID() << ", ";
  *out << "#" << std::dec << HeaderOp->OriginalRIP << ", ";
  *out << "#" << std::dec << HeaderOp->BlockCount << ", ";
  *out << "#" << std::dec << HeaderOp->NumHostInstructions << std::endl;

  for (auto [BlockNode, BlockHeader] : IR->GetBlocks()) {
    {
      auto BlockIROp = BlockHeader->C<FEXCore::IR::IROp_CodeBlock>();

      AddIndent();
      *out << "(%" << IR->GetID(BlockNode) << ") " << "CodeBlock ";

      *out << "%" << BlockIROp->Begin.ID() << ", ";
      *out << "%" << BlockIROp->Last.ID() << std::endl;
    }

    ++CurrentIndent;
    for (auto [CodeNode, IROp] : IR->GetCode(BlockNode)) {
      const auto ID = IR->GetID(CodeNode);
      const auto Name = FEXCore::IR::GetName(IROp->Op);

      {
        AddIndent();
        if (GetHasDest(IROp->Op)) {

          uint32_t ElementSize = IROp->ElementSize;
          uint32_t NumElements = IROp->Size;
          if (!IROp->ElementSize) {
            ElementSize = IROp->Size;
          }

          if (ElementSize) {
            NumElements /= ElementSize;
          }

          *out << "%" << std::dec << ID;

          if (RAData) {
            auto PhyReg = RAData->GetNodeRegister(ID);
            switch (PhyReg.Class) {
              case FEXCore::IR::GPRClass.Val: *out << "(GPR"; break;
              case FEXCore::IR::GPRFixedClass.Val: *out << "(GPRFixed"; break;
              case FEXCore::IR::FPRClass.Val: *out << "(FPR"; break;
              case FEXCore::IR::FPRFixedClass.Val: *out << "(FPRFixed"; break;
              case FEXCore::IR::GPRPairClass.Val: *out << "(GPRPair"; break;
              case FEXCore::IR::ComplexClass.Val: *out << "(Complex"; break;
              case FEXCore::IR::InvalidClass.Val: *out << "(Invalid"; break;
              default: *out << "(Unknown"; break;
            }
            if (PhyReg.Class != FEXCore::IR::InvalidClass.Val) {
              *out << std::dec << (uint32_t)PhyReg.Reg << ")";
            } else {
              *out << ")";
            }
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

          *out << "(%" << std::dec << ID << ' ';
          *out << 'i' << std::dec << (ElementSize * 8);
          if (NumElements > 1) {
            *out << 'v' << std::dec << NumElements;
          }
          *out << ") ";
        }
        *out << Name;

        #define IROP_ARGPRINTER_HELPER
        #include <FEXCore/IR/IRDefines.inc>
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
