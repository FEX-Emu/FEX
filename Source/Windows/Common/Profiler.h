// SPDX-License-Identifier: MIT
#pragma once

#include "Common/Profiler.h"

namespace FEX::Windows {
class StatAlloc final : public FEX::Profiler::StatAllocBase {
public:
  StatAlloc(FEXCore::Profiler::AppType AppType);
  virtual ~StatAlloc();

  FEXCore::Profiler::ThreadStats* AllocateSlot(uint32_t TID) {
    return StatAllocBase::AllocateSlot(TID);
  }

  void DeallocateSlot(FEXCore::Profiler::ThreadStats* AllocatedSlot) {
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
