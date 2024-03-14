// SPDX-License-Identifier: MIT
/*
$info$
tags: ir|opts
desc: Cross block store-after-store elimination
$end_info$
*/

#include "Interface/IR/IREmitter.h"
#include "Interface/IR/PassManager.h"

#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/Profiler.h>
#include <FEXCore/fextl/unordered_map.h>

#include <memory>
#include <stddef.h>
#include <stdint.h>

namespace FEXCore::IR {

constexpr int PropagationRounds = 5;

class DeadStoreElimination final : public FEXCore::IR::Pass {
public:
  explicit DeadStoreElimination(bool SupportsAVX_) : SupportsAVX{SupportsAVX_} {}

  bool Run(IREmitter *IREmit) override;

private:
  bool SupportsAVX;

  bool IsFPR(uint32_t Offset) const {
    const auto [begin, end] = [this]() -> std::pair<ptrdiff_t, ptrdiff_t> {
      if (SupportsAVX) {
        return {
          offsetof(FEXCore::Core::CpuStateFrame, State.xmm.avx.data[0][0]),
          offsetof(FEXCore::Core::CpuStateFrame, State.xmm.avx.data[16][0])
        };
      } else {
        return {
          offsetof(FEXCore::Core::CpuStateFrame, State.xmm.sse.data[0][0]),
          offsetof(FEXCore::Core::CpuStateFrame, State.xmm.sse.data[16][0])
        };
      }
    }();

    if (Offset < begin || Offset >= end)
      return false;

    return true;
  }

  bool IsTrackedWriteFPR(uint32_t Offset, uint8_t Size) const {
    if (Size != 16 && Size != 8 && Size != 4)
      return false;
    if (Offset & 15)
      return false;

    return IsFPR(Offset);
  }

