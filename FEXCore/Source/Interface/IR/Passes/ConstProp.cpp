// SPDX-License-Identifier: MIT
/*
$info$
tags: ir|opts
desc: ConstProp, ZExt elim, const pooling, fcmp reduction, const inlining
$end_info$
*/


// aarch64 heuristics
#include "aarch64/assembler-aarch64.h"
#include "aarch64/cpu-aarch64.h"
#include "aarch64/disasm-aarch64.h"
#include "aarch64/assembler-aarch64.h"

#include "Interface/IR/IREmitter.h"
#include "Interface/IR/PassManager.h"
#include "Interface/Core/CPUID.h"

#include <FEXCore/IR/IR.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/Profiler.h>
#include <FEXCore/fextl/map.h>
#include <FEXCore/fextl/robin_map.h>
#include <FEXCore/fextl/unordered_map.h>

#include <bit>
#include <cstdint>
#include <memory>
#include <optional>
#include <string.h>
#include <tuple>
#include <utility>

namespace FEXCore::IR {

uint64_t getMask(IROp_Header* Op) {
  uint64_t NumBits = Op->Size * 8;
  return (~0ULL) >> (64 - NumBits);
}

// Returns true if the number bits from [0:width) contain the same bit.
// Ensuring that the consecutive bits in the range are entirely 0 or 1.
static bool HasConsecutiveBits(uint64_t imm, unsigned width) {
  if (width == 0) {
    return true;
  }

  // Credit to https://github.com/dougallj for this implementation.
  return ((imm ^ (imm >> 1)) & ((1ULL << (width - 1)) - 1)) == 0;
}

// aarch64 heuristics
static bool IsImmLogical(uint64_t imm, unsigned width) {
  if (width < 32) {
    width = 32;
  }
  return vixl::aarch64::Assembler::IsImmLogical(imm, width);
}
static bool IsImmAddSub(uint64_t imm) {
  return vixl::aarch64::Assembler::IsImmAddSub(imm);
}

static bool IsBfeAlreadyDone(IREmitter* IREmit, OrderedNodeWrapper src, uint64_t Width) {
  auto IROp = IREmit->GetOpHeader(src);
  if (IROp->Op == OP_BFE) {
    auto Op = IROp->C<IR::IROp_Bfe>();
    if (Width >= Op->Width) {
      return true;
    }
  }
  return false;
}

class ConstProp final : public FEXCore::IR::Pass {
public:
  explicit ConstProp(bool SupportsTSOImm9, const FEXCore::CPUIDEmu* CPUID)
    : SupportsTSOImm9 {SupportsTSOImm9}
    , CPUID {CPUID} {}

  void Run(IREmitter* IREmit) override;

private:
  void HandleConstantPools(IREmitter* IREmit, const IRListView& CurrentIR);
  void ConstantPropagation(IREmitter* IREmit, const IRListView& CurrentIR, Ref CodeNode, IROp_Header* IROp);
  void ConstantInlining(IREmitter* IREmit, const IRListView& CurrentIR);

  fextl::unordered_map<uint64_t, Ref> ConstPool;

  // Pool inline constant generation. These are typically very small and pool efficiently.
  fextl::robin_map<uint64_t, Ref> InlineConstantGen;
  Ref CreateInlineConstant(IREmitter* IREmit, uint64_t Constant) {
    const auto it = InlineConstantGen.find(Constant);
    if (it != InlineConstantGen.end()) {
      return it->second;
    }
    auto Result = InlineConstantGen.insert_or_assign(Constant, IREmit->_InlineConstant(Constant));
    return Result.first->second;
  }
  bool SupportsTSOImm9 {};
  const FEXCore::CPUIDEmu* CPUID;

  void InlineMemImmediate(IREmitter* IREmit, const IRListView& IR, Ref CodeNode, IROp_Header* IROp, OrderedNodeWrapper Offset,
                          MemOffsetType OffsetType, const size_t Offset_Index, uint8_t& OffsetScale, bool TSO) {
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
    bool IsExtended = (Imm & (IROp->Size - 1)) == 0 && Imm / IROp->Size <= 4095;
    IsExtended &= !TSO;

    if (IsSIMM9 || IsExtended) {
      IREmit->SetWriteCursor(IR.GetNode(Offset));
      IREmit->ReplaceNodeArgument(CodeNode, Offset_Index, CreateInlineConstant(IREmit, Imm));
      OffsetScale = 1;
    }
  }
};

// Constants are pooled per block.
void ConstProp::HandleConstantPools(IREmitter* IREmit, const IRListView& CurrentIR) {
  for (auto [BlockNode, BlockIROp] : CurrentIR.GetBlocks()) {
    for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {
      if (IROp->Op == OP_CONSTANT) {
        auto Op = IROp->C<IR::IROp_Constant>();
        auto it = ConstPool.find(Op->Constant);

        if (it != ConstPool.end()) {
          auto CodeIter = CurrentIR.at(CodeNode);
          IREmit->ReplaceUsesWithAfter(CodeNode, it->second, CodeIter);
        } else {
          ConstPool[Op->Constant] = CodeNode;
        }
      }
    }
    ConstPool.clear();
  }
}

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
    bool IsConstant1 = IREmit->IsValueConstant(IROp->Args[0], &Constant1);
    bool IsConstant2 = IREmit->IsValueConstant(IROp->Args[1], &Constant2);

