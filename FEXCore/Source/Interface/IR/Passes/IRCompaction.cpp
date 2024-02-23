// SPDX-License-Identifier: MIT
/*
$info$
tags: ir|opts
desc: Sorts the ssa storage in memory, needed for RA and others
$end_info$
*/

#include "Interface/IR/IREmitter.h"
#include "Interface/IR/PassManager.h"
#include "Interface/Core/OpcodeDispatcher.h"

#include <FEXCore/IR/IR.h>
#include <FEXCore/IR/IntrusiveIRList.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/MathUtils.h>
#include <FEXCore/Utils/Profiler.h>
#include <FEXCore/fextl/vector.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>

namespace FEXCore::IR {

// struct to avoid zero-initialization
struct RemapNode {
  IR::NodeID NodeID;
};

static_assert(sizeof(RemapNode) == 4);

class IRCompaction final : public FEXCore::IR::Pass {
public:
  IRCompaction(FEXCore::Utils::IntrusivePooledAllocator& Allocator);
  bool Run(IREmitter* IREmit) override;

private:
  static constexpr size_t AlignSize = 0x2000;
  OpDispatchBuilder LocalBuilder;
  fextl::vector<RemapNode> OldToNewRemap;
  struct CodeBlockData {
    OrderedNode* OldNode;
    OrderedNode* NewNode;
  };