  uint64_t FPRBit(uint32_t Offset, uint32_t Size) const {
    if (!IsFPR(Offset)) {
      return 0;
    }

    const auto begin = offsetof(Core::CpuStateFrame, State.xmm.avx.data[0][0]);

    const auto regSize = SupportsAVX ? Core::CPUState::XMM_AVX_REG_SIZE
                                     : Core::CPUState::XMM_SSE_REG_SIZE;
    const auto regn = (Offset - begin) / regSize;
    const auto bitn = regn * 3;

    if (!IsTrackedWriteFPR(Offset, Size))
      return 7UL << (bitn);

    if (Size == 16)
      return  7UL << (bitn);
    else if (Size == 8)
      return  3UL << (bitn);
    else if (Size == 4)
      return  1UL << (bitn);
    else
      LOGMAN_MSG_A_FMT("Unexpected FPR size {}", Size);

    return 7UL << (bitn); // Return maximum on failure case
  }
};

struct FlagInfo {
  uint64_t reads { 0 };
  uint64_t writes { 0 };
  uint64_t kill { 0 };
};


struct GPRInfo {
  uint32_t reads { 0 };
  uint32_t writes { 0 };
  uint32_t kill { 0 };
};

bool IsFullGPR(uint32_t Offset, uint8_t Size) {
  if (Size != 8)
    return false;
  if (Offset & 7)
    return false;

  if (Offset < 8 || Offset >= (17 * 8))
    return false;

  return true;
}

bool IsGPR(uint32_t Offset) {

  if (Offset < 8 || Offset >= (17 * 8))
    return false;

  return true;
}

uint32_t GPRBit(uint32_t Offset) {
  if (!IsGPR(Offset)) {
    return 0;
  }

  return  1 << ((Offset - 8)/8);
}

struct FPRInfo {
  uint64_t reads { 0 };
  uint64_t writes { 0 };
  uint64_t kill { 0 };
};

struct Info {
  FlagInfo flag;
  GPRInfo gpr;
  FPRInfo fpr;
};

/**
 * @brief This is a temporary pass to detect simple multiblock dead flag/gpr/fpr stores
 *
 * First pass computes which flags/gprs/fprs are read and written per block
 *
 * Second pass computes which flags/gprs/fprs are stored, but overwritten by the next block(s).
 * It also propagates this information a few times to catch dead flags/gprs/fprs across multiple blocks.
 *
 * Third pass removes the dead stores.
 *
 */
bool DeadStoreElimination::Run(IREmitter *IREmit) {
  FEXCORE_PROFILE_SCOPED("PassManager::DSE");

  fextl::unordered_map<OrderedNode*, Info> InfoMap;

  bool Changed = false;
  auto CurrentIR = IREmit->ViewIR();

  // Pass 1
  // Compute flags/gprs/fprs read/writes per block
  // This is conservative and doesn't try to be smart about loads after writes
  {
    for (auto [BlockNode, BlockIROp] : CurrentIR.GetBlocks()) {
      for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {

        auto ClassifyRegisterStore = [this](Info &BlockInfo, uint32_t Offset, uint8_t Size) {
          //// GPR ////
          if (IsFullGPR(Offset, Size))
            BlockInfo.gpr.writes |= GPRBit(Offset);
          else
            BlockInfo.gpr.reads |= GPRBit(Offset);

          //// FPR ////
          if (IsTrackedWriteFPR(Offset, Size))
            BlockInfo.fpr.writes |= FPRBit(Offset, Size);
          else
            BlockInfo.fpr.reads |= FPRBit(Offset, Size);
        };

        auto ClassifyRegisterLoad = [this](Info &BlockInfo, uint32_t Offset, uint8_t Size) {
          //// GPR ////
          BlockInfo.gpr.reads |= GPRBit(Offset);

          //// FPR ////
          BlockInfo.fpr.reads |= FPRBit(Offset, Size);
        };

        //// Flags ////
        if (IROp->Op == OP_STOREFLAG) {
          auto Op = IROp->C<IR::IROp_StoreFlag>();

          auto& BlockInfo = InfoMap[BlockNode];

          BlockInfo.flag.writes |= 1UL << Op->Flag;
        } else if  (IROp->Op == OP_INVALIDATEFLAGS) {
          auto Op = IROp->C<IR::IROp_InvalidateFlags>();

          auto& BlockInfo = InfoMap[BlockNode];

          BlockInfo.flag.writes |= Op->Flags;
        } else if (IROp->Op == OP_LOADFLAG) {
          auto Op = IROp->C<IR::IROp_LoadFlag>();

          auto& BlockInfo = InfoMap[BlockNode];

          BlockInfo.flag.reads |= 1UL << Op->Flag;
        } else if (IROp->Op == OP_LOADDF) {
          auto& BlockInfo = InfoMap[BlockNode];

          BlockInfo.flag.reads |= 1UL << X86State::RFLAG_DF_RAW_LOC;
        } else if (IROp->Op == OP_STOREREGISTER) {
          auto Op = IROp->C<IR::IROp_StoreRegister>();

          auto& BlockInfo = InfoMap[BlockNode];
          ClassifyRegisterStore(BlockInfo, Op->Offset, IROp->Size);
        } else if (IROp->Op == OP_LOADREGISTER) {
          auto Op = IROp->C<IR::IROp_LoadRegister>();

          auto& BlockInfo = InfoMap[BlockNode];
          ClassifyRegisterLoad(BlockInfo, Op->Offset, IROp->Size);
        }
      }
    }
  }

  // Pass 2
  // Compute flags/gprs/fprs that are stored, but always ovewritten in the next blocks
  // Propagate the information a few times to eliminate more
  for (int i = 0; i < PropagationRounds; i++)
  {
    for (auto [BlockNode, BlockIROp] : CurrentIR.GetBlocks()) {
      auto CodeBlock = BlockIROp->C<IROp_CodeBlock>();

      auto IROp = CurrentIR.GetNode(CurrentIR.GetNode(CodeBlock->Last)->Header.Previous)->Op(CurrentIR.GetData());

      if (IROp->Op == OP_JUMP) {
        auto Op = IROp->C<IR::IROp_Jump>();
        OrderedNode *TargetNode = CurrentIR.GetNode(Op->Header.Args[0]);

        auto& BlockInfo = InfoMap[BlockNode];
        auto& TargetInfo = InfoMap[TargetNode];

        //// Flags ////

        // stores to remove are written by the next block but not read
        BlockInfo.flag.kill = TargetInfo.flag.writes & ~(TargetInfo.flag.reads) & ~BlockInfo.flag.reads;

        // Flags that are written by the next block can be considered as written by this block, if not read
        BlockInfo.flag.writes |= BlockInfo.flag.kill & ~BlockInfo.flag.reads;


        //// GPRs ////

        // stores to remove are written by the next block but not read
        BlockInfo.gpr.kill = TargetInfo.gpr.writes & ~(TargetInfo.gpr.reads) & ~BlockInfo.gpr.reads;

        // GPRs that are written by the next block can be considered as written by this block, if not read
        BlockInfo.gpr.writes |= BlockInfo.gpr.kill & ~BlockInfo.gpr.reads;


        //// FPRs ////

        // stores to remove are written by the next block but not read
        BlockInfo.fpr.kill = TargetInfo.fpr.writes & ~(TargetInfo.fpr.reads) & ~BlockInfo.fpr.reads;

        // FPRs that are written by the next block can be considered as written by this block, if not read
        BlockInfo.fpr.writes |= BlockInfo.fpr.kill & ~BlockInfo.fpr.reads;

      } else if (IROp->Op == OP_CONDJUMP) {
        auto Op = IROp->C<IR::IROp_CondJump>();

        OrderedNode *TrueTargetNode = CurrentIR.GetNode(Op->TrueBlock);
        OrderedNode *FalseTargetNode = CurrentIR.GetNode(Op->FalseBlock);

        auto& BlockInfo = InfoMap[BlockNode];
        auto& TrueTargetInfo = InfoMap[TrueTargetNode];
        auto& FalseTargetInfo = InfoMap[FalseTargetNode];

        //// Flags ////

        // stores to remove are written by the next blocks but not read
        BlockInfo.flag.kill = TrueTargetInfo.flag.writes & ~(TrueTargetInfo.flag.reads) & ~BlockInfo.flag.reads;
        BlockInfo.flag.kill &= FalseTargetInfo.flag.writes & ~(FalseTargetInfo.flag.reads) & ~BlockInfo.flag.reads;

        // Flags that are written by the next blocks can be considered as written by this block, if not read
        BlockInfo.flag.writes |= BlockInfo.flag.kill & ~BlockInfo.flag.reads;


        //// GPRs ////

        // stores to remove are written by the next blocks but not read
        BlockInfo.gpr.kill = TrueTargetInfo.gpr.writes & ~(TrueTargetInfo.gpr.reads) & ~BlockInfo.gpr.reads;
        BlockInfo.gpr.kill &= FalseTargetInfo.gpr.writes & ~(FalseTargetInfo.gpr.reads) & ~BlockInfo.gpr.reads;

        // GPRs that are written by the next blocks can be considered as written by this block, if not read
        BlockInfo.gpr.writes |= BlockInfo.gpr.kill & ~BlockInfo.gpr.reads;


        //// FPRs ////

        // stores to remove are written by the next blocks but not read
        BlockInfo.fpr.kill = TrueTargetInfo.fpr.writes & ~(TrueTargetInfo.fpr.reads) & ~BlockInfo.fpr.reads;
        BlockInfo.fpr.kill &= FalseTargetInfo.fpr.writes & ~(FalseTargetInfo.fpr.reads) & ~BlockInfo.fpr.reads;

        // FPRs that are written by the next blocks can be considered as written by this block, if not read
        BlockInfo.fpr.writes |= BlockInfo.fpr.kill & ~BlockInfo.fpr.reads;
      }
    }
  }

  // Pass 3
  // Remove the dead stores
  {
    for (auto [BlockNode, BlockIROp] : CurrentIR.GetBlocks()) {
      for (auto [CodeNode, IROp] : CurrentIR.GetCode(BlockNode)) {

        auto RemoveDeadRegisterStore = [this](FEXCore::IR::IREmitter *IREmit, FEXCore::IR::OrderedNode *CodeNode, Info &BlockInfo, uint32_t Offset, uint8_t Size) -> bool {
          bool Changed{};
          //// GPRs ////
          // If this OP_STOREREGISTER is never read, remove it
          if (BlockInfo.gpr.kill & GPRBit(Offset)) {
            IREmit->Remove(CodeNode);
            Changed = true;
          }

          //// FPRs ////
          // If this OP_STOREREGISTER is never read, remove it
          if ((BlockInfo.fpr.kill & FPRBit(Offset, Size)) == FPRBit(Offset, Size) && (FPRBit(Offset, Size) != 0)) {
            IREmit->Remove(CodeNode);
            Changed = true;
          }

          return Changed;
        };

        //// Flags ////
        if (IROp->Op == OP_STOREFLAG) {
          auto Op = IROp->C<IR::IROp_StoreFlag>();

          auto& BlockInfo = InfoMap[BlockNode];

          // If this StoreFlag is never read, remove it
          if (BlockInfo.flag.kill & (1UL << Op->Flag)) {
            IREmit->Remove(CodeNode);
            Changed = true;
          }
        } else if (IROp->Op == OP_STOREREGISTER) {
          auto Op = IROp->C<IR::IROp_StoreRegister>();

          auto& BlockInfo = InfoMap[BlockNode];

          Changed |= RemoveDeadRegisterStore(IREmit, CodeNode, BlockInfo, Op->Offset, IROp->Size);
        }
      }
    }
  }

  return Changed;
}

fextl::unique_ptr<FEXCore::IR::Pass> CreateDeadStoreElimination(bool SupportsAVX) {
  return fextl::make_unique<DeadStoreElimination>(SupportsAVX);
}

}
