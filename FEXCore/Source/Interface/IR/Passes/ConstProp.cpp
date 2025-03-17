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
#include "Interface/Core/CPUID.h"

#include <FEXCore/IR/IR.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/Profiler.h>
#include <FEXCore/fextl/map.h>
#include <FEXCore/fextl/unordered_map.h>

#include <cstdint>
#include <string.h>

namespace FEXCore::IR {

uint64_t getMask(IROp_Header* Op) {
  uint64_t NumBits = IR::OpSizeAsBits(Op->Size);
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
  return ARMEmitter::Emitter::IsImmLogical(imm, width);
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

  bool SupportsTSOImm9 {};
  const FEXCore::CPUIDEmu* CPUID;

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
    bool IsExtended = (Imm & (IR::OpSizeToSize(IROp->Size) - 1)) == 0 && Imm / IR::OpSizeToSize(IROp->Size) <= 4095;
    IsExtended &= !TSO;

    if (IsSIMM9 || IsExtended) {
      IREmit->SetWriteCursor(IR.GetNode(Offset));
      IREmit->ReplaceNodeArgument(CodeNode, Offset_Index, IREmit->_InlineConstant(Imm));
      OffsetScale = 1;
    }
  }
};

// Constants are pooled per block.
void ConstProp::HandleConstantPools(IREmitter* IREmit, const IRListView& CurrentIR) {
  const uint32_t SSACount = CurrentIR.GetSSACount();

  // Allocation/initialization deferred until first use, since many multiblocks
  // don't have constants leftover after all inlining.
  fextl::vector<Ref> Remap {};

  struct Entry {
    int64_t Value;
    Ref R;
  };


  fextl::vector<Entry> Pool {};

  for (auto [BlockNode, BlockIROp] : CurrentIR.GetBlocks()) {
    Pool.clear();

    for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {
      if (IROp->Op == OP_CONSTANT) {
        auto Op = IROp->C<IR::IROp_Constant>();
        bool Found = false;

        // Search for the constant. This is O(n^2) but n is small since it's
        // local and most constants are inlined. In practice, it ends up much
        // faster than a hash table.
        for (auto K : Pool) {
          if (K.Value == Op->Constant) {
            uint32_t Value = CurrentIR.GetID(CodeNode).Value;
            LOGMAN_THROW_A_FMT(Value < SSACount, "def not yet remapped");

            if (Remap.empty()) {
              Remap.resize(SSACount, nullptr);
            }

            Remap[Value] = K.R;
            Found = true;
            break;
          }
        }

        if (!Found) {
          Pool.push_back({.Value = Op->Constant, .R = CodeNode});
        }
      } else if (!Remap.empty()) {
        const uint8_t NumArgs = IR::GetArgs(IROp->Op);
        for (uint8_t i = 0; i < NumArgs; ++i) {
          if (IROp->Args[i].IsInvalid()) {
            continue;
          }

          uint32_t Value = IROp->Args[i].ID().Value;
          LOGMAN_THROW_A_FMT(Value < SSACount, "src not yet remapped");

          Ref New = Remap[Value];
          if (New) {
            IREmit->ReplaceNodeArgument(CodeNode, i, New);
          }
        }
      }
    }
  }
}