  fextl::vector<CodeBlockData> GeneratedCodeBlocks {};
};

IRCompaction::IRCompaction(FEXCore::Utils::IntrusivePooledAllocator& Allocator)
  : LocalBuilder {Allocator} {
  OldToNewRemap.resize(AlignSize);
}

bool IRCompaction::Run(IREmitter* IREmit) {
  FEXCORE_PROFILE_SCOPED("PassManager::IRCompaction");

  LocalBuilder.ReownOrClaimBuffer();

  auto CurrentIR = IREmit->ViewIR();
  uint32_t NodeCount = CurrentIR.GetSSACount();

  if (OldToNewRemap.size() < NodeCount) {
    OldToNewRemap.resize(std::max(OldToNewRemap.size() * 2U, AlignUp(NodeCount, AlignSize)));
  }
#ifndef NDEBUG
  memset(&OldToNewRemap.at(0), 0xFF, NodeCount * sizeof(RemapNode));
#endif

  GeneratedCodeBlocks.clear();

  // Reset our local working list
  LocalBuilder.ResetWorkingList();
  auto LocalIR = LocalBuilder.ViewIR();

  uintptr_t LocalListBegin = LocalIR.GetListData();
  uintptr_t LocalDataBegin = LocalIR.GetData();

  uintptr_t ListBegin = CurrentIR.GetListData();

  auto HeaderNode = CurrentIR.GetHeaderNode();
  auto HeaderOp = CurrentIR.GetHeader();
  LOGMAN_THROW_AA_FMT(HeaderOp->Header.Op == OP_IRHEADER, "First op wasn't IRHeader");

  // This compaction pass is something that we need to ensure correct ordering and distances between IROps
  // Later on we assume that an IROp's SSA value live range is its Node locations
  //
  // RA distance calculation is calculated purely on the Node locations
  // So we need to reorder those
  //
  // Additionally there may be some dead ops hanging out in the IR list that are orphaned.
  // These can also be dropped during this pass

  // First thing is first, we need to do some housekeeping
  // Create the IRHeader op
  // Create the codeblocks
  // Then create all the ops inside the code blocks

  // Zero is always zero(invalid)
  OldToNewRemap[0].NodeID.Invalidate();
  auto LocalHeaderOp = LocalBuilder._IRHeader(OrderedNodeWrapper::WrapOffset(0).GetNode(ListBegin), HeaderOp->OriginalRIP,
                                              HeaderOp->BlockCount, HeaderOp->NumHostInstructions);

  OldToNewRemap[CurrentIR.GetID(HeaderNode).Value].NodeID = LocalIR.GetID(LocalHeaderOp.Node);

  {
    // Generate our codeblocks and link them together
    for (auto [BlockNode, BlockHeader] : CurrentIR.GetBlocks()) {
      LOGMAN_THROW_AA_FMT(BlockHeader->Op == OP_CODEBLOCK, "IR type failed to be a code block");

      auto LocalBlockIRNode = LocalBuilder._CodeBlock(LocalHeaderOp, LocalHeaderOp); // Use LocalHeaderOp as a dummy arg for now
      OldToNewRemap[CurrentIR.GetID(BlockNode).Value].NodeID = LocalIR.GetID(LocalBlockIRNode.Node);
      GeneratedCodeBlocks.emplace_back(CodeBlockData {BlockNode, LocalBlockIRNode});
    }

    // Link the IRHeader to the first code block
    LocalHeaderOp.first->Blocks = GeneratedCodeBlocks[0].NewNode->Wrapped(LocalListBegin);
  }

  {
    // Copy all of our IR ops over to the new location
    for (auto& Block : GeneratedCodeBlocks) {

      // Isolate block contents from any previous headers/blocks
      LocalBuilder.SetWriteCursor(nullptr);

      CodeBlockData FirstNode {};
      CodeBlockData LastNode {};
      uint32_t i {};
      for (auto [CodeNode, IROp] : CurrentIR.GetCode(Block.OldNode)) {
        const size_t OpSize = FEXCore::IR::GetSize(IROp->Op);
        const auto CodeID = CurrentIR.GetID(CodeNode);

        // Allocate the ops locally for our local dispatch
        auto LocalPair = LocalBuilder.AllocateRawOp(OpSize);

        // Copy usage infomation
        LocalPair.Node->NumUses = CodeNode->GetUses();

        // Copy over the op
        memcpy(LocalPair.first, IROp, OpSize);

        // Set our map remapper to map the new location
        // Even nodes that don't have a destination need to be in this map
        // Need to be able to remap branch targets any other bits
        OldToNewRemap[CodeID.Value].NodeID = LocalIR.GetID(LocalPair.Node);

        if (i == 0) {
          FirstNode.OldNode = CodeNode;
          FirstNode.NewNode = LocalPair.Node;
        }

        if (IROp->Op == OP_ENDBLOCK) {
          LastNode.OldNode = CodeNode;
          LastNode.NewNode = LocalPair.Node;
        }

        ++i;
      }

      // Set the code block's begin and end correctly
      auto NewBlockIROp = Block.NewNode->Op(LocalDataBegin)->CW<FEXCore::IR::IROp_CodeBlock>();
      NewBlockIROp->Begin = FirstNode.NewNode->Wrapped(LocalListBegin);
      NewBlockIROp->Last = LastNode.NewNode->Wrapped(LocalListBegin);
    }
  }

  {
    // Fixup the arguments of all the IROps
    for (auto& Block : GeneratedCodeBlocks) {
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
      auto BlockIROp = LocalIR.GetOp<FEXCore::IR::IROp_CodeBlock>(Block.NewNode);
      LOGMAN_THROW_AA_FMT(BlockIROp->Header.Op == OP_CODEBLOCK, "IR type failed to be a code block");
#endif

      for (auto [LocalNode, LocalIROp] : LocalIR.GetCode(Block.NewNode)) {

        // Now that we have the op copied over, we need to modify SSA values to point to the new correct locations
        // This doesn't use IR::GetRAArgs(Op) because we need to remap all SSA nodes
        // Including ones that we don't RA
        const uint8_t NumArgs = IR::GetArgs(LocalIROp->Op);
        for (uint8_t i = 0; i < NumArgs; ++i) {
          const auto OldArg = LocalIROp->Args[i].ID();
          const auto NewArg = OldToNewRemap[OldArg.Value].NodeID;

#ifndef NDEBUG
          LOGMAN_THROW_A_FMT(NewArg.Value != UINT32_MAX, "Tried remapping unfound node %{}", OldArg);
#endif

          LocalIROp->Args[i].NodeOffset = NewArg.Value * sizeof(OrderedNode);
        }
      }
    }
  }

  // uintptr_t OldListSize = CurrentIR.GetListSize();
  // uintptr_t OldDataSize = CurrentIR.GetDataSize();

  // uintptr_t NewListSize = LocalIR.GetListSize();
  // uintptr_t NewDataSize = LocalIR.GetDataSize();

  // if (NewListSize < OldListSize ||
  //     NewDataSize < OldDataSize) {
  //   if (NewListSize < OldListSize) {
  //     LogMan::Msg::DFmt("Shaved {} bytes off the list size", OldListSize - NewListSize);
  //   }
  //   if (NewDataSize < OldDataSize) {
  //     LogMan::Msg::DFmt("Shaved {} bytes off the data size", OldDataSize - NewDataSize);
  //   }
  // }

  // if (NewListSize > OldListSize ||
  //     NewDataSize > OldDataSize) {
  //   LOGMAN_MSG_A_FMT("Whoa. Compaction made the IR a different size when it shouldn't have. 0x{:x} > 0x{:x} or 0x{:x} > 0x{:x}",
  //                    NewListSize, OldListSize, NewDataSize, OldDataSize);
  // }

  IREmit->CopyData(LocalBuilder);

  LocalBuilder.DelayedDisownBuffer();
  return true;
}

fextl::unique_ptr<FEXCore::IR::Pass> CreateIRCompaction(FEXCore::Utils::IntrusivePooledAllocator& Allocator) {
  return fextl::make_unique<IRCompaction>(Allocator);
}

} // namespace FEXCore::IR
