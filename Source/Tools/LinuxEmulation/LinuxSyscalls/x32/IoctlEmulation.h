// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/fextl/map.h>

#include <cstdint>

namespace FEXCore::Core {
struct CpuStateFrame;
}

namespace FEX::HLE::x32 {
class DRMLRUCacheFDCache final {
public:
  using HandlerType = uint32_t (*)(FEXCore::Core::CpuStateFrame* Frame, int fd, uint32_t cmd, uint32_t args);
  DRMLRUCacheFDCache();
  void SetFDHandler(uint32_t FD, HandlerType Handler);
  void DuplicateFD(int fd, int NewFD);
  HandlerType FindHandler(int32_t FD);
  uint32_t AddAndRunMapHandler(FEXCore::Core::CpuStateFrame* Frame, int fd, uint32_t cmd, uint32_t args);

protected:
  constexpr static size_t LRUSize = 3;
  void AddToFront(int32_t FD, HandlerType Handler);

  struct LRUObject {
    int32_t FD;
    HandlerType Handler;
  };
  // With four elements total (3 + 1) then this is a single cacheline in size
  LRUObject LRUCache[LRUSize + 1];

  fextl::map<int32_t, HandlerType> FDToHandler;
};

uint32_t ioctl32(FEXCore::Core::CpuStateFrame* Frame, int fd, uint32_t request, uint32_t args);
void CheckAndAddFDDuplication(FEXCore::Core::CpuStateFrame* Frame, int fd, int NewFD);
} // namespace FEX::HLE::x32