// Helper to replace the destination of an instruction with one of its sources,
// to implement algebraic identities. This is surprisingly tricky due to
// implicit masking in our IR.
//
// FEX's IR uses sized opcodes, matching arm64 semantics. 64-bit opcodes do not
// mask, whereas smaller opcodes mask/zero-extend from 32-bits. Therefore, if
// the instruction is 32-bit, we need to mask the source for a sound
// replacement, in case there was garbage in the upper bits.
//
// However, if that source is in turn written by a 32-bit instruction, it is
// guaranteed to have already been masked, so we know there's no garbage and we
// can avoid the zero-extension. This is the case 99% of the time, but the
// masking here is correctness-bearing nevertheless (and new versions of Denuvo
// break if you get this wrong!)
static inline void ReplaceWithSource(IREmitter* IREmit, const IRListView& CurrentIR, Ref CodeNode, IROp_Header* IROp, unsigned Idx) {
  Ref Arg = CurrentIR.GetNode(IROp->Args[Idx]);

  if (IROp->Size < OpSize::i64Bit) {
    LOGMAN_THROW_A_FMT(IROp->Size == OpSize::i32Bit, "other sizes not here");

    auto Header = IREmit->GetOpHeader(IROp->Args[Idx]);
    if (Header->Size > OpSize::i32Bit) {
      Arg = IREmit->_Bfe(OpSize::i32Bit, 32, 0, Arg);
    }
  }

  IREmit->ReplaceAllUsesWith(CodeNode, Arg);
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

    /* IsImmAddSub assumes the constants are sign-extended, take care of that
     * here so we get the optimization for 32-bit adds too.
     */
    if (Op->Header.Size == OpSize::i32Bit) {
      Constant1 = (int64_t)(int32_t)Constant1;
      Constant2 = (int64_t)(int32_t)Constant2;
    }

    if (IsConstant1 && IsConstant2 && IROp->Op == OP_ADD) {
      uint64_t NewConstant = (Constant1 + Constant2) & getMask(IROp);
      IREmit->ReplaceWithConstant(CodeNode, NewConstant);
      break;
    } else if (IsConstant1 && IsConstant2 && IROp->Op == OP_SUB) {
      uint64_t NewConstant = (Constant1 - Constant2) & getMask(IROp);
      IREmit->ReplaceWithConstant(CodeNode, NewConstant);
      break;
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

    bool Replaced = false;

    // Order matter for short circuit evaluation, subsequent ifs read constant2.
    if (IREmit->IsValueConstant(IROp->Args[1], &Constant2) && IREmit->IsValueConstant(IROp->Args[0], &Constant1)) {
      uint64_t NewConstant = (Constant1 & Constant2) & getMask(IROp);
      IREmit->ReplaceWithConstant(CodeNode, NewConstant);
      Replaced = true;
    } else if (IROp->Args[0].ID() == IROp->Args[1].ID() || (Constant2 & getMask(IROp)) == getMask(IROp)) {
      // AND with same value results in original value
      ReplaceWithSource(IREmit, CurrentIR, CodeNode, IROp, 0);
      Replaced = true;
    }

    if (!Replaced) {
      InlineIf(IREmit, CurrentIR, CodeNode, IROp, 1, [&IROp](uint64_t X) { return IsImmLogical(X, IR::OpSizeAsBits(IROp->Size)); });
    }
    break;
  }
  case OP_OR: {
    InlineIf(IREmit, CurrentIR, CodeNode, IROp, 1, [&IROp](uint64_t X) { return IsImmLogical(X, IR::OpSizeAsBits(IROp->Size)); });
    break;
  }
  case OP_XOR: {
    uint64_t Constant1 {};

    if (IROp->Args[0].ID() == IROp->Args[1].ID()) {
      // XOR with same value results to zero
      IREmit->SetWriteCursor(CodeNode);
      IREmit->ReplaceAllUsesWith(CodeNode, IREmit->_Constant(0));
    } else {
      // XOR with zero results in the nonzero source
      bool Replaced = false;
      for (unsigned i = 0; i < 2; ++i) {
        if (!IREmit->IsValueConstant(IROp->Args[i], &Constant1)) {
          continue;
        }

        if (Constant1 != 0) {
          continue;
        }

        IREmit->SetWriteCursor(CodeNode);
        ReplaceWithSource(IREmit, CurrentIR, CodeNode, IROp, 1 - i);
        Replaced = true;
        break;
      }

      if (!Replaced) {
        InlineIf(IREmit, CurrentIR, CodeNode, IROp, 1, [&IROp](uint64_t X) { return IsImmLogical(X, IR::OpSizeAsBits(IROp->Size)); });
      }
    }
    break;
  }
  case OP_ANDWITHFLAGS:
  case OP_ANDN:
  case OP_TESTNZ: {
    InlineIf(IREmit, CurrentIR, CodeNode, IROp, 1, [&IROp](uint64_t X) { return IsImmLogical(X, IR::OpSizeAsBits(IROp->Size)); });
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
  case OP_ASHR:
  case OP_ROR: {
    Inline(IREmit, CurrentIR, CodeNode, IROp, 1);
    break;
  }
  case OP_LSHL: {
    uint64_t Constant1 {};
    uint64_t Constant2 {};

    if (IREmit->IsValueConstant(IROp->Args[0], &Constant1) && IREmit->IsValueConstant(IROp->Args[1], &Constant2)) {
      // Shifts mask the shift amount by 63 or 31 depending on operating size;
      uint64_t ShiftMask = IROp->Size == OpSize::i64Bit ? 63 : 31;
      uint64_t NewConstant = (Constant1 << (Constant2 & ShiftMask)) & getMask(IROp);
      IREmit->ReplaceWithConstant(CodeNode, NewConstant);
    } else if (IREmit->IsValueConstant(IROp->Args[1], &Constant2) && Constant2 == 0) {
      IREmit->SetWriteCursor(CodeNode);
      ReplaceWithSource(IREmit, CurrentIR, CodeNode, IROp, 0);
    } else {
      Inline(IREmit, CurrentIR, CodeNode, IROp, 1);
    }
    break;
  }
  case OP_LSHR: {
    uint64_t Constant2 {};

    if (IREmit->IsValueConstant(IROp->Args[1], &Constant2) && Constant2 == 0) {
      IREmit->SetWriteCursor(CodeNode);
      ReplaceWithSource(IREmit, CurrentIR, CodeNode, IROp, 0);
    } else {
      Inline(IREmit, CurrentIR, CodeNode, IROp, 1);
    }
    break;
  }
  case OP_BFE: {
    auto Op = IROp->C<IR::IROp_Bfe>();
    uint64_t Constant;

    if (IROp->Size <= OpSize::i64Bit && IREmit->IsValueConstant(Op->Src, &Constant)) {
      uint64_t SourceMask = Op->Width == 64 ? ~0ULL : ((1ULL << Op->Width) - 1);
      SourceMask <<= Op->lsb;

      uint64_t NewConstant = (Constant & SourceMask) >> Op->lsb;
      IREmit->ReplaceWithConstant(CodeNode, NewConstant);
    }

    break;
  }
  case OP_SBFE: {
    auto Op = IROp->C<IR::IROp_Bfe>();
    uint64_t Constant;
    if (IREmit->IsValueConstant(Op->Src, &Constant)) {
      // SBFE of a constant can be converted to a constant.
      uint64_t SourceMask = Op->Width == 64 ? ~0ULL : ((1ULL << Op->Width) - 1);
      uint64_t DestSizeInBits = IR::OpSizeAsBits(IROp->Size);
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
    uint64_t ConstantSrc {};
    bool SrcIsConstant = IREmit->IsValueConstant(IROp->Args[1], &ConstantSrc);

    if (SrcIsConstant && HasConsecutiveBits(ConstantSrc, Op->Width)) {
      // We are trying to insert constant, if it is a bitfield of only set bits then we can orr or and it.
      IREmit->SetWriteCursor(CodeNode);
      uint64_t SourceMask = Op->Width == 64 ? ~0ULL : ((1ULL << Op->Width) - 1);
      uint64_t NewConstant = SourceMask << Op->lsb;

      if (ConstantSrc & 1) {
        auto orr = IREmit->_Or(IROp->Size, CurrentIR.GetNode(IROp->Args[0]), IREmit->_Constant(NewConstant));
        IREmit->ReplaceAllUsesWith(CodeNode, orr);
      } else {
        // We are wanting to clear the bitfield.
        auto andn = IREmit->_Andn(IROp->Size, CurrentIR.GetNode(IROp->Args[0]), IREmit->_Constant(NewConstant));
        IREmit->ReplaceAllUsesWith(CodeNode, andn);
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
          const auto Result = CPUID->RunFunction(ConstantFunction, ConstantLeaf);

          IREmit->SetWriteCursor(CodeNode);
          IREmit->ReplaceAllUsesWith(CurrentIR.GetNode(Op->OutEAX), IREmit->_Constant(Result.eax));
          IREmit->ReplaceAllUsesWith(CurrentIR.GetNode(Op->OutEBX), IREmit->_Constant(Result.ebx));
          IREmit->ReplaceAllUsesWith(CurrentIR.GetNode(Op->OutECX), IREmit->_Constant(Result.ecx));
          IREmit->ReplaceAllUsesWith(CurrentIR.GetNode(Op->OutEDX), IREmit->_Constant(Result.edx));
          IREmit->Remove(CodeNode);
        }
      }
    }
    break;
  }

  case OP_XGETBV: {
    auto Op = IROp->CW<IR::IROp_XGetBV>();

    uint64_t ConstantFunction {};
    if (IREmit->IsValueConstant(Op->Function, &ConstantFunction) && CPUID->DoesXCRFunctionReportConstantData(ConstantFunction)) {
      const auto Result = CPUID->RunXCRFunction(ConstantFunction);
      IREmit->SetWriteCursor(CodeNode);
      IREmit->ReplaceAllUsesWith(CurrentIR.GetNode(Op->OutEAX), IREmit->_Constant(Result.eax));
      IREmit->ReplaceAllUsesWith(CurrentIR.GetNode(Op->OutEDX), IREmit->_Constant(Result.edx));
      IREmit->Remove(CodeNode);
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
  case OP_SELECT: {
    InlineIf(IREmit, CurrentIR, CodeNode, IROp, 1, ARMEmitter::IsImmAddSub);

    uint64_t AllOnes = IROp->Size == OpSize::i64Bit ? 0xffff'ffff'ffff'ffffull : 0xffff'ffffull;

    uint64_t Constant2 {};
    uint64_t Constant3 {};
    if (IREmit->IsValueConstant(IROp->Args[2], &Constant2) && IREmit->IsValueConstant(IROp->Args[3], &Constant3) &&
        (Constant2 == 1 || Constant2 == AllOnes) && Constant3 == 0) {
      IREmit->SetWriteCursor(CurrentIR.GetNode(IROp->Args[2]));

      IREmit->ReplaceNodeArgument(CodeNode, 2, IREmit->_InlineConstant(Constant2));
      IREmit->ReplaceNodeArgument(CodeNode, 3, IREmit->_InlineConstant(Constant3));
    }

    break;
  }
  case OP_NZCVSELECT: {
    // We always allow source 1 to be zero, but source 0 can only be a
    // special 1/~0 constant if source 1 is 0.
    if (InlineIfZero(IREmit, CurrentIR, CodeNode, IROp, 1)) {
      uint64_t AllOnes = IROp->Size == OpSize::i64Bit ? 0xffff'ffff'ffff'ffffull : 0xffff'ffffull;
      InlineIf(IREmit, CurrentIR, CodeNode, IROp, 0, [&AllOnes](uint64_t X) { return X == 1 || X == AllOnes; });
    }
    break;
  }
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
    InlineMemImmediate(IREmit, CurrentIR, CodeNode, IROp, Op->Offset, Op->OffsetType, Op->Offset_Index, Op->OffsetScale, false);
    break;
  }
  case OP_STOREMEM: {
    auto Op = IROp->CW<IR::IROp_StoreMem>();
    InlineMemImmediate(IREmit, CurrentIR, CodeNode, IROp, Op->Offset, Op->OffsetType, Op->Offset_Index, Op->OffsetScale, false);
    InlineIfZero(IREmit, CurrentIR, CodeNode, IROp, Op->Value_Index);
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

  for (auto [CodeNode, IROp] : CurrentIR.GetAllCode()) {
    ConstantPropagation(IREmit, CurrentIR, CodeNode, IROp);
  }

  HandleConstantPools(IREmit, IREmit->ViewIR());
}

fextl::unique_ptr<FEXCore::IR::Pass> CreateConstProp(bool SupportsTSOImm9, const FEXCore::CPUIDEmu* CPUID) {
  return fextl::make_unique<ConstProp>(SupportsTSOImm9, CPUID);
}
} // namespace FEXCore::IR