    if (IsConstant1 && IsConstant2 && IROp->Op == OP_ADD) {
      uint64_t NewConstant = (Constant1 + Constant2) & getMask(IROp);
      IREmit->ReplaceWithConstant(CodeNode, NewConstant);
    } else if (IsConstant1 && IsConstant2 && IROp->Op == OP_SUB) {
      uint64_t NewConstant = (Constant1 - Constant2) & getMask(IROp);
      IREmit->ReplaceWithConstant(CodeNode, NewConstant);
    } else if (IsConstant2 && !IsImmAddSub(Constant2) && IsImmAddSub(-Constant2)) {
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
    break;
  }
  case OP_SUBSHIFT: {
    auto Op = IROp->C<IR::IROp_SubShift>();

    uint64_t Constant1, Constant2;
    if (IREmit->IsValueConstant(IROp->Args[0], &Constant1) && IREmit->IsValueConstant(IROp->Args[1], &Constant2) &&
        Op->Shift == IR::ShiftType::LSL) {
      // Optimize the LSL case when we know both sources are constant.
      // This is a pattern that shows up with direction flag calculations if DF was set just before the operation.
      uint64_t NewConstant = (Constant1 - (Constant2 << Op->ShiftAmount)) & getMask(IROp);
      IREmit->ReplaceWithConstant(CodeNode, NewConstant);
    }
    break;
  }
  case OP_AND: {
    uint64_t Constant1 {};
    uint64_t Constant2 {};

    if (IREmit->IsValueConstant(IROp->Args[0], &Constant1) && IREmit->IsValueConstant(IROp->Args[1], &Constant2)) {
      uint64_t NewConstant = (Constant1 & Constant2) & getMask(IROp);
      IREmit->ReplaceWithConstant(CodeNode, NewConstant);
    } else if (Constant2 == 1) {
      // happens from flag calcs
      auto val = IREmit->GetOpHeader(IROp->Args[0]);

      uint64_t Constant3;
      if (val->Op == OP_SELECT && IREmit->IsValueConstant(val->Args[2], &Constant2) && IREmit->IsValueConstant(val->Args[3], &Constant3) &&
          Constant2 == 1 && Constant3 == 0) {
        IREmit->ReplaceAllUsesWith(CodeNode, CurrentIR.GetNode(IROp->Args[0]));
      }
    } else if (IROp->Args[0].ID() == IROp->Args[1].ID()) {
      // AND with same value results in original value
      IREmit->ReplaceAllUsesWith(CodeNode, CurrentIR.GetNode(IROp->Args[0]));
    }
    break;
  }
  case OP_OR: {
    uint64_t Constant1 {};
    uint64_t Constant2 {};

    if (IREmit->IsValueConstant(IROp->Args[0], &Constant1) && IREmit->IsValueConstant(IROp->Args[1], &Constant2)) {
      uint64_t NewConstant = Constant1 | Constant2;
      IREmit->ReplaceWithConstant(CodeNode, NewConstant);
    } else if (IROp->Args[0].ID() == IROp->Args[1].ID()) {
      // OR with same value results in original value
      IREmit->ReplaceAllUsesWith(CodeNode, CurrentIR.GetNode(IROp->Args[0]));
    }
    break;
  }
  case OP_XOR: {
    uint64_t Constant1 {};
    uint64_t Constant2 {};

    if (IREmit->IsValueConstant(IROp->Args[0], &Constant1) && IREmit->IsValueConstant(IROp->Args[1], &Constant2)) {
      uint64_t NewConstant = Constant1 ^ Constant2;
      IREmit->ReplaceWithConstant(CodeNode, NewConstant);
    } else if (IROp->Args[0].ID() == IROp->Args[1].ID()) {
      // XOR with same value results to zero
      IREmit->SetWriteCursor(CodeNode);
      IREmit->ReplaceAllUsesWith(CodeNode, IREmit->_Constant(0));
    } else {
      // XOR with zero results in the nonzero source
      for (unsigned i = 0; i < 2; ++i) {
        if (!IREmit->IsValueConstant(IROp->Args[i], &Constant1)) {
          continue;
        }

        if (Constant1 != 0) {
          continue;
        }

        IREmit->SetWriteCursor(CodeNode);
        Ref Arg = CurrentIR.GetNode(IROp->Args[1 - i]);
        IREmit->ReplaceAllUsesWith(CodeNode, Arg);
        break;
      }
    }
    break;
  }
  case OP_NEG: {
    uint64_t Constant {};

    if (IREmit->IsValueConstant(IROp->Args[0], &Constant)) {
      uint64_t NewConstant = -Constant;
      IREmit->ReplaceWithConstant(CodeNode, NewConstant);
    }
    break;
  }
  case OP_LSHL: {
    uint64_t Constant1 {};
    uint64_t Constant2 {};

    if (IREmit->IsValueConstant(IROp->Args[0], &Constant1) && IREmit->IsValueConstant(IROp->Args[1], &Constant2)) {
      // Shifts mask the shift amount by 63 or 31 depending on operating size;
      uint64_t ShiftMask = IROp->Size == 8 ? 63 : 31;
      uint64_t NewConstant = (Constant1 << (Constant2 & ShiftMask)) & getMask(IROp);
      IREmit->ReplaceWithConstant(CodeNode, NewConstant);
    } else if (IREmit->IsValueConstant(IROp->Args[1], &Constant2) && Constant2 == 0) {
      IREmit->SetWriteCursor(CodeNode);
      Ref Arg = CurrentIR.GetNode(IROp->Args[0]);
      IREmit->ReplaceAllUsesWith(CodeNode, Arg);
    }
    break;
  }
  case OP_LSHR: {
    uint64_t Constant1 {};
    uint64_t Constant2 {};

    if (IREmit->IsValueConstant(IROp->Args[0], &Constant1) && IREmit->IsValueConstant(IROp->Args[1], &Constant2)) {
      // Shifts mask the shift amount by 63 or 31 depending on operating size;
      // The source is masked, which will produce a correctly masked
      // destination. Masking the destination without the source instead will
      // right-shift garbage into the upper bits instead of zeroes.
      Constant1 &= getMask(IROp);
      Constant2 &= (IROp->Size == 8 ? 63 : 31);
      uint64_t NewConstant = (Constant1 >> Constant2);
      IREmit->ReplaceWithConstant(CodeNode, NewConstant);
    } else if (IREmit->IsValueConstant(IROp->Args[1], &Constant2) && Constant2 == 0) {
      IREmit->SetWriteCursor(CodeNode);
      Ref Arg = CurrentIR.GetNode(IROp->Args[0]);
      IREmit->ReplaceAllUsesWith(CodeNode, Arg);
    }
    break;
  }
  case OP_BFE: {
    auto Op = IROp->C<IR::IROp_Bfe>();
    uint64_t Constant;

    // Is this value already BFE'd?
    if (IsBfeAlreadyDone(IREmit, Op->Src, Op->Width)) {
      IREmit->ReplaceAllUsesWith(CodeNode, CurrentIR.GetNode(Op->Src));
      break;
    }

    // Is this value already ZEXT'd?
    if (Op->lsb == 0) {
      // LoadMem, LoadMemTSO & LoadContext ZExt
      auto source = Op->Src;
      auto sourceHeader = IREmit->GetOpHeader(source);

      if (Op->Width >= (sourceHeader->Size * 8) &&
          (sourceHeader->Op == OP_LOADMEM || sourceHeader->Op == OP_LOADMEMTSO || sourceHeader->Op == OP_LOADCONTEXT)) {
        //  Load mem / load ctx zexts, no need to vmem
        IREmit->ReplaceAllUsesWith(CodeNode, CurrentIR.GetNode(source));
        break;
      }
    }

    if (IROp->Size <= 8 && IREmit->IsValueConstant(Op->Src, &Constant)) {
      uint64_t SourceMask = Op->Width == 64 ? ~0ULL : ((1ULL << Op->Width) - 1);
      SourceMask <<= Op->lsb;

      uint64_t NewConstant = (Constant & SourceMask) >> Op->lsb;
      IREmit->ReplaceWithConstant(CodeNode, NewConstant);
    } else if (IROp->Size == CurrentIR.GetOp<IROp_Header>(IROp->Args[0])->Size && Op->Width == (IROp->Size * 8) && Op->lsb == 0) {
      // A BFE that extracts all bits results in original value
      // XXX - This is broken for now - see https://github.com/FEX-Emu/FEX/issues/351
      // IREmit->ReplaceAllUsesWith(CodeNode, CurrentIR.GetNode(IROp->Args[0]));
    } else if (Op->Width == 1 && Op->lsb == 0) {
      // common from flag codegen
      auto val = IREmit->GetOpHeader(IROp->Args[0]);

      uint64_t Constant2 {};
      uint64_t Constant3 {};
      if (val->Op == OP_SELECT && IREmit->IsValueConstant(val->Args[2], &Constant2) && IREmit->IsValueConstant(val->Args[3], &Constant3) &&
          Constant2 == 1 && Constant3 == 0) {
        IREmit->ReplaceAllUsesWith(CodeNode, CurrentIR.GetNode(IROp->Args[0]));
      }
    }

    break;
  }
  case OP_SBFE: {
    auto Op = IROp->C<IR::IROp_Bfe>();
    uint64_t Constant;
    if (IREmit->IsValueConstant(Op->Src, &Constant)) {
      // SBFE of a constant can be converted to a constant.
      uint64_t SourceMask = Op->Width == 64 ? ~0ULL : ((1ULL << Op->Width) - 1);
      uint64_t DestSizeInBits = IROp->Size * 8;
      uint64_t DestMask = DestSizeInBits == 64 ? ~0ULL : ((1ULL << DestSizeInBits) - 1);
      SourceMask <<= Op->lsb;

      int64_t NewConstant = (Constant & SourceMask) >> Op->lsb;
      NewConstant <<= 64 - Op->Width;
      NewConstant >>= 64 - Op->Width;
      NewConstant &= DestMask;
      IREmit->ReplaceWithConstant(CodeNode, NewConstant);
    }
    break;
  }
  case OP_BFI: {
    auto Op = IROp->C<IR::IROp_Bfi>();
    uint64_t ConstantDest {};
    uint64_t ConstantSrc {};
    bool DestIsConstant = IREmit->IsValueConstant(IROp->Args[0], &ConstantDest);
    bool SrcIsConstant = IREmit->IsValueConstant(IROp->Args[1], &ConstantSrc);

    if (DestIsConstant && SrcIsConstant) {
      uint64_t SourceMask = Op->Width == 64 ? ~0ULL : ((1ULL << Op->Width) - 1);
      uint64_t NewConstant = ConstantDest & ~(SourceMask << Op->lsb);
      NewConstant |= (ConstantSrc & SourceMask) << Op->lsb;

      IREmit->ReplaceWithConstant(CodeNode, NewConstant);
    } else if (SrcIsConstant && HasConsecutiveBits(ConstantSrc, Op->Width)) {
      // We are trying to insert constant, if it is a bitfield of only set bits then we can orr or and it.
      IREmit->SetWriteCursor(CodeNode);
      uint64_t SourceMask = Op->Width == 64 ? ~0ULL : ((1ULL << Op->Width) - 1);
      uint64_t NewConstant = SourceMask << Op->lsb;

      if (ConstantSrc & 1) {
        auto orr = IREmit->_Or(IR::SizeToOpSize(IROp->Size), CurrentIR.GetNode(IROp->Args[0]), IREmit->_Constant(NewConstant));
        IREmit->ReplaceAllUsesWith(CodeNode, orr);
      } else {
        // We are wanting to clear the bitfield.
        auto andn = IREmit->_Andn(IR::SizeToOpSize(IROp->Size), CurrentIR.GetNode(IROp->Args[0]), IREmit->_Constant(NewConstant));
        IREmit->ReplaceAllUsesWith(CodeNode, andn);
      }
    }
    break;
  }
  case OP_MUL: {
    uint64_t Constant1 {};
    uint64_t Constant2 {};

    if (IREmit->IsValueConstant(IROp->Args[0], &Constant1) && IREmit->IsValueConstant(IROp->Args[1], &Constant2)) {
      uint64_t NewConstant = (Constant1 * Constant2) & getMask(IROp);
      IREmit->ReplaceWithConstant(CodeNode, NewConstant);
    } else if (IREmit->IsValueConstant(IROp->Args[1], &Constant2) && std::popcount(Constant2) == 1) {
      if (IROp->Size == 4 || IROp->Size == 8) {
        uint64_t amt = std::countr_zero(Constant2);
        IREmit->SetWriteCursor(CodeNode);
        auto shift = IREmit->_Lshl(IR::SizeToOpSize(IROp->Size), CurrentIR.GetNode(IROp->Args[0]), IREmit->_Constant(amt));
        IREmit->ReplaceAllUsesWith(CodeNode, shift);
      }
    }
    break;
  }

