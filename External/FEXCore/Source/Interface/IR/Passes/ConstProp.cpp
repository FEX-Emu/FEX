#if defined(_M_ARM_64)
//aarch64 heuristics
#include "aarch64/assembler-aarch64.h"
#include "aarch64/cpu-aarch64.h"
#include "aarch64/disasm-aarch64.h"
#include "aarch64/assembler-aarch64.h"
#endif

#include "Interface/IR/PassManager.h"
#include "Interface/Core/OpcodeDispatcher.h"

namespace FEXCore::IR {

class ConstProp final : public FEXCore::IR::Pass {
  std::unordered_map<uint64_t, OrderedNode*> ConstPool;
  std::map<OrderedNode*, uint64_t> AddressgenConsts;
public:
  bool Run(IREmitter *IREmit) override;
  bool InlineConstants;
  ConstProp(bool DoInlineConstants) : InlineConstants(DoInlineConstants) { }
};

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

#ifdef _M_X86_64
// very lazy heuristics
static bool IsImmLogical(uint64_t imm, unsigned width) { return imm < 0x8000'0000; }
static bool IsImmAddSub(uint64_t imm) { return imm < 0x8000'0000; }
static bool IsMemoryScale(uint64_t Scale, uint8_t AccessSize) {
  return Scale  == 1 || Scale  == 2 || Scale  == 4 || Scale  == 8;
}
#elif defined(_M_ARM_64)
//aarch64 heuristics
static bool IsImmLogical(uint64_t imm, unsigned width) { if (width < 32) width = 32; return vixl::aarch64::Assembler::IsImmLogical(imm, width); }
static bool IsImmAddSub(uint64_t imm) { return vixl::aarch64::Assembler::IsImmAddSub(imm); }
static bool IsMemoryScale(uint64_t Scale, uint8_t AccessSize) {
  return Scale  == AccessSize;
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

std::tuple<MemOffsetType, uint8_t, OrderedNode*, OrderedNode*> MemExtendedAddressing(IREmitter *IREmit, uint8_t AccessSize,  IROp_Header* AddressHeader) {

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

OrderedNodeWrapper RemoveUselessMasking(IREmitter *IREmit, OrderedNodeWrapper src, uint64_t mask) {
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

bool IsBfeAlreadyDone(IREmitter *IREmit, OrderedNodeWrapper src, uint64_t Width) {
  auto IROp = IREmit->GetOpHeader(src);
  if (IROp->Op == OP_BFE) {
    auto Op = IROp->C<IR::IROp_Bfe>();
    if (Width >= Op->Width) {
      return true;
    }
  }
  return false;
}

bool ConstProp::Run(IREmitter *IREmit) {

  bool Changed = false;
  auto CurrentIR = IREmit->ViewIR();

  auto Header = CurrentIR.GetHeader();

  auto OriginalWriteCursor = IREmit->GetWriteCursor();

  auto HeaderOp = CurrentIR.GetHeader();

  {

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
  }

  // Code motion around selects
  // Moves unary ops that depend on a select before the select, if both inputs are constants
  // assumes that unary ops without side effects on constants will be constprop'd
  for (auto [BlockNode, BlockIROp] : CurrentIR.GetBlocks()) {
    auto BlockOp = BlockIROp->CW<FEXCore::IR::IROp_CodeBlock>();
    for (auto [UnaryOpNode, UnaryOpHdr] : CurrentIR.GetCode(BlockNode)) {
      if (UnaryOpHdr->NumArgs == 1 && !HasSideEffects(UnaryOpHdr->Op)) {
        // could be moved
        auto SelectOpNode = IREmit->UnwrapNode(UnaryOpHdr->Args[0]);
        auto SelectOpHdr = IREmit->GetOpHeader(UnaryOpHdr->Args[0]);
        auto SelectOp = SelectOpHdr->CW<IR::IROp_Select>();

        // the value isn't used after the select otherwise
        if (SelectOpHdr->Op == OP_SELECT && SelectOpNode->NumUses == 1
          && IREmit->IsValueConstant(SelectOp->TrueVal)
          && IREmit->IsValueConstant(SelectOp->FalseVal)) {

          IREmit->SetWriteCursor(IREmit->UnwrapNode(SelectOpNode->Header.Previous));

          size_t OpSize = FEXCore::IR::GetSize(UnaryOpHdr->Op);

          /// copy for TrueVal ///
          auto NewUnaryOp1 = IREmit->AllocateRawOp(OpSize);

          // Copy over the op
          memcpy(NewUnaryOp1.first, UnaryOpHdr, OpSize);

          for (int i = 0; i < NewUnaryOp1.first->NumArgs; i++) {
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

          for (int i = 0; i < NewUnaryOp2.first->NumArgs; i++) {
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

  // FCMP optimization

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

      auto fcmp = IREmit->GetOpHeader(ghf->GPR)->CW<IR::IROp_FCmp>();
      LogMan::Throw::A(fcmp->Header.Op == OP_FCMP || fcmp->Header.Op == OP_F80CMP, "Unexpected OP_GETHOSTFLAG source");
      if(fcmp->Header.Op == OP_FCMP) {
        fcmp->Flags |= 1 << ghf->Flag;
      }
    }
  }


  // LoadMem / StoreMem imm pooling
  // If imms are close by, use address gen to generate the values instead of using a new imm
  for (auto [BlockNode, BlockIROp] : CurrentIR.GetBlocks()) {
    for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {
      if (IROp->Op == OP_LOADMEM || IROp->Op == OP_STOREMEM) {
        uint64_t Addr;

        if (IREmit->IsValueConstant(IROp->Args[0], &Addr) && IROp->Args[1].IsInvalid()) {
          for (auto& Const: AddressgenConsts) {
            if ((Addr - Const.second) < 65536) {
              IREmit->ReplaceNodeArgument(CodeNode, 0, Const.first);
              IREmit->ReplaceNodeArgument(CodeNode, 1, IREmit->_Constant(Addr - Const.second));
              goto doneOp;
            }
          }

          AddressgenConsts[IREmit->UnwrapNode(IROp->Args[0])] = Addr;
        }
        doneOp:
        ;
      }
      IREmit->SetWriteCursor(CodeNode);
    }
    AddressgenConsts.clear();
  }

  for (auto [CodeNode, IROp] : CurrentIR.GetAllCode()) {
    // zext / masking elimination
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
        for (int i = 0; i < IROp->NumArgs; i++) {
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
        for (int i = 0; i < IROp->NumArgs; i++) {
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
        if (IsBfeAlreadyDone(IREmit, IROp->Args[0], Op->Width)) {
          IREmit->ReplaceAllUsesWith(CodeNode, CurrentIR.GetNode(IROp->Args[0]));
          //printf("Removed BFE once \n");
          break;
        }

        // Is this value already ZEXT'd?
        if (Op->lsb == 0) {
          //LoadMem, LoadMemTSO & LoadContext ZExt
          auto source = IROp->Args[0];
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

        auto newArg = RemoveUselessMasking(IREmit, IROp->Args[0], imm);

        if (newArg.ID() != IROp->Args[0].ID()) {
          IREmit->ReplaceNodeArgument(CodeNode, 0, IREmit->UnwrapNode(newArg));
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

        auto newArg = RemoveUselessMasking(IREmit, IROp->Args[0], imm);

        if (newArg.ID() != IROp->Args[0].ID()) {
          IREmit->ReplaceNodeArgument(CodeNode, 0, IREmit->UnwrapNode(newArg));
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
        for (int i = 0; i < IROp->NumArgs; i++) {
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
          //printf("printf vmov of same size?!\n");
          IREmit->ReplaceAllUsesWith(CodeNode, CurrentIR.GetNode(source));
        }
        break;
      }
      default: break;
    }

    // constprop + some more per instruction logic
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

    case OP_LOADMEM: {
      auto Op = IROp->CW<IR::IROp_LoadMem>();
      auto AddressHeader = IREmit->GetOpHeader(Op->Header.Args[0]);

      if (AddressHeader->Op == OP_ADD && AddressHeader->Size == 8) {

        auto [OffsetType, OffsetScale, Arg0, Arg1] = MemExtendedAddressing(IREmit, Op->Size, AddressHeader);

        Op->OffsetType = OffsetType;
        Op->OffsetScale = OffsetScale;
        IREmit->ReplaceNodeArgument(CodeNode, 0, Arg0);
        IREmit->ReplaceNodeArgument(CodeNode, 1, Arg1);

        Changed = true;
      }
      break;
    }

    case OP_STOREMEM: {
      auto Op = IROp->CW<IR::IROp_StoreMem>();
      auto AddressHeader = IREmit->GetOpHeader(Op->Header.Args[0]);

      if (AddressHeader->Op == OP_ADD && AddressHeader->Size == 8) {
        auto [OffsetType, OffsetScale, Arg0, Arg1] = MemExtendedAddressing(IREmit, Op->Size, AddressHeader);

        Op->OffsetType = OffsetType;
        Op->OffsetScale = OffsetScale;
        IREmit->ReplaceNodeArgument(CodeNode, 0, Arg0);
        IREmit->ReplaceNodeArgument(CodeNode, 2, Arg1);

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
        continue;
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
        uint64_t NewConstant = (Constant1 >> Constant2) & getMask(Op);
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
    case OP_MUL: {
      auto Op = IROp->C<IR::IROp_Mul>();
      uint64_t Constant1{};
      uint64_t Constant2{};

      if (IREmit->IsValueConstant(Op->Header.Args[0], &Constant1) &&
          IREmit->IsValueConstant(Op->Header.Args[1], &Constant2)) {
        uint64_t NewConstant = (Constant1 * Constant2) & getMask(Op);
        IREmit->ReplaceWithConstant(CodeNode, NewConstant);
        Changed = true;
      } else if (IREmit->IsValueConstant(Op->Header.Args[1], &Constant2) && __builtin_popcountl(Constant2) == 1) {
        if (IROp->Size == 4 || IROp->Size == 8) {
          uint64_t amt = __builtin_ctzl(Constant2);
          IREmit->SetWriteCursor(CodeNode);
          auto shift = IREmit->_Lshl(CurrentIR.GetNode(Op->Header.Args[0]), IREmit->_Constant(amt));
          shift.first->Header.Size = IROp->Size; // force Lshl to be the same size as the original Mul
          IREmit->ReplaceAllUsesWith(CodeNode, shift);
          Changed = true;
        }
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
    }
    default: break;
    }
  }

  // constant inlining
  if (InlineConstants) {
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

            IREmit->ReplaceNodeArgument(CodeNode, 1, IREmit->_InlineConstant(Constant2));

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

              IREmit->ReplaceNodeArgument(CodeNode, 1, IREmit->_InlineConstant(Constant2));

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

              IREmit->ReplaceNodeArgument(CodeNode, 1, IREmit->_InlineConstant(Constant1));

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

            IREmit->ReplaceNodeArgument(CodeNode, 2, IREmit->_InlineConstant(Constant2));
            IREmit->ReplaceNodeArgument(CodeNode, 3, IREmit->_InlineConstant(Constant3));
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

              IREmit->ReplaceNodeArgument(CodeNode, 1, IREmit->_InlineConstant(Constant2));

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

            IREmit->ReplaceNodeArgument(CodeNode, 0, IREmit->_InlineConstant(Constant));

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

              IREmit->ReplaceNodeArgument(CodeNode, 1, IREmit->_InlineConstant(Constant2));

              Changed = true;
            }
          }
          break;
        }

	case OP_LOADMEM:
        {
          auto Op = IROp->CW<IR::IROp_LoadMem>();

          uint64_t Constant2{};
          if (Op->OffsetType == MEM_OFFSET_SXTX && IREmit->IsValueConstant(Op->Header.Args[1], &Constant2)) {
            if (IsImmMemory(Constant2, Op->Size)) {
              IREmit->SetWriteCursor(CurrentIR.GetNode(Op->Header.Args[1]));

              IREmit->ReplaceNodeArgument(CodeNode, 1, IREmit->_InlineConstant(Constant2));

              Changed = true;
            }
          }
          break;
        }

	case OP_STOREMEM:
        {
          auto Op = IROp->CW<IR::IROp_StoreMem>();

          uint64_t Constant2{};
          if (Op->OffsetType == MEM_OFFSET_SXTX && IREmit->IsValueConstant(Op->Header.Args[2], &Constant2)) {
            if (IsImmMemory(Constant2, Op->Size)) {
              IREmit->SetWriteCursor(CurrentIR.GetNode(Op->Header.Args[2]));

              IREmit->ReplaceNodeArgument(CodeNode, 2, IREmit->_InlineConstant(Constant2));

              Changed = true;
            }
          }
          break;
        }

        default: break;
      }
    }
  }

  IREmit->SetWriteCursor(OriginalWriteCursor);

  return Changed;
}

FEXCore::IR::Pass* CreateConstProp(bool InlineConstants) {
  return new ConstProp(InlineConstants);
}

}
