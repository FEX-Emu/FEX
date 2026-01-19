// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/fextl/map.h>

#include <cstdint>

namespace FEXCore::Core {
struct CpuStateFrame;
}

namespace FEX::HLE::x32 {
class LRUCacheFDCache {
public:
  using HandlerType = uint32_t (*)(FEXCore::Core::CpuStateFrame* Frame, int fd, uint32_t cmd, uint32_t args);
  virtual ~LRUCacheFDCache() = default;
  void SetFDHandler(uint32_t FD, HandlerType Handler) {
    FDToHandler[FD] = Handler;
  }

  void DuplicateFD(int fd, int NewFD) {
    auto it = FDToHandler.find(fd);
    if (it != FDToHandler.end()) {
      FDToHandler[NewFD] = it->second;
    }
  }

  HandlerType FindHandler(int32_t FD) {
    HandlerType Handler {};
    for (size_t i = 0; i < LRUSize; ++i) {
      auto& it = LRUCache[i];
      if (it.FD == FD) {
        if (i == 0) {
          // If we are the first in the queue then just return it
          return it.Handler;
        }
        Handler = it.Handler;
        break;
      }
    }

    if (Handler) {
      AddToFront(FD, Handler);
      return Handler;
    }
    return LRUCache[LRUSize].Handler;
  }

  virtual uint32_t AddAndRunMapHandler(FEXCore::Core::CpuStateFrame* Frame, int fd, uint32_t cmd, uint32_t args) = 0;
protected:
  constexpr static size_t LRUSize = 3;
  void AddToFront(int32_t FD, HandlerType Handler) {
    // Push the element to the front if we found one
    // First copy all the other elements back one
    // Ensuring the final element isn't written over
    memmove(&LRUCache[1], &LRUCache[0], (LRUSize - 1) * sizeof(LRUCache[0]));
    // Now set the first element to the one we just found
    LRUCache[0] = LRUObject {FD, Handler};
  }

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