  case OP_VMOV: {
    // elim from load mem
    auto source = IROp->Args[0];
    auto sourceHeader = IREmit->GetOpHeader(source);

    if (IROp->Size >= sourceHeader->Size &&
        (sourceHeader->Op == OP_LOADMEM || sourceHeader->Op == OP_LOADMEMTSO || sourceHeader->Op == OP_LOADCONTEXT)) {
      //  Load mem / load ctx zexts, no need to vmem
      IREmit->ReplaceAllUsesWith(CodeNode, CurrentIR.GetNode(source));
    }
    break;
  }

  case OP_SYSCALL: {
    auto Op = IROp->CW<IR::IROp_Syscall>();

    // Is the first argument a constant?
    uint64_t Constant;
    if (IREmit->IsValueConstant(Op->SyscallID, &Constant)) {
      auto SyscallDef = Manager->SyscallHandler->GetSyscallABI(Constant);
      auto SyscallFlags = Manager->SyscallHandler->GetSyscallFlags(Constant);

      // Update the syscall flags
      Op->Flags = SyscallFlags;

      // XXX: Once we have the ability to do real function calls then we can call directly in to the syscall handler
      if (SyscallDef.NumArgs < FEXCore::HLE::SyscallArguments::MAX_ARGS) {
        // If the number of args are less than what the IR op supports then we can remove arg usage
        // We need +1 since we are still passing in syscall number here
        for (uint8_t Arg = (SyscallDef.NumArgs + 1); Arg < FEXCore::HLE::SyscallArguments::MAX_ARGS; ++Arg) {
          IREmit->ReplaceNodeArgument(CodeNode, Arg, IREmit->Invalid());
        }
        // Replace syscall with inline passthrough syscall if we can
        if (SyscallDef.HostSyscallNumber != -1) {
          IREmit->SetWriteCursor(CodeNode);
          // Skip Args[0] since that is the syscallid
          auto InlineSyscall =
            IREmit->_InlineSyscall(CurrentIR.GetNode(IROp->Args[1]), CurrentIR.GetNode(IROp->Args[2]), CurrentIR.GetNode(IROp->Args[3]),
                                   CurrentIR.GetNode(IROp->Args[4]), CurrentIR.GetNode(IROp->Args[5]), CurrentIR.GetNode(IROp->Args[6]),
                                   SyscallDef.HostSyscallNumber, Op->Flags);

          // Replace all syscall uses with this inline one
          IREmit->ReplaceAllUsesWith(CodeNode, InlineSyscall);

          // We must remove here since DCE can't remove a IROp with sideeffects
          IREmit->Remove(CodeNode);
        }
      }
    }
    break;
  }

