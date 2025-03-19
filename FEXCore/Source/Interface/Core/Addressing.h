// SPDX-License-Identifier: MIT
#pragma once

#include "Interface/IR/IR.h"
#include <cstdint>

namespace FEXCore::IR {
class IREmitter;

struct AddressMode {
  Ref Segment {nullptr};
  Ref Base {nullptr};
  Ref Index {nullptr};
  MemOffsetType IndexType = MEM_OFFSET_SXTX;
  uint8_t IndexScale = 1;
  int64_t Offset = 0;

  // Size in bytes for the address calculation. 8 for an arm64 hardware mode.
  IR::OpSize AddrSize;
  bool NonTSO;
};

Ref LoadEffectiveAddress(IREmitter* IREmit, AddressMode A, IR::OpSize GPRSize, bool AddSegmentBase, bool AllowUpperGarbage = false);
AddressMode SelectAddressMode(IREmitter* IREmit, AddressMode A, IR::OpSize GPRSize, bool HostSupportsTSOImm9, bool AtomicTSO, bool Vector,
                              IR::OpSize AccessSize);

}; // namespace FEXCore::IR