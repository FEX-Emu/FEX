/*
$info$
tags: ir|opts
desc: ConstProp, ZExt elim, addressgen coalesce, const pooling, fcmp reduction, const inlining
$end_info$
*/


#if JIT_ARM64
//aarch64 heuristics
#include "aarch64/assembler-aarch64.h"
#include "aarch64/cpu-aarch64.h"
#include "aarch64/disasm-aarch64.h"
#include "aarch64/assembler-aarch64.h"
#endif

#include "Interface/IR/PassManager.h"

#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IREmitter.h>
#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/Profiler.h>
#include <FEXCore/fextl/map.h>
#include <FEXCore/fextl/robin_map.h>
#include <FEXCore/fextl/unordered_map.h>

#include <bit>
#include <cstdint>
#include <memory>
#include <string.h>
#include <tuple>
#include <utility>

namespace FEXCore::IR {

template<typename T>
uint64_t getMask(T Op) {
  uint64_t NumBits = Op->Header.Size * 8;
  return (~0ULL) >> (64 - NumBits);
}


template<>
uint64_t getMask(IROp_Header* Op) {
  uint64_t NumBits = Op->Size * 8;
  return (~0ULL) >> (64 - NumBits);
}

#if JIT_ARM64
//aarch64 heuristics
static bool IsImmLogical(uint64_t imm, unsigned width) { if (width < 32) width = 32; return vixl::aarch64::Assembler::IsImmLogical(imm, width); }
static bool IsImmAddSub(uint64_t imm) { return vixl::aarch64::Assembler::IsImmAddSub(imm); }
static bool IsMemoryScale(uint64_t Scale, uint8_t AccessSize) {
  return Scale  == AccessSize;
}
#elif JIT_X86_64
// very lazy heuristics
static bool IsImmLogical(uint64_t imm, unsigned width) { return imm < 0x8000'0000; }
static bool IsImmAddSub(uint64_t imm) { return imm < 0x8000'0000; }
static bool IsMemoryScale(uint64_t Scale, uint8_t AccessSize) {
  return Scale  == 1 || Scale  == 2 || Scale  == 4 || Scale  == 8;
}
#else
#error No inline constant heuristics for this target
#endif

static bool IsImmMemory(uint64_t imm, uint8_t AccessSize) {
  if ( ((int64_t)imm >= -255) && ((int64_t)imm <= 256) )
    return true;
  else if ( (imm & (AccessSize-1)) == 0 &&  imm/AccessSize <= 4095 )
    return true;
  else {
    return false;
  }
}

static bool IsTSOImm9(uint64_t imm) {
  // RCPC2 only has a 9-bit signed offset
  if ( ((int64_t)imm >= -256) && ((int64_t)imm <= 255) )
    return true;
  else {
    return false;
  }
}

static std::tuple<MemOffsetType, uint8_t, OrderedNode*, OrderedNode*> MemExtendedAddressing(IREmitter *IREmit, uint8_t AccessSize,  IROp_Header* AddressHeader) {
  auto Src0Header = IREmit->GetOpHeader(AddressHeader->Args[0]);
  if (Src0Header->Size == 8) {
    //Try to optimize: Base + MUL(Offset, Scale)
    if (Src0Header->Op == OP_MUL) {
      uint64_t Scale;
      if (IREmit->IsValueConstant(Src0Header->Args[1], &Scale)) {
        if (IsMemoryScale(Scale, AccessSize)) {
          // remove mul as it can be folded to the mem op
          return { MEM_OFFSET_SXTX, (uint8_t)Scale, IREmit->UnwrapNode(AddressHeader->Args[1]), IREmit->UnwrapNode(Src0Header->Args[0]) };
        } else if (Scale == 1) {
          // remove nop mul
          return { MEM_OFFSET_SXTX, 1, IREmit->UnwrapNode(AddressHeader->Args[1]), IREmit->UnwrapNode(Src0Header->Args[0]) };
        }
      }
    }
    //Try to optimize: Base + LSHL(Offset, Scale)
    else if (Src0Header->Op == OP_LSHL) {
      uint64_t Constant2;
      if (IREmit->IsValueConstant(Src0Header->Args[1], &Constant2)) {
        uint64_t Scale = 1<<Constant2;
        if (IsMemoryScale(Scale, AccessSize)) {
          // remove shift as it can be folded to the mem op
          return { MEM_OFFSET_SXTX, Scale, IREmit->UnwrapNode(AddressHeader->Args[1]), IREmit->UnwrapNode(Src0Header->Args[0]) };
        } else if (Scale == 1) {
          // remove nop shift
          return { MEM_OFFSET_SXTX, 1, IREmit->UnwrapNode(AddressHeader->Args[1]), IREmit->UnwrapNode(Src0Header->Args[0]) };
        }
      }
    }
#if defined(_M_ARM_64) // x86 can't sext or zext on mem ops
    //Try to optimize: Base + (u32)Offset
    else if (Src0Header->Op == OP_BFE) {
      auto Bfe = Src0Header->C<IROp_Bfe>();
      if (Bfe->lsb == 0 && Bfe->Width == 32) {
        //todo: arm can also scale here
        return { MEM_OFFSET_UXTW, 1, IREmit->UnwrapNode(AddressHeader->Args[1]), IREmit->UnwrapNode(Src0Header->Args[0]) };
      }
    }
    //Try to optimize: Base + (s32)Offset
    else if (Src0Header->Op == OP_SBFE) {
      auto Sbfe = Src0Header->C<IROp_Sbfe>();
      if (Sbfe->lsb == 0 && Sbfe->Width == 32) {
        //todo: arm can also scale here
        return { MEM_OFFSET_SXTW, 1, IREmit->UnwrapNode(AddressHeader->Args[1]), IREmit->UnwrapNode(Src0Header->Args[0]) };
      }
    }
#endif
  }

  // no match anywhere, just add
  return { MEM_OFFSET_SXTX, 1, IREmit->UnwrapNode(AddressHeader->Args[0]), IREmit->UnwrapNode(AddressHeader->Args[1]) };
}

static OrderedNodeWrapper RemoveUselessMasking(IREmitter *IREmit, OrderedNodeWrapper src, uint64_t mask) {
#if 1 // HOTFIX: We need to clear up the meaning of opsize and dest size. See #594
  return src;
#else
  auto IROp = IREmit->GetOpHeader(src);
  if (IROp->Op == OP_AND) {
    auto Op = IROp->C<IR::IROp_And>();
    uint64_t imm;
    if (IREmit->IsValueConstant(IROp->Args[1], &imm) && ((imm & mask) == mask)) {
      return RemoveUselessMasking(IREmit, IROp->Args[0], mask);
    }
  } else if (IROp->Op == OP_BFE) {
    auto Op = IROp->C<IR::IROp_Bfe>();
    if (Op->lsb == 0) {
      uint64_t imm = 1ULL << (Op->Width-1);
      imm = (imm-1) *2 + 1;

      if ((imm & mask) == mask) {
        return RemoveUselessMasking(IREmit, IROp->Args[0], mask);
      }
    }
  }

  return src;
#endif
}

static bool IsBfeAlreadyDone(IREmitter *IREmit, OrderedNodeWrapper src, uint64_t Width) {
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
  explicit ConstProp(bool DoInlineConstants, bool SupportsTSOImm9)
    : InlineConstants(DoInlineConstants)
    , SupportsTSOImm9 {SupportsTSOImm9} { }

  bool Run(IREmitter *IREmit) override;

  bool InlineConstants;

private:
  bool HandleConstantPools(IREmitter *IREmit, const IRListView& CurrentIR);
  void CodeMotionAroundSelects(IREmitter *IREmit, const IRListView& CurrentIR);
  void FCMPOptimization(IREmitter *IREmit, const IRListView& CurrentIR);
  void LoadMemStoreMemImmediatePooling(IREmitter *IREmit, const IRListView& CurrentIR);
  bool ZextAndMaskingElimination(IREmitter *IREmit, const IRListView& CurrentIR,
      OrderedNode* CodeNode, IROp_Header* IROp);
  bool ConstantPropagation(IREmitter *IREmit, const IRListView& CurrentIR,
      OrderedNode* CodeNode, IROp_Header* IROp);
  bool ConstantInlining(IREmitter *IREmit, const IRListView& CurrentIR);

  fextl::unordered_map<uint64_t, OrderedNode*> ConstPool;
  fextl::map<OrderedNode*, uint64_t> AddressgenConsts;

  // Pool inline constant generation. These are typically very small and pool efficiently.
  fextl::robin_map<uint64_t, OrderedNode*> InlineConstantGen;
  OrderedNode *CreateInlineConstant(IREmitter *IREmit, uint64_t Constant) {
    const auto it = InlineConstantGen.find(Constant);
    if (it != InlineConstantGen.end()) {
      return it->second;
    }
    auto Result = InlineConstantGen.insert_or_assign(Constant, IREmit->_InlineConstant(Constant));
    return Result.first->second;
  }
  bool SupportsTSOImm9{};
};

bool ConstProp::HandleConstantPools(IREmitter *IREmit, const IRListView& CurrentIR) {
  bool Changed = false;

  // constants are pooled per block
  for (auto [BlockNode, BlockHeader] : CurrentIR.GetBlocks()) {
    for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {
      if (IROp->Op == OP_CONSTANT) {
        auto Op = IROp->C<IR::IROp_Constant>();
        if (ConstPool.count(Op->Constant)) {
          IREmit->ReplaceAllUsesWith(CodeNode, ConstPool[Op->Constant]);
          Changed = true;
        } else {
          ConstPool[Op->Constant] = CodeNode;
        }
      }
    }
    ConstPool.clear();
  }

  return Changed;
}

// Code motion around selects
// Moves unary ops that depend on a select before the select, if both inputs are constants
// assumes that unary ops without side effects on constants will be constprop'd
void ConstProp::CodeMotionAroundSelects(IREmitter *IREmit, const IRListView& CurrentIR) {
  // Code motion around selects
  // Moves unary ops that depend on a select before the select, if both inputs are constants
  // assumes that unary ops without side effects on constants will be constprop'd
  for (auto [BlockNode, BlockIROp] : CurrentIR.GetBlocks()) {
    auto BlockOp = BlockIROp->CW<FEXCore::IR::IROp_CodeBlock>();
    for (auto [UnaryOpNode, UnaryOpHdr] : CurrentIR.GetCode(BlockNode)) {
      if (IR::GetArgs(UnaryOpHdr->Op) == 1 && !HasSideEffects(UnaryOpHdr->Op)) {
        // could be moved
        auto SelectOpNode = IREmit->UnwrapNode(UnaryOpHdr->Args[0]);
        auto SelectOpHdr = IREmit->GetOpHeader(UnaryOpHdr->Args[0]);
        auto SelectOp = SelectOpHdr->CW<IR::IROp_Select>();

        // the value isn't used after the select otherwise
        // make sure the sizes match
        if (SelectOpHdr->Size == UnaryOpHdr->Size && SelectOpHdr->Op == OP_SELECT && SelectOpNode->NumUses == 1
            && IREmit->IsValueConstant(SelectOp->TrueVal)
            && IREmit->IsValueConstant(SelectOp->FalseVal)) {

          IREmit->SetWriteCursor(IREmit->UnwrapNode(SelectOpNode->Header.Previous));

          size_t OpSize = FEXCore::IR::GetSize(UnaryOpHdr->Op);

          /// copy for TrueVal ///
          auto NewUnaryOp1 = IREmit->AllocateRawOp(OpSize);

          // Copy over the op
          memcpy(NewUnaryOp1.first, UnaryOpHdr, OpSize);

          for (int i = 0; i < IR::GetArgs(NewUnaryOp1.first->Op); i++) {
            NewUnaryOp1.first->Args[i] = IREmit->WrapNode(IREmit->Invalid());
          }
          // Set New Op to operate on the constant
          IREmit->ReplaceNodeArgument(NewUnaryOp1, 0, IREmit->UnwrapNode(SelectOp->TrueVal));
          // Make select use the operated constant
          IREmit->ReplaceNodeArgument(SelectOpNode, 2, NewUnaryOp1);

          /// copy for FalseVal ///
          auto NewUnaryOp2 = IREmit->AllocateRawOp(OpSize);

          // Copy over the op
          memcpy(NewUnaryOp2.first, UnaryOpHdr, OpSize);

          for (int i = 0; i < IR::GetArgs(NewUnaryOp2.first->Op); i++) {
            NewUnaryOp2.first->Args[i] = IREmit->WrapNode(IREmit->Invalid());
          }
          // Set New Op to operate on the constant
          IREmit->ReplaceNodeArgument(NewUnaryOp2, 0, IREmit->UnwrapNode(SelectOp->FalseVal));
          // Make select use the operated constant
          IREmit->ReplaceNodeArgument(SelectOpNode, 3, NewUnaryOp2);

          // Replace uses of the defuct unary op w/ select
          IREmit->ReplaceAllUsesWithRange(UnaryOpNode, SelectOpNode, IREmit->GetIterator(IREmit->WrapNode(UnaryOpNode)), IREmit->GetIterator(BlockOp->Last));
        }
      }
    }
  }
}

void ConstProp::FCMPOptimization(IREmitter *IREmit, const IRListView& CurrentIR) {
  // Make all FCMPs set no flags
  for (auto [CodeNode, IROp] : CurrentIR.GetAllCode()) {
    if (IROp->Op == OP_FCMP) {
      auto fcmp = IROp->CW<IR::IROp_FCmp>();
      fcmp->Flags = 0;
    }
  }

  // Set needed flags
  for (auto [CodeNode, IROp] : CurrentIR.GetAllCode()) {
    if (IROp->Op == OP_GETHOSTFLAG) {
      auto ghf = IROp->CW<IR::IROp_GetHostFlag>();

      auto fcmp = IREmit->GetOpHeader(ghf->Value)->CW<IR::IROp_FCmp>();
      LOGMAN_THROW_AA_FMT(fcmp->Header.Op == OP_FCMP || fcmp->Header.Op == OP_F80CMP, "Unexpected OP_GETHOSTFLAG source");
      if(fcmp->Header.Op == OP_FCMP) {
        fcmp->Flags |= 1 << ghf->Flag;
      }
    }
  }
}

// LoadMem / StoreMem imm pooling
// If imms are close by, use address gen to generate the values instead of using a new imm
void ConstProp::LoadMemStoreMemImmediatePooling(IREmitter *IREmit, const IRListView& CurrentIR) {
  for (auto [BlockNode, BlockIROp] : CurrentIR.GetBlocks()) {
    for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {
      if (IROp->Op == OP_LOADMEM || IROp->Op == OP_STOREMEM) {
        size_t AddrIndex = 0;
        size_t OffsetIndex = 0;

        if (IROp->Op == OP_LOADMEM) {
          AddrIndex = IR::IROp_LoadMem::Addr_Index;
          OffsetIndex = IR::IROp_LoadMem::Offset_Index;
        }
        else {
          AddrIndex = IR::IROp_StoreMem::Addr_Index;
          OffsetIndex = IR::IROp_StoreMem::Offset_Index;
        }
        uint64_t Addr;

        if (IREmit->IsValueConstant(IROp->Args[AddrIndex], &Addr) && IROp->Args[OffsetIndex].IsInvalid()) {
          for (auto& Const: AddressgenConsts) {
            if ((Addr - Const.second) < 65536) {
              IREmit->ReplaceNodeArgument(CodeNode, AddrIndex, Const.first);
              IREmit->ReplaceNodeArgument(CodeNode, OffsetIndex, IREmit->_Constant(Addr - Const.second));
              goto doneOp;
            }
          }

          AddressgenConsts[IREmit->UnwrapNode(IROp->Args[AddrIndex])] = Addr;
        }
        doneOp:
        ;
      }
      IREmit->SetWriteCursor(CodeNode);
    }
    AddressgenConsts.clear();
  }
}

bool ConstProp::ZextAndMaskingElimination(IREmitter *IREmit, const IRListView& CurrentIR,
                                          OrderedNode* CodeNode, IROp_Header* IROp) {
  bool Changed = false;

  switch (IROp->Op) {
    // Generic handling
    case OP_OR:
    case OP_XOR:
    case OP_NOT:
    case OP_ADD:
    case OP_SUB:
    case OP_MUL:
    case OP_UMUL:
    case OP_DIV:
    case OP_UDIV:
    case OP_LSHR:
    case OP_ASHR:
    case OP_LSHL:
    case OP_ROR: {
      for (int i = 0; i < IR::GetArgs(IROp->Op); i++) {
        auto newArg = RemoveUselessMasking(IREmit, IROp->Args[i], getMask(IROp));
        if (newArg.ID() != IROp->Args[i].ID()) {
          IREmit->ReplaceNodeArgument(CodeNode, i, IREmit->UnwrapNode(newArg));
          Changed = true;
        }
      }
      break;
    }

    case OP_AND: {
      // if AND's arguments are imms, they are masking
      for (int i = 0; i < IR::GetArgs(IROp->Op); i++) {
        auto mask = getMask(IROp);
        uint64_t imm = 0;
        if (IREmit->IsValueConstant(IROp->Args[i^1], &imm))
          mask = imm;

        auto newArg = RemoveUselessMasking(IREmit, IROp->Args[i], imm);

        if (newArg.ID() != IROp->Args[i].ID()) {
          IREmit->ReplaceNodeArgument(CodeNode, i, IREmit->UnwrapNode(newArg));
          Changed = true;
        }
      }
      break;
    }

    case OP_BFE: {
      auto Op = IROp->C<IR::IROp_Bfe>();

      // Is this value already BFE'd?
      if (IsBfeAlreadyDone(IREmit, Op->Src, Op->Width)) {
        IREmit->ReplaceAllUsesWith(CodeNode, CurrentIR.GetNode(Op->Src));
        //printf("Removed BFE once \n");
        break;
      }

      // Is this value already ZEXT'd?
      if (Op->lsb == 0) {
        //LoadMem, LoadMemTSO & LoadContext ZExt
        auto source = Op->Src;
        auto sourceHeader = IREmit->GetOpHeader(source);

        if (Op->Width >= (sourceHeader->Size*8)  &&
          (sourceHeader->Op == OP_LOADMEM || sourceHeader->Op == OP_LOADMEMTSO || sourceHeader->Op == OP_LOADCONTEXT)
        ) {
          //printf("Eliminated needless zext bfe\n");
          // Load mem / load ctx zexts, no need to vmem
          IREmit->ReplaceAllUsesWith(CodeNode, CurrentIR.GetNode(source));
          break;
        }
      }

      // BFE does implicit masking, remove any masks leading to this, if possible
      uint64_t imm = 1ULL << (Op->Width-1);
      imm = (imm-1) *2 + 1;
      imm <<= Op->lsb;

      auto newArg = RemoveUselessMasking(IREmit, Op->Src, imm);

      if (newArg.ID() != Op->Src.ID()) {
        IREmit->ReplaceNodeArgument(CodeNode, Op->Src_Index, IREmit->UnwrapNode(newArg));
        Changed = true;
      }
      break;
    }

    case OP_SBFE: {
      auto Op = IROp->C<IR::IROp_Sbfe>();

      // BFE does implicit masking
      uint64_t imm = 1ULL << (Op->Width-1);
      imm = (imm-1) *2 + 1;
      imm <<= Op->lsb;

      auto newArg = RemoveUselessMasking(IREmit, Op->Src, imm);

      if (newArg.ID() != Op->Src.ID()) {
        IREmit->ReplaceNodeArgument(CodeNode, Op->Src_Index, IREmit->UnwrapNode(newArg));
        Changed = true;
      }
      break;
    }

    case OP_VFADD:
    case OP_VFSUB:
    case OP_VFMUL:
    case OP_VFDIV:
    case OP_FCMP: {
      auto flopSize = IROp->Size;
      for (int i = 0; i < IR::GetArgs(IROp->Op); i++) {
        auto argHeader = IREmit->GetOpHeader(IROp->Args[i]);

        if (argHeader->Op == OP_VMOV) {
          auto source = argHeader->Args[0];
          auto sourceHeader = IREmit->GetOpHeader(source);
          if (sourceHeader->Size >= flopSize) {
            IREmit->ReplaceNodeArgument(CodeNode, i, IREmit->UnwrapNode(source));
            //printf("VMOV bypassed\n");
          }
        }
      }
      break;
    }

    case OP_VMOV: {
      // elim from load mem
      auto source = IROp->Args[0];
      auto sourceHeader = IREmit->GetOpHeader(source);

      if (IROp->Size >= sourceHeader->Size  &&
        (sourceHeader->Op == OP_LOADMEM || sourceHeader->Op == OP_LOADMEMTSO || sourceHeader->Op == OP_LOADCONTEXT)
        ) {
        //printf("Eliminated needless zext VMOV\n");
        // Load mem / load ctx zexts, no need to vmem
        IREmit->ReplaceAllUsesWith(CodeNode, CurrentIR.GetNode(source));
      } else if (IROp->Size == sourceHeader->Size) {
        // VMOV of same size
        // XXX: This is unsafe of an optimization since in some cases we can't see through garbage data in the upper bits of a vector
        // RCLSE generates VMOV instructions which are being used as a zero extension
        //printf("printf vmov of same size?!\n");
        //IREmit->ReplaceAllUsesWith(CodeNode, CurrentIR.GetNode(source));
      }
      break;
    }
    default:
      break;
  }

  return Changed;
}

// constprop + some more per instruction logic
bool ConstProp::ConstantPropagation(IREmitter *IREmit, const IRListView& CurrentIR,
                                    OrderedNode* CodeNode, IROp_Header* IROp) {
    bool Changed = false;

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
        LOGMAN_MSG_A_FMT("Could const prop op: {}", IR::GetName(IROp->Op));
      }
    break;
    }

    case OP_SEXT:
    case OP_NEG:
    case OP_POPCOUNT:
    case OP_FINDLSB:
    case OP_FINDMSB:
    case OP_REV:
    case OP_SBFE: {
      uint64_t Constant1;

      if (IREmit->IsValueConstant(IROp->Args[0], &Constant1)) {
        LOGMAN_MSG_A_FMT("Could const prop op: {}", IR::GetName(IROp->Op));
      }
    break;
    }
*/

    case OP_LOADMEMTSO: {
      auto Op = IROp->CW<IR::IROp_LoadMemTSO>();
      auto AddressHeader = IREmit->GetOpHeader(Op->Addr);

      if (Op->Class == FEXCore::IR::FPRClass && AddressHeader->Op == OP_ADD && AddressHeader->Size == 8) {
        // TODO: LRCPC3 supports a vector unscaled offset like LRCPC2.
        // Support once hardware is available to use this.
        auto [OffsetType, OffsetScale, Arg0, Arg1] = MemExtendedAddressing(IREmit, IROp->Size, AddressHeader);

        Op->OffsetType = OffsetType;
        Op->OffsetScale = OffsetScale;
        IREmit->ReplaceNodeArgument(CodeNode, Op->Addr_Index, Arg0); // Addr
        IREmit->ReplaceNodeArgument(CodeNode, Op->Offset_Index, Arg1); // Offset

        Changed = true;
      }
      break;
    }

    case OP_STOREMEMTSO: {
      auto Op = IROp->CW<IR::IROp_StoreMemTSO>();
      auto AddressHeader = IREmit->GetOpHeader(Op->Addr);

      if (Op->Class == FEXCore::IR::FPRClass && AddressHeader->Op == OP_ADD && AddressHeader->Size == 8) {
        // TODO: LRCPC3 supports a vector unscaled offset like LRCPC2.
        // Support once hardware is available to use this.
        auto [OffsetType, OffsetScale, Arg0, Arg1] = MemExtendedAddressing(IREmit, IROp->Size, AddressHeader);

        Op->OffsetType = OffsetType;
        Op->OffsetScale = OffsetScale;
        IREmit->ReplaceNodeArgument(CodeNode, Op->Addr_Index, Arg0); // Addr
        IREmit->ReplaceNodeArgument(CodeNode, Op->Offset_Index, Arg1); // Offset

        Changed = true;
      }
      break;
    }

    case OP_LOADMEM: {
      auto Op = IROp->CW<IR::IROp_LoadMem>();
      auto AddressHeader = IREmit->GetOpHeader(Op->Addr);

      if (AddressHeader->Op == OP_ADD && AddressHeader->Size == 8) {
        auto [OffsetType, OffsetScale, Arg0, Arg1] = MemExtendedAddressing(IREmit, IROp->Size, AddressHeader);

        Op->OffsetType = OffsetType;
        Op->OffsetScale = OffsetScale;
        IREmit->ReplaceNodeArgument(CodeNode, Op->Addr_Index, Arg0); // Addr
        IREmit->ReplaceNodeArgument(CodeNode, Op->Offset_Index, Arg1); // Offset

        Changed = true;
      }
      break;
    }

    case OP_STOREMEM: {
      auto Op = IROp->CW<IR::IROp_StoreMem>();
      auto AddressHeader = IREmit->GetOpHeader(Op->Addr);

      if (AddressHeader->Op == OP_ADD && AddressHeader->Size == 8) {
        auto [OffsetType, OffsetScale, Arg0, Arg1] = MemExtendedAddressing(IREmit, IROp->Size, AddressHeader);

        Op->OffsetType = OffsetType;
        Op->OffsetScale = OffsetScale;
        IREmit->ReplaceNodeArgument(CodeNode, Op->Addr_Index, Arg0); // Addr
        IREmit->ReplaceNodeArgument(CodeNode, Op->Offset_Index, Arg1); // Offset

        Changed = true;
      }
      break;
    }

    case OP_ADD: {
      auto Op = IROp->C<IR::IROp_Add>();
      uint64_t Constant1{};
      uint64_t Constant2{};

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
      uint64_t Constant1{};
      uint64_t Constant2{};

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
      uint64_t Constant1{};
      uint64_t Constant2{};

      if (IREmit->IsValueConstant(Op->Header.Args[0], &Constant1) &&
          IREmit->IsValueConstant(Op->Header.Args[1], &Constant2)) {
        uint64_t NewConstant = (Constant1 & Constant2) & getMask(Op) ;
        IREmit->ReplaceWithConstant(CodeNode, NewConstant);
        Changed = true;
      } else if (Constant2 == 1) {
        // happens from flag calcs
        auto val = IREmit->GetOpHeader(Op->Header.Args[0]);

        uint64_t Constant3;
        if (val->Op == OP_SELECT &&
            IREmit->IsValueConstant(val->Args[2], &Constant2) &&
            IREmit->IsValueConstant(val->Args[3], &Constant3) &&
            Constant2 == 1 &&
            Constant3 == 0)
        {
          IREmit->ReplaceAllUsesWith(CodeNode, CurrentIR.GetNode(Op->Header.Args[0]));
          Changed = true;
        }
      } else if (Op->Header.Args[0].ID() == Op->Header.Args[1].ID()) {
        // AND with same value results in original value
        IREmit->ReplaceAllUsesWith(CodeNode, CurrentIR.GetNode(Op->Header.Args[0]));
        Changed = true;
      }
    break;
    }
    case OP_OR: {
      auto Op = IROp->CW<IR::IROp_Or>();
      uint64_t Constant1{};
      uint64_t Constant2{};

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
    case OP_ORLSHL: {
      auto Op = IROp->CW<IR::IROp_Orlshl>();
      uint64_t Constant1{};
      uint64_t Constant2{};

      if (IREmit->IsValueConstant(Op->Header.Args[0], &Constant1) &&
          IREmit->IsValueConstant(Op->Header.Args[1], &Constant2)) {
        uint64_t NewConstant = Constant1 | (Constant2 << Op->BitShift);
        IREmit->ReplaceWithConstant(CodeNode, NewConstant);
        Changed = true;
      }
    break;
    }
    case OP_XOR: {
      auto Op = IROp->C<IR::IROp_Xor>();
      uint64_t Constant1{};
      uint64_t Constant2{};

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
      uint64_t Constant1{};
      uint64_t Constant2{};

      if (IREmit->IsValueConstant(Op->Header.Args[0], &Constant1) &&
          IREmit->IsValueConstant(Op->Header.Args[1], &Constant2)) {
        // Shifts mask the shift amount by 63 or 31 depending on operating size;
        uint64_t ShiftMask = IROp->Size == 8 ? 63 : 31;
        uint64_t NewConstant = (Constant1 << (Constant2 & ShiftMask)) & getMask(Op);
        IREmit->ReplaceWithConstant(CodeNode, NewConstant);
        Changed = true;
      }
      else if (IREmit->IsValueConstant(Op->Header.Args[1], &Constant2) &&
                Constant2 == 0) {
        IREmit->SetWriteCursor(CodeNode);
        OrderedNode *Arg = CurrentIR.GetNode(Op->Header.Args[0]);
        IREmit->ReplaceAllUsesWith(CodeNode, Arg);
        Changed = true;
      } else {
        auto newArg = RemoveUselessMasking(IREmit, Op->Header.Args[1], IROp->Size * 8 - 1);
        if (newArg.ID() != Op->Header.Args[1].ID()) {
          IREmit->ReplaceNodeArgument(CodeNode, 1, IREmit->UnwrapNode(newArg));
          Changed = true;
        }
      }
    break;
    }
    case OP_LSHR: {
      auto Op = IROp->CW<IR::IROp_Lshr>();
      uint64_t Constant1{};
      uint64_t Constant2{};

      if (IREmit->IsValueConstant(Op->Header.Args[0], &Constant1) &&
          IREmit->IsValueConstant(Op->Header.Args[1], &Constant2)) {
        // Shifts mask the shift amount by 63 or 31 depending on operating size;
        uint64_t ShiftMask = IROp->Size == 8 ? 63 : 31;
        uint64_t NewConstant = (Constant1 >> (Constant2 & ShiftMask)) & getMask(Op);
        IREmit->ReplaceWithConstant(CodeNode, NewConstant);
        Changed = true;
      }
      else if (IREmit->IsValueConstant(Op->Header.Args[1], &Constant2) &&
                Constant2 == 0) {
        IREmit->SetWriteCursor(CodeNode);
        OrderedNode *Arg = CurrentIR.GetNode(Op->Header.Args[0]);
        IREmit->ReplaceAllUsesWith(CodeNode, Arg);
        Changed = true;
      } else {
        auto newArg = RemoveUselessMasking(IREmit, Op->Header.Args[1], IROp->Size * 8 - 1);
        if (newArg.ID() != Op->Header.Args[1].ID()) {
          IREmit->ReplaceNodeArgument(CodeNode, 1, IREmit->UnwrapNode(newArg));
          Changed = true;
        }
      }
    break;
    }
    case OP_BFE: {
      auto Op = IROp->C<IR::IROp_Bfe>();
      uint64_t Constant;
      if (IROp->Size <= 8 && IREmit->IsValueConstant(Op->Src, &Constant)) {
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
      } else if (Op->Width == 1 && Op->lsb == 0) {
        // common from flag codegen
        auto val = IREmit->GetOpHeader(Op->Header.Args[0]);

        uint64_t Constant2{};
        uint64_t Constant3{};
        if (val->Op == OP_SELECT &&
            IREmit->IsValueConstant(val->Args[2], &Constant2) &&
            IREmit->IsValueConstant(val->Args[3], &Constant3) &&
            Constant2 == 1 &&
            Constant3 == 0)
        {
          IREmit->ReplaceAllUsesWith(CodeNode, CurrentIR.GetNode(Op->Header.Args[0]));
          Changed = true;
        }
      }

    break;
    }
    case OP_SBFE: {
      auto Op = IROp->C<IR::IROp_Bfe>();
      uint64_t Constant;
      if (IREmit->IsValueConstant(Op->Src, &Constant)) {
        // SBFE of a constant can be converted to a constant.
        uint64_t SourceMask = (1ULL << Op->Width) - 1;
        if (Op->Width == 64)
          SourceMask = ~0ULL;
        SourceMask <<= Op->lsb;

        int64_t NewConstant = (Constant & SourceMask) >> Op->lsb;
        NewConstant <<= 64 - Op->Width;
        NewConstant >>= 64 - Op->Width;
        IREmit->ReplaceWithConstant(CodeNode, NewConstant);

        Changed = true;
      }
      break;
    }
    case OP_BFI: {
      auto Op = IROp->C<IR::IROp_Bfi>();
      uint64_t ConstantDest{};
      uint64_t ConstantSrc{};

      if (IREmit->IsValueConstant(Op->Header.Args[0], &ConstantDest) &&
          IREmit->IsValueConstant(Op->Header.Args[1], &ConstantSrc)) {

        uint64_t SourceMask = (1ULL << Op->Width) - 1;
        if (Op->Width == 64)
          SourceMask = ~0ULL;

        uint64_t NewConstant = ConstantDest & ~(SourceMask << Op->lsb);
        NewConstant |= (ConstantSrc & SourceMask) << Op->lsb;

        IREmit->ReplaceWithConstant(CodeNode, NewConstant);
        Changed = true;
      }
      break;
    }
    case OP_MUL: {
      auto Op = IROp->C<IR::IROp_Mul>();
      uint64_t Constant1{};
      uint64_t Constant2{};

      if (IREmit->IsValueConstant(Op->Header.Args[0], &Constant1) &&
          IREmit->IsValueConstant(Op->Header.Args[1], &Constant2)) {
        uint64_t NewConstant = (Constant1 * Constant2) & getMask(Op);
        IREmit->ReplaceWithConstant(CodeNode, NewConstant);
        Changed = true;
      } else if (IREmit->IsValueConstant(Op->Header.Args[1], &Constant2) && std::popcount(Constant2) == 1) {
        if (IROp->Size == 4 || IROp->Size == 8) {
          uint64_t amt = std::countr_zero(Constant2);
          IREmit->SetWriteCursor(CodeNode);
          auto shift = IREmit->_Lshl(CurrentIR.GetNode(Op->Header.Args[0]), IREmit->_Constant(amt));
          shift.first->Header.Size = IROp->Size; // force Lshl to be the same size as the original Mul
          IREmit->ReplaceAllUsesWith(CodeNode, shift);
          Changed = true;
        }
      }
      break;
    }
    case OP_SELECT: {
      auto Op = IROp->C<IR::IROp_Select>();
      uint64_t Constant1{};
      uint64_t Constant2{};

      if (IREmit->IsValueConstant(Op->Header.Args[0], &Constant1) &&
          IREmit->IsValueConstant(Op->Header.Args[1], &Constant2) &&
          Op->Cond == COND_EQ) {

        Constant1 &= getMask(Op);
        Constant2 &= getMask(Op);

        bool is_true = Constant1 == Constant2;

        IREmit->ReplaceAllUsesWith(CodeNode, CurrentIR.GetNode(Op->Header.Args[is_true ? 2 : 3]));
        Changed = true;
      }
      break;
    }
    case OP_CONDJUMP: {
      auto Op = IROp->CW<IR::IROp_CondJump>();

      auto Select = IREmit->GetOpHeader(Op->Header.Args[0]);

      uint64_t Constant;
      // Fold the select into the CondJump if possible. Could handle more complex cases, too.
      if (Op->Cond.Val == COND_NEQ && IREmit->IsValueConstant(Op->Cmp2, &Constant) && Constant == 0 &&  Select->Op == OP_SELECT) {

        uint64_t Constant1{};
        uint64_t Constant2{};

        if (IREmit->IsValueConstant(Select->Args[2], &Constant1) && IREmit->IsValueConstant(Select->Args[3], &Constant2)) {
          if (Constant1 == 1 && Constant2 == 0) {
            auto slc = Select->C<IR::IROp_Select>();
            IREmit->ReplaceNodeArgument(CodeNode, 0, IREmit->UnwrapNode(Select->Args[0]));
            IREmit->ReplaceNodeArgument(CodeNode, 1, IREmit->UnwrapNode(Select->Args[1]));
            Op->Cond = slc->Cond;
            Op->CompareSize = slc->CompareSize;
            Changed = true;
          }
        }
      }
      break;
    }

    default:
      break;
    }

    return Changed;
}

bool ConstProp::ConstantInlining(IREmitter *IREmit, const IRListView& CurrentIR) {
  InlineConstantGen.clear();
  bool Changed = false;

  for (auto [CodeNode, IROp] : CurrentIR.GetAllCode()) {
    switch(IROp->Op) {
      case OP_LSHR:
      case OP_ASHR:
      case OP_ROR:
      case OP_LSHL:
      {
        auto Op = IROp->C<IR::IROp_Lshr>();

        uint64_t Constant2{};
        if (IREmit->IsValueConstant(Op->Header.Args[1], &Constant2)) {
          IREmit->SetWriteCursor(CurrentIR.GetNode(Op->Header.Args[1]));

          // this shouldn't be here, but rather on the emitter themselves or the constprop transformation?
          if (IROp->Size <=4)
            Constant2 &= 31;
          else
            Constant2 &= 63;

          IREmit->ReplaceNodeArgument(CodeNode, 1, CreateInlineConstant(IREmit, Constant2));

          Changed = true;
        }
        break;
      }
      case OP_ADD:
      case OP_SUB:
      {
        auto Op = IROp->C<IR::IROp_Add>();

        uint64_t Constant2{};
        if (IREmit->IsValueConstant(Op->Header.Args[1], &Constant2)) {
          if (IsImmAddSub(Constant2)) {
            IREmit->SetWriteCursor(CurrentIR.GetNode(Op->Header.Args[1]));

            IREmit->ReplaceNodeArgument(CodeNode, 1, CreateInlineConstant(IREmit, Constant2));

            Changed = true;
          }
        }
        break;
      }
      case OP_SELECT:
      {
        auto Op = IROp->C<IR::IROp_Select>();

        uint64_t Constant1{};
        if (IREmit->IsValueConstant(Op->Header.Args[1], &Constant1)) {
          if (IsImmAddSub(Constant1)) {
            IREmit->SetWriteCursor(CurrentIR.GetNode(Op->Header.Args[1]));

            IREmit->ReplaceNodeArgument(CodeNode, 1, CreateInlineConstant(IREmit, Constant1));

            Changed = true;
          }
        }

        uint64_t Constant2{};
        uint64_t Constant3{};
        if (IREmit->IsValueConstant(Op->Header.Args[2], &Constant2) &&
            IREmit->IsValueConstant(Op->Header.Args[3], &Constant3) &&
            Constant2 == 1 &&
            Constant3 == 0)
        {
          IREmit->SetWriteCursor(CurrentIR.GetNode(Op->Header.Args[2]));

          IREmit->ReplaceNodeArgument(CodeNode, 2, CreateInlineConstant(IREmit, Constant2));
          IREmit->ReplaceNodeArgument(CodeNode, 3, CreateInlineConstant(IREmit, Constant3));
        }

        break;
      }
      case OP_CONDJUMP:
      {
        auto Op = IROp->C<IR::IROp_CondJump>();

        uint64_t Constant2{};
        if (IREmit->IsValueConstant(Op->Header.Args[1], &Constant2)) {
          if (IsImmAddSub(Constant2)) {
            IREmit->SetWriteCursor(CurrentIR.GetNode(Op->Header.Args[1]));

            IREmit->ReplaceNodeArgument(CodeNode, 1, CreateInlineConstant(IREmit, Constant2));

            Changed = true;
          }
        }
        break;
      }
      case OP_EXITFUNCTION:
      {
        auto Op = IROp->C<IR::IROp_ExitFunction>();

        uint64_t Constant{};
        if (IREmit->IsValueConstant(Op->NewRIP, &Constant)) {

          IREmit->SetWriteCursor(CurrentIR.GetNode(Op->NewRIP));

          IREmit->ReplaceNodeArgument(CodeNode, 0, CreateInlineConstant(IREmit, Constant));

          Changed = true;
        } else {
          auto NewRIP = IREmit->GetOpHeader(Op->NewRIP);
          if (NewRIP->Op == OP_ENTRYPOINTOFFSET) {
            auto EO = NewRIP->C<IR::IROp_EntrypointOffset>();
            IREmit->SetWriteCursor(CurrentIR.GetNode(Op->NewRIP));

            IREmit->ReplaceNodeArgument(CodeNode, 0, IREmit->_InlineEntrypointOffset(EO->Offset, EO->Header.Size));
            Changed = true;
          }
        }
        break;
      }
      case OP_OR:
      case OP_XOR:
      case OP_AND:
      {
        auto Op = IROp->CW<IR::IROp_Or>();

        uint64_t Constant2{};
        if (IREmit->IsValueConstant(Op->Header.Args[1], &Constant2)) {
          if (IsImmLogical(Constant2, IROp->Size * 8)) {
            IREmit->SetWriteCursor(CurrentIR.GetNode(Op->Header.Args[1]));

            IREmit->ReplaceNodeArgument(CodeNode, 1, CreateInlineConstant(IREmit, Constant2));

            Changed = true;
          }
        }
        break;
      }
      case OP_LOADMEM:
      {
        auto Op = IROp->CW<IR::IROp_LoadMem>();

        uint64_t Constant2{};
        if (Op->OffsetType == MEM_OFFSET_SXTX && IREmit->IsValueConstant(Op->Offset, &Constant2)) {
          if (IsImmMemory(Constant2, IROp->Size)) {
            IREmit->SetWriteCursor(CurrentIR.GetNode(Op->Offset));

            IREmit->ReplaceNodeArgument(CodeNode, Op->Offset_Index, CreateInlineConstant(IREmit, Constant2));

            Changed = true;
          }
        }
        break;
      }
      case OP_STOREMEM:
      {
        auto Op = IROp->CW<IR::IROp_StoreMem>();

        uint64_t Constant2{};
        if (Op->OffsetType == MEM_OFFSET_SXTX && IREmit->IsValueConstant(Op->Offset, &Constant2)) {
          if (IsImmMemory(Constant2, IROp->Size)) {
            IREmit->SetWriteCursor(CurrentIR.GetNode(Op->Offset));

            IREmit->ReplaceNodeArgument(CodeNode, Op->Offset_Index, CreateInlineConstant(IREmit, Constant2));

            Changed = true;
          }
        }
        break;
      }
      case OP_LOADMEMTSO:
      {
        auto Op = IROp->CW<IR::IROp_LoadMemTSO>();

        uint64_t Constant2{};
        if (SupportsTSOImm9) {
          if (Op->OffsetType == MEM_OFFSET_SXTX && IREmit->IsValueConstant(Op->Offset, &Constant2)) {
            if (IsTSOImm9(Constant2)) {
              IREmit->SetWriteCursor(CurrentIR.GetNode(Op->Offset));

              IREmit->ReplaceNodeArgument(CodeNode, Op->Offset_Index, CreateInlineConstant(IREmit, Constant2));

              Changed = true;
            }
          }
        }
        break;
      }
      case OP_STOREMEMTSO:
      {
        auto Op = IROp->CW<IR::IROp_StoreMemTSO>();

        uint64_t Constant2{};
        if (SupportsTSOImm9) {
          if (Op->OffsetType == MEM_OFFSET_SXTX && IREmit->IsValueConstant(Op->Offset, &Constant2)) {
            if (IsTSOImm9(Constant2)) {
              IREmit->SetWriteCursor(CurrentIR.GetNode(Op->Offset));

              IREmit->ReplaceNodeArgument(CodeNode, Op->Offset_Index, CreateInlineConstant(IREmit, Constant2));

              Changed = true;
            }
          }
        }
        break;
      }
      default:
        break;
    }
  }

  return Changed;
}

bool ConstProp::Run(IREmitter *IREmit) {
  FEXCORE_PROFILE_SCOPED("PassManager::ConstProp");

  bool Changed = false;
  auto CurrentIR = IREmit->ViewIR();
  auto OriginalWriteCursor = IREmit->GetWriteCursor();

  if (HandleConstantPools(IREmit, CurrentIR)) {
    Changed = true;
  }

  CodeMotionAroundSelects(IREmit, CurrentIR);
  FCMPOptimization(IREmit, CurrentIR);
  LoadMemStoreMemImmediatePooling(IREmit, CurrentIR);

  for (auto [CodeNode, IROp] : CurrentIR.GetAllCode()) {
    if (ZextAndMaskingElimination(IREmit, CurrentIR, CodeNode, IROp)) {
      Changed = true;
    }
    if (ConstantPropagation(IREmit, CurrentIR, CodeNode, IROp)) {
      Changed = true;
    }
  }

  if (InlineConstants && ConstantInlining(IREmit, CurrentIR)) {
    Changed = true;
  }

  IREmit->SetWriteCursor(OriginalWriteCursor);
  return Changed;
}

fextl::unique_ptr<FEXCore::IR::Pass> CreateConstProp(bool InlineConstants, bool SupportsTSOImm9) {
  return fextl::make_unique<ConstProp>(InlineConstants, SupportsTSOImm9);
}

}