  case OP_CPUID: {
    auto Op = IROp->CW<IR::IROp_CPUID>();

    uint64_t ConstantFunction {}, ConstantLeaf {};
    bool IsConstantFunction = IREmit->IsValueConstant(Op->Function, &ConstantFunction);
    bool IsConstantLeaf = IREmit->IsValueConstant(Op->Leaf, &ConstantLeaf);
    // If the CPUID function is constant then we can try and optimize.
    if (IsConstantFunction) { // && ConstantFunction != 1) {
      // Check if it supports constant data reporting for this function.
      const auto SupportsConstant = CPUID->DoesFunctionReportConstantData(ConstantFunction);
      if (SupportsConstant.SupportsConstantFunction == CPUIDEmu::SupportsConstant::CONSTANT) {
        // If the CPUID needs a constant leaf to be optimized then this can't work if we didn't const-prop the leaf register.
        if (!(SupportsConstant.NeedsLeaf == CPUIDEmu::NeedsLeafConstant::NEEDSLEAFCONSTANT && !IsConstantLeaf)) {
          // Calculate the constant data and replace all uses.
          // DCE will remove the CPUID IR operation.
          const auto ConstantCPUIDResult = CPUID->RunFunction(ConstantFunction, ConstantLeaf);
          uint64_t ResultsLower = (static_cast<uint64_t>(ConstantCPUIDResult.ebx) << 32) | ConstantCPUIDResult.eax;
          uint64_t ResultsUpper = (static_cast<uint64_t>(ConstantCPUIDResult.edx) << 32) | ConstantCPUIDResult.ecx;
          IREmit->SetWriteCursor(CodeNode);
          auto ElementPair = IREmit->_CreateElementPair(IR::OpSize::i128Bit, IREmit->_Constant(ResultsLower), IREmit->_Constant(ResultsUpper));
          // Replace all CPUID uses with this inline one
          IREmit->ReplaceAllUsesWith(CodeNode, ElementPair);
        }
      }
    }
    break;
  }

