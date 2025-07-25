// SPDX-License-Identifier: MIT
#pragma once

#include "Common/SHMStats.h"

namespace FEX::Windows {
class StatAlloc final : public FEX::SHMStats::StatAllocBase {
public:
  StatAlloc(FEXCore::SHMStats::AppType AppType);
  virtual ~StatAlloc();

  FEXCore::SHMStats::ThreadStats* AllocateSlot(uint32_t TID) {
    return StatAllocBase::AllocateSlot(TID);
  }

  void DeallocateSlot(FEXCore::SHMStats::ThreadStats* AllocatedSlot) {
    if (!AllocatedSlot) {
      return;
    }

    StatAllocBase::DeallocateSlot(AllocatedSlot);
  }

private:
  uint32_t FrontendAllocateSlots(uint32_t NewSize) override;
  bool UsingNTQueryPath {};
};

} // namespace FEX::Windows
