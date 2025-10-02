// SPDX-License-Identifier: MIT
/*
$info$
meta: ir|dumper ~ IR -> Text
tags: ir|dumper
$end_info$
*/

#include "Interface/IR/IntrusiveIRList.h"
#include "Interface/IR/RegisterAllocationData.h"

#include <FEXCore/IR/IR.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/sstream.h>

#include <algorithm>
#include <array>
#include <ostream>
#include <stdint.h>
#include <string_view>
#include <iomanip>

namespace FEXCore::IR {
#define IROP_GETNAME_IMPL
#define IROP_GETRAARGS_IMPL
#define IROP_REG_CLASSES_IMPL
#define IROP_HASSIDEEFFECTS_IMPL
#define IROP_SIZES_IMPL
#define IROP_GETHASDEST_IMPL

#include <FEXCore/IR/IRDefines.inc>

static void PrintArg(fextl::stringstream* out, const IRListView*, const SHA256Sum& Arg) {
  *out << fextl::fmt::format("sha256:{:02x}", fmt::join(Arg.data, ""));
}

static void PrintArg(fextl::stringstream* out, const IRListView*, uint64_t Arg) {
  *out << fextl::fmt::format("#{:#x}", Arg);
}

static void PrintArg(fextl::stringstream* out, const IRListView*, CondClass Arg) {
  if (Arg == CondClass::AL) {
    *out << "ALWAYS";
    return;
  }

  static constexpr std::array<std::string_view, 22> CondNames = {"EQ",  "NEQ", "UGE",  "ULT", "MI",  "PL",  "VS",   "VC",
                                                                 "UGT", "ULE", "SGE",  "SLT", "SGT", "SLE", "TSTZ", "TSTNZ",
                                                                 "FLU", "FGE", "FLEU", "FGT", "FU",  "FNU"};

  *out << CondNames[FEXCore::ToUnderlying(Arg)];
}

static void PrintArg(fextl::stringstream* out, const IRListView*, MemOffsetType Arg) {
  static constexpr std::array<std::string_view, 3> Names = {
    "SXTX",
    "UXTW",
    "SXTW",
  };

  *out << Names[FEXCore::ToUnderlying(Arg)];
}

static void PrintArg(fextl::stringstream* out, const IRListView*, RegisterClassType Arg) {
  if (Arg == GPRClass.Val) {
    *out << "GPR";
  } else if (Arg == GPRFixedClass.Val) {
    *out << "GPRFixed";
  } else if (Arg == FPRClass.Val) {
    *out << "FPR";
  } else if (Arg == FPRFixedClass.Val) {
    *out << "FPRFixed";
  } else {
    *out << "Unknown Registerclass " << Arg;
  }
}

static void PrintArg(fextl::stringstream* out, const IRListView* IR, OrderedNodeWrapper Arg) {
  if (Arg.IsImmediate()) {
    auto PhyReg = PhysicalRegister(Arg);

    switch (PhyReg.Class) {
    case FEXCore::IR::GPRClass.Val: *out << "r"; break;
    case FEXCore::IR::GPRFixedClass.Val: *out << "R"; break;
    case FEXCore::IR::FPRClass.Val: *out << "v"; break;
    case FEXCore::IR::FPRFixedClass.Val: *out << "V"; break;
    case FEXCore::IR::ComplexClass.Val: *out << "c"; break;
    case FEXCore::IR::InvalidClass.Val: *out << "invalid"; break;
    default: *out << "unknown"; break;
    }

    if (PhyReg.Class != FEXCore::IR::InvalidClass.Val) {
      *out << std::dec << (uint32_t)PhyReg.Reg;
    }

    return;
  }

  auto [CodeNode, IROp] = IR->at(Arg)();
  const auto ArgID = Arg.ID();

  if (ArgID.IsInvalid()) {
    *out << "%Invalid";
  } else {
    *out << "%" << std::dec << ArgID;
  }

  if (GetHasDest(IROp->Op)) {
    auto ElementSize = IROp->ElementSize;
    uint32_t NumElements = 0;
    if (IROp->ElementSize == OpSize::iUnsized) {
      ElementSize = IROp->Size;
    }

    if (ElementSize != OpSize::iUnsized) {
      NumElements = IR::NumElements(IROp->Size, ElementSize);
    }

    *out << " i" << std::dec << IR::OpSizeAsBits(ElementSize);

    if (NumElements > 1) {
      *out << "v" << std::dec << NumElements;
    }
  }
}

static void PrintArg(fextl::stringstream* out, const IRListView*, FenceType Arg) {
  *out << [Arg] {
    switch (Arg) {
    case FenceType::Load: return "Loads";
    case FenceType::Store: return "Stores";
    case FenceType::LoadStore: return "LoadStores";
    case FenceType::Inst: return "Instruction";
    }
    return "<Unknown Fence Type>";
  }();
}

static void PrintArg(fextl::stringstream* out, const IRListView*, RoundMode Arg) {
  *out << [Arg] {
    switch (Arg) {
    case RoundMode::Nearest: return "Nearest";
    case RoundMode::NegInfinity: return "-Inf";
    case RoundMode::PosInfinity: return "+Inf";
    case RoundMode::TowardsZero: return "Towards Zero";
    case RoundMode::Host: return "Host";
    }
    return "<Unknown Round Type>";
  }();
}

static void PrintArg(fextl::stringstream* out, const IRListView*, SyscallFlags Arg) {
  *out << [Arg] {
    switch (Arg) {
    case SyscallFlags::DEFAULT: return "Default";
    case SyscallFlags::OPTIMIZETHROUGH: return "Optimize Through";
    case SyscallFlags::NOSYNCSTATEONENTRY: return "No Sync State on Entry";
    case SyscallFlags::NORETURN: return "No Return";
    case SyscallFlags::NOSIDEEFFECTS: return "No Side Effects";
    case SyscallFlags::NORETURNEDRESULT: return "No Returned Result";
    }
    return "<Unknown Syscall Flags>";
  }();
}

static void PrintArg(fextl::stringstream* out, const IRListView*, NamedVectorConstant Arg) {
  *out << [Arg] {
    // clang-format off
    switch (Arg) {
      case NamedVectorConstant::NAMED_VECTOR_INCREMENTAL_U16_INDEX:
        return "u16_incremental_index";
      case NamedVectorConstant::NAMED_VECTOR_INCREMENTAL_U16_INDEX_UPPER:
        return "u16_incremental_index_upper";
      case NamedVectorConstant::NAMED_VECTOR_PADDSUBPS_INVERT:
        return "addsubps_invert";
      case NamedVectorConstant::NAMED_VECTOR_PADDSUBPS_INVERT_UPPER:
        return "addsubps_invert_upper";
      case NamedVectorConstant::NAMED_VECTOR_PADDSUBPD_INVERT:
        return "addsubpd_invert";
      case NamedVectorConstant::NAMED_VECTOR_PADDSUBPD_INVERT_UPPER:
        return "addsubpd_invert_upper";
      case NamedVectorConstant::NAMED_VECTOR_PSUBADDPS_INVERT:
        return "subaddps_invert";
      case NamedVectorConstant::NAMED_VECTOR_PSUBADDPS_INVERT_UPPER:
        return "subaddps_invert_upper";
      case NamedVectorConstant::NAMED_VECTOR_PSUBADDPD_INVERT:
        return "subaddpd_invert";
      case NamedVectorConstant::NAMED_VECTOR_PSUBADDPD_INVERT_UPPER:
        return "subaddpd_invert_upper";
      case NamedVectorConstant::NAMED_VECTOR_MOVMSKPS_SHIFT:
        return "movmskps_shift";
      case NamedVectorConstant::NAMED_VECTOR_AESKEYGENASSIST_SWIZZLE:
        return "aeskeygenassist_swizzle";
      case NamedVectorConstant::NAMED_VECTOR_ZERO:
        return "vectorzero";
      case NamedVectorConstant::NAMED_VECTOR_X87_ONE:
        return "x87_1_0";
      case NamedVectorConstant::NAMED_VECTOR_X87_LOG2_10:
        return "x87_log2_10";
      case NamedVectorConstant::NAMED_VECTOR_X87_LOG2_E:
        return "x87_log2_e";
      case NamedVectorConstant::NAMED_VECTOR_X87_PI:
        return "x87_pi";
      case NamedVectorConstant::NAMED_VECTOR_X87_LOG10_2:
        return "x87_log10_2";
      case NamedVectorConstant::NAMED_VECTOR_X87_LOG_2:
        return "x87_log2";
      case NamedVectorConstant::NAMED_VECTOR_CVTMAX_F32_I32:
        return "cvtmax_f32_i32";
      case NamedVectorConstant::NAMED_VECTOR_CVTMAX_F32_I32_UPPER:
        return "cvtmax_f32_i32_upper";
      case NamedVectorConstant::NAMED_VECTOR_CVTMAX_F32_I64:
        return "cvtmax_f32_i64";
      case NamedVectorConstant::NAMED_VECTOR_CVTMAX_F64_I32:
        return "cvtmax_f64_i32";
      case NamedVectorConstant::NAMED_VECTOR_CVTMAX_F64_I32_UPPER:
        return "cvtmax_f64_i32_upper";
      case NamedVectorConstant::NAMED_VECTOR_CVTMAX_F64_I64:
        return "cvtmax_f64_i64";
      case NamedVectorConstant::NAMED_VECTOR_CVTMAX_I32:
        return "cvtmax_i32";
      case NamedVectorConstant::NAMED_VECTOR_CVTMAX_I64:
        return "cvtmax_i64";
    }
    return "<Unknown Named Vector Constant>";
    // clang-format on
  }();
}

static void PrintArg(fextl::stringstream* out, const IRListView*, IndexNamedVectorConstant Arg) {
  *out << [Arg] {
    // clang-format off
    switch (Arg) {
    case IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_PSHUFLW:
      return "pshuflw";
    case IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_PSHUFHW:
      return "pshufhw";
    case IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_PSHUFD:
      return "pshufd";
    case IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_SHUFPS:
      return "shufps";
    case IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_DPPS_MASK:
      return "dpps_mask";
    case IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_DPPD_MASK:
      return "dppd_mask";
    case IndexNamedVectorConstant::INDEXED_NAMED_VECTOR_PBLENDW:
      return "pblendw";
    }
    return "<Unknown Indexed Named Vector Constant>";
    // clang-format on
  }();
}

static void PrintArg(fextl::stringstream* out, const IRListView*, OpSize Arg) {
  *out << [Arg] {
    switch (Arg) {
    case OpSize::iUnsized: return "Unsized";
    case OpSize::i8Bit: return "i8";
    case OpSize::i16Bit: return "i16";
    case OpSize::i32Bit: return "i32";
    case OpSize::i64Bit: return "i64";
    case OpSize::f80Bit: return "f80";
    case OpSize::i128Bit: return "i128";
    case OpSize::i256Bit: return "i256";
    case OpSize::iInvalid: return "Invalid";
    }
    return "<Unknown OpSize Type>";
  }();
}

static void PrintArg(fextl::stringstream* out, const IRListView*, FloatCompareOp Arg) {
  *out << [Arg] {
    switch (Arg) {
    case FloatCompareOp::EQ: return "FEQ";
    case FloatCompareOp::LT: return "FLT";
    case FloatCompareOp::LE: return "FLE";
    case FloatCompareOp::UNO: return "UNO";
    case FloatCompareOp::NEQ: return "NEQ";
    case FloatCompareOp::ORD: return "ORD";
    }
    return "<Unknown FloatCompareOp Type>";
  }();
}

static void PrintArg(fextl::stringstream* out, const IRListView*, FEXCore::IR::BreakDefinition Arg) {
  *out << "{" << Arg.ErrorRegister << ".";
  *out << static_cast<uint32_t>(Arg.Signal) << ".";
  *out << static_cast<uint32_t>(Arg.TrapNumber) << ".";
  *out << static_cast<uint32_t>(Arg.si_code) << "}";
}

static void PrintArg(fextl::stringstream* out, const IRListView*, ShiftType Arg) {
  *out << [Arg] {
    switch (Arg) {
    case ShiftType::LSL: return "LSL";
    case ShiftType::LSR: return "LSR";
    case ShiftType::ASR: return "ASR";
    case ShiftType::ROR: return "ROR";
    }
    return "<Unknown Shift Type>";
  }();
}

static void PrintArg(fextl::stringstream* out, const IRListView*, BranchHint Arg) {
  *out << [Arg] {
    switch (Arg) {
    case BranchHint::None: return "None";
    case BranchHint::Call: return "Call";
    case BranchHint::Return: return "Return";
    case BranchHint::CheckTF: return "CheckTF";
    }
    return "<Unknown Branch Hint>";
  }();
}

static void PrintArg(fextl::stringstream* out, const IRListView*, const std::array<uint8_t, 0x10>& Arg) {
  *out << fextl::fmt::format("{:02x}", fmt::join(Arg, ""));
}

void Dump(fextl::stringstream* out, const IRListView* IR) {
  auto HeaderOp = IR->GetHeader();

  int8_t CurrentIndent = 0;
  auto AddIndent = [&out, &CurrentIndent]() {
    for (uint8_t i = 0; i < CurrentIndent; ++i) {
      *out << "\t";
    }
  };

  ++CurrentIndent;
  AddIndent();
  *out << fextl::fmt::format("(%0) IRHeader %{}, #{:#x}, #{}, #{}\n", HeaderOp->Blocks.ID(), HeaderOp->OriginalRIP, HeaderOp->BlockCount,
                             HeaderOp->NumHostInstructions);

  for (auto [BlockNode, BlockHeader] : IR->GetBlocks()) {
    {
      auto BlockIROp = BlockHeader->C<FEXCore::IR::IROp_CodeBlock>();

      AddIndent();
      *out << "(%" << IR->GetID(BlockNode) << ") "
           << "CodeBlock ";

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

          auto ElementSize = IROp->ElementSize;
          uint8_t NumElements = 0;
          if (IROp->ElementSize != OpSize::iUnsized) {
            ElementSize = IROp->Size;
          }

          if (ElementSize != OpSize::iUnsized) {
            NumElements = IR::NumElements(IROp->Size, ElementSize);
          }

          *out << "%" << std::dec << ID;

          auto PhyReg = PhysicalRegister(CodeNode);
          if (!PhyReg.IsInvalid()) {
            switch (PhyReg.Class) {
            case FEXCore::IR::GPRClass.Val: *out << "(r"; break;
            case FEXCore::IR::GPRFixedClass.Val: *out << "(R"; break;
            case FEXCore::IR::FPRClass.Val: *out << "(v"; break;
            case FEXCore::IR::FPRFixedClass.Val: *out << "(V"; break;
            case FEXCore::IR::ComplexClass.Val: *out << "(complex"; break;
            case FEXCore::IR::InvalidClass.Val: *out << "(invalid"; break;
            default: *out << "(unknown"; break;
            }
            if (PhyReg.Class != FEXCore::IR::InvalidClass.Val) {
              *out << std::dec << (uint32_t)PhyReg.Reg << ")";
            } else {
              *out << ")";
            }
          }

          *out << " i" << std::dec << IR::OpSizeAsBits(ElementSize);

          if (NumElements > 1) {
            *out << "v" << std::dec << NumElements;
          }

          *out << " = ";
        } else {

          auto ElementSize = IROp->ElementSize;
          if (IROp->ElementSize == OpSize::iUnsized) {
            ElementSize = IROp->Size;
          }
          uint32_t NumElements = 0;
          if (ElementSize != OpSize::iUnsized) {
            NumElements = IR::NumElements(IROp->Size, ElementSize);
          }

          *out << "(%" << std::dec << ID << ' ';
          *out << 'i' << std::dec << IR::OpSizeAsBits(ElementSize);
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