  case OP_XGETBV: {
    auto Op = IROp->CW<IR::IROp_XGetBV>();

    uint64_t ConstantFunction {};
    if (IREmit->IsValueConstant(Op->Function, &ConstantFunction) && CPUID->DoesXCRFunctionReportConstantData(ConstantFunction)) {
      const auto ConstantXCRResult = CPUID->RunXCRFunction(ConstantFunction);
      IREmit->SetWriteCursor(CodeNode);
      auto ElementPair =
        IREmit->_CreateElementPair(IR::OpSize::i64Bit, IREmit->_Constant(ConstantXCRResult.eax), IREmit->_Constant(ConstantXCRResult.edx));
      // Replace all xgetbv uses with this inline one
      IREmit->ReplaceAllUsesWith(CodeNode, ElementPair);
    }
    break;
  }

  case OP_LDIV:
  case OP_LREM: {
    auto Op = IROp->C<IR::IROp_LDiv>();
    auto UpperIROp = IREmit->GetOpHeader(Op->Upper);

    // Check upper Op to see if it came from a sign-extension
    if (UpperIROp->Op != OP_SBFE) {
      break;
    }

    auto Sbfe = UpperIROp->C<IR::IROp_Sbfe>();
    if (Sbfe->Width != 1 || Sbfe->lsb != 63 || Sbfe->Header.Args[0] != Op->Lower) {
      break;
    }

    // If it does then it we only need a 64bit SDIV
    IREmit->SetWriteCursor(CodeNode);
    Ref Lower = CurrentIR.GetNode(Op->Lower);
    Ref Divisor = CurrentIR.GetNode(Op->Divisor);
    Ref SDivOp {};
    if (IROp->Op == OP_LDIV) {
      SDivOp = IREmit->_Div(OpSize::i64Bit, Lower, Divisor);
    } else {
      SDivOp = IREmit->_Rem(OpSize::i64Bit, Lower, Divisor);
    }
    IREmit->ReplaceAllUsesWith(CodeNode, SDivOp);
    break;
  }

