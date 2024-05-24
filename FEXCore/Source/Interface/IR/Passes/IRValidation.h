// SPDX-License-Identifier: MIT
#pragma once

#include "Common/BitSet.h"
#include <FEXCore/IR/IR.h>
#include <FEXCore/fextl/unordered_map.h>
#include <FEXCore/fextl/vector.h>

namespace FEXCore::IR::Validation {

struct BlockInfo {
  bool HasExit;
  const OrderedNode* BlockNode;

  fextl::vector<OrderedNode*> Predecessors;
  fextl::vector<OrderedNode*> Successors;
};

class RAValidation;

class IRValidation final : public FEXCore::IR::Pass {
public:
  ~IRValidation();
  void Run(IREmitter* IREmit) override;

private:

  BitSet<uint64_t> NodeIsLive;
  OrderedNode* EntryBlock;
  fextl::unordered_map<IR::NodeID, BlockInfo> OffsetToBlockMap;
  size_t MaxNodes {};

  friend class RAValidation;
};
} // namespace FEXCore::IR::Validation
