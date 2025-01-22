// SPDX-License-Identifier: MIT
#pragma once

#include "Common/Profiler.h"

namespace FEX::Windows {
class StatAlloc final : public FEX::Profiler::StatAllocBase {
public:
  StatAlloc(FEXCore::Profiler::AppType AppType);
  virtual ~StatAlloc();

  FEXCore::Profiler::ThreadStats* AllocateSlot(uint32_t TID) {
    return AllocateBaseSlot(TID);
  }

  void DeallocateSlot(FEXCore::Profiler::ThreadStats* AllocatedSlot) {
    if (!AllocatedSlot) {
      return;
    }

    DeallocateBaseSlot(AllocatedSlot);
  }

private:
  uint64_t AllocateMoreSlots(uint64_t NewSize) override;
};

} // namespace FEX::Windows