  case OP_LUDIV:
  case OP_LUREM: {
    auto Op = IROp->C<IR::IROp_LUDiv>();
    // Check upper Op to see if it came from a zeroing op
    // If it does then it we only need a 64bit UDIV
    uint64_t Value;
    if (!IREmit->IsValueConstant(Op->Upper, &Value) || Value != 0) {
      break;
    }

    IREmit->SetWriteCursor(CodeNode);
    Ref Lower = CurrentIR.GetNode(Op->Lower);
    Ref Divisor = CurrentIR.GetNode(Op->Divisor);
    Ref UDivOp {};
    if (IROp->Op == OP_LUDIV) {
      UDivOp = IREmit->_UDiv(OpSize::i64Bit, Lower, Divisor);
    } else {
      UDivOp = IREmit->_URem(OpSize::i64Bit, Lower, Divisor);
    }
    IREmit->ReplaceAllUsesWith(CodeNode, UDivOp);
    break;
  }

  default: break;
  }
}

void ConstProp::ConstantInlining(IREmitter* IREmit, const IRListView& CurrentIR) {
  InlineConstantGen.clear();

  for (auto [CodeNode, IROp] : CurrentIR.GetAllCode()) {
    switch (IROp->Op) {
    case OP_LSHR:
    case OP_ASHR:
    case OP_ROR:
    case OP_LSHL: {
      uint64_t Constant2 {};
      if (IREmit->IsValueConstant(IROp->Args[1], &Constant2)) {
        IREmit->SetWriteCursor(CurrentIR.GetNode(IROp->Args[1]));

        // this shouldn't be here, but rather on the emitter themselves or the constprop transformation?
        if (IROp->Size <= 4) {
          Constant2 &= 31;
        } else {
          Constant2 &= 63;
        }

        IREmit->ReplaceNodeArgument(CodeNode, 1, CreateInlineConstant(IREmit, Constant2));
      }
      break;
    }
    case OP_ADD:
    case OP_SUB:
    case OP_ADDNZCV:
    case OP_SUBNZCV:
    case OP_ADDWITHFLAGS:
    case OP_SUBWITHFLAGS: {
      uint64_t Constant2 {};
      if (IREmit->IsValueConstant(IROp->Args[1], &Constant2)) {
        // We don't allow 8/16-bit operations to have constants, since no
        // constant would be in bounds after the JIT's 24/16 shift.
        if (IsImmAddSub(Constant2) && IROp->Size >= 4) {
          IREmit->SetWriteCursor(CurrentIR.GetNode(IROp->Args[1]));
          IREmit->ReplaceNodeArgument(CodeNode, 1, CreateInlineConstant(IREmit, Constant2));
        }
      } else if (IROp->Op == OP_SUBNZCV || IROp->Op == OP_SUBWITHFLAGS || IROp->Op == OP_SUB) {
        // TODO: Generalize this
        uint64_t Constant1 {};
        if (IREmit->IsValueConstant(IROp->Args[0], &Constant1)) {
          if (Constant1 == 0) {
            IREmit->SetWriteCursor(CurrentIR.GetNode(IROp->Args[0]));
            IREmit->ReplaceNodeArgument(CodeNode, 0, CreateInlineConstant(IREmit, 0));
          }
        }
      }

      break;
    }
    case OP_ADC:
    case OP_ADCWITHFLAGS:
    case OP_STORECONTEXT: {
      uint64_t Constant1 {};
      if (IREmit->IsValueConstant(IROp->Args[0], &Constant1)) {
        if (Constant1 == 0) {
          IREmit->SetWriteCursor(CurrentIR.GetNode(IROp->Args[0]));
          IREmit->ReplaceNodeArgument(CodeNode, 0, CreateInlineConstant(IREmit, 0));
        }
      }

      break;
    }
    case OP_RMIFNZCV: {
      uint64_t Constant1 {};
      if (IREmit->IsValueConstant(IROp->Args[0], &Constant1)) {
        if (Constant1 == 0) {
          IREmit->SetWriteCursor(CurrentIR.GetNode(IROp->Args[0]));
          IREmit->ReplaceNodeArgument(CodeNode, 0, CreateInlineConstant(IREmit, 0));
        }
      }

      break;
    }
    case OP_CONDADDNZCV:
    case OP_CONDSUBNZCV: {
      uint64_t Constant2 {};
      if (IREmit->IsValueConstant(IROp->Args[1], &Constant2)) {
        if (IsImmAddSub(Constant2)) {
          IREmit->SetWriteCursor(CurrentIR.GetNode(IROp->Args[1]));
          IREmit->ReplaceNodeArgument(CodeNode, 1, CreateInlineConstant(IREmit, Constant2));
        }
      }

      uint64_t Constant1 {};
      if (IREmit->IsValueConstant(IROp->Args[0], &Constant1)) {
        if (Constant1 == 0) {
          IREmit->SetWriteCursor(CurrentIR.GetNode(IROp->Args[0]));
          IREmit->ReplaceNodeArgument(CodeNode, 0, CreateInlineConstant(IREmit, 0));
        }
      }
      break;
    }
    case OP_TESTNZ: {
      uint64_t Constant1 {};
      if (IREmit->IsValueConstant(IROp->Args[1], &Constant1)) {
        if (IsImmLogical(Constant1, IROp->Size * 8)) {
          IREmit->SetWriteCursor(CurrentIR.GetNode(IROp->Args[1]));
          IREmit->ReplaceNodeArgument(CodeNode, 1, CreateInlineConstant(IREmit, Constant1));
        }
      }
      break;
    }
    case OP_SELECT: {
      uint64_t Constant1 {};
      if (IREmit->IsValueConstant(IROp->Args[1], &Constant1)) {
        if (IsImmAddSub(Constant1)) {
          IREmit->SetWriteCursor(CurrentIR.GetNode(IROp->Args[1]));
          IREmit->ReplaceNodeArgument(CodeNode, 1, CreateInlineConstant(IREmit, Constant1));
        }
      }

      uint64_t AllOnes = IROp->Size == 8 ? 0xffff'ffff'ffff'ffffull : 0xffff'ffffull;

      uint64_t Constant2 {};
      uint64_t Constant3 {};
      if (IREmit->IsValueConstant(IROp->Args[2], &Constant2) && IREmit->IsValueConstant(IROp->Args[3], &Constant3) &&
          (Constant2 == 1 || Constant2 == AllOnes) && Constant3 == 0) {
        IREmit->SetWriteCursor(CurrentIR.GetNode(IROp->Args[2]));

        IREmit->ReplaceNodeArgument(CodeNode, 2, CreateInlineConstant(IREmit, Constant2));
        IREmit->ReplaceNodeArgument(CodeNode, 3, CreateInlineConstant(IREmit, Constant3));
      }

      break;
    }
    case OP_NZCVSELECT: {
      uint64_t AllOnes = IROp->Size == 8 ? 0xffff'ffff'ffff'ffffull : 0xffff'ffffull;

      // We always allow source 1 to be zero, but source 0 can only be a
      // special 1/~0 constant if source 1 is 0.
      uint64_t Constant0 {};
      uint64_t Constant1 {};
      if (IREmit->IsValueConstant(IROp->Args[1], &Constant1) && Constant1 == 0) {
        IREmit->SetWriteCursor(CurrentIR.GetNode(IROp->Args[1]));
        IREmit->ReplaceNodeArgument(CodeNode, 1, CreateInlineConstant(IREmit, Constant1));

        if (IREmit->IsValueConstant(IROp->Args[0], &Constant0) && (Constant0 == 1 || Constant0 == AllOnes)) {
          IREmit->SetWriteCursor(CurrentIR.GetNode(IROp->Args[0]));
          IREmit->ReplaceNodeArgument(CodeNode, 0, CreateInlineConstant(IREmit, Constant0));
        }
      }

      break;
    }
    case OP_CONDJUMP: {
      uint64_t Constant2 {};
      if (IREmit->IsValueConstant(IROp->Args[1], &Constant2)) {
        if (IsImmAddSub(Constant2)) {
          IREmit->SetWriteCursor(CurrentIR.GetNode(IROp->Args[1]));
          IREmit->ReplaceNodeArgument(CodeNode, 1, CreateInlineConstant(IREmit, Constant2));
        }
      }
      break;
    }
    case OP_EXITFUNCTION: {
      auto Op = IROp->C<IR::IROp_ExitFunction>();

      uint64_t Constant {};
      if (IREmit->IsValueConstant(Op->NewRIP, &Constant)) {
        IREmit->SetWriteCursor(CurrentIR.GetNode(Op->NewRIP));
        IREmit->ReplaceNodeArgument(CodeNode, 0, CreateInlineConstant(IREmit, Constant));
      } else {
        auto NewRIP = IREmit->GetOpHeader(Op->NewRIP);
        if (NewRIP->Op == OP_ENTRYPOINTOFFSET) {
          auto EO = NewRIP->C<IR::IROp_EntrypointOffset>();
          IREmit->SetWriteCursor(CurrentIR.GetNode(Op->NewRIP));

          IREmit->ReplaceNodeArgument(CodeNode, 0, IREmit->_InlineEntrypointOffset(IR::SizeToOpSize(EO->Header.Size), EO->Offset));
        }
      }
      break;
    }
    case OP_OR:
    case OP_XOR:
    case OP_AND:
    case OP_ANDWITHFLAGS:
    case OP_ANDN: {
      uint64_t Constant2 {};
      if (IREmit->IsValueConstant(IROp->Args[1], &Constant2)) {
        if (IsImmLogical(Constant2, IROp->Size * 8)) {
          IREmit->SetWriteCursor(CurrentIR.GetNode(IROp->Args[1]));
          IREmit->ReplaceNodeArgument(CodeNode, 1, CreateInlineConstant(IREmit, Constant2));
        }
      }
      break;
    }
    case OP_LOADMEM: {
      auto Op = IROp->CW<IR::IROp_LoadMem>();
      InlineMemImmediate(IREmit, CurrentIR, CodeNode, IROp, Op->Offset, Op->OffsetType, Op->Offset_Index, Op->OffsetScale, false);
      break;
    }
    case OP_STOREMEM: {
      auto Op = IROp->CW<IR::IROp_StoreMem>();
      InlineMemImmediate(IREmit, CurrentIR, CodeNode, IROp, Op->Offset, Op->OffsetType, Op->Offset_Index, Op->OffsetScale, false);
      break;
    }
    case OP_PREFETCH: {
      auto Op = IROp->CW<IR::IROp_Prefetch>();
      InlineMemImmediate(IREmit, CurrentIR, CodeNode, IROp, Op->Offset, Op->OffsetType, Op->Offset_Index, Op->OffsetScale, false);
      break;
    }
    case OP_LOADMEMTSO: {
      auto Op = IROp->CW<IR::IROp_LoadMemTSO>();
      InlineMemImmediate(IREmit, CurrentIR, CodeNode, IROp, Op->Offset, Op->OffsetType, Op->Offset_Index, Op->OffsetScale, true);
      break;
    }
    case OP_STOREMEMTSO: {
      auto Op = IROp->CW<IR::IROp_StoreMemTSO>();
      InlineMemImmediate(IREmit, CurrentIR, CodeNode, IROp, Op->Offset, Op->OffsetType, Op->Offset_Index, Op->OffsetScale, true);
      break;
    }
    case OP_MEMCPY: {
      auto Op = IROp->CW<IR::IROp_MemCpy>();

      uint64_t Constant {};
      if (IREmit->IsValueConstant(Op->Direction, &Constant)) {
        IREmit->SetWriteCursor(CurrentIR.GetNode(Op->Direction));
        IREmit->ReplaceNodeArgument(CodeNode, Op->Direction_Index, CreateInlineConstant(IREmit, Constant));
      }
      break;
    }
    case OP_MEMSET: {
      auto Op = IROp->CW<IR::IROp_MemSet>();

      uint64_t Constant {};
      if (IREmit->IsValueConstant(Op->Direction, &Constant)) {
        IREmit->SetWriteCursor(CurrentIR.GetNode(Op->Direction));
        IREmit->ReplaceNodeArgument(CodeNode, Op->Direction_Index, CreateInlineConstant(IREmit, Constant));
      }
      break;
    }

    default: break;
    }
  }
}

void ConstProp::Run(IREmitter* IREmit) {
  FEXCORE_PROFILE_SCOPED("PassManager::ConstProp");

  auto CurrentIR = IREmit->ViewIR();

  HandleConstantPools(IREmit, CurrentIR);

  for (auto [CodeNode, IROp] : CurrentIR.GetAllCode()) {
    ConstantPropagation(IREmit, CurrentIR, CodeNode, IROp);
  }

  ConstantInlining(IREmit, CurrentIR);
}

fextl::unique_ptr<FEXCore::IR::Pass> CreateConstProp(bool SupportsTSOImm9, const FEXCore::CPUIDEmu* CPUID) {
  return fextl::make_unique<ConstProp>(SupportsTSOImm9, CPUID);
}
} // namespace FEXCore::IR
