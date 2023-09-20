// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <unordered_map>

namespace FEXCore {
class BlockSamplingData {
public:
  struct BlockData {
    uint64_t Start, End;
    uint64_t Min, Max;
    uint64_t TotalTime;
    uint64_t TotalCalls;
  };

  BlockData *GetBlockData(uint64_t RIP);
  ~BlockSamplingData();

  void DumpBlockData();

private:
  std::unordered_map<uint64_t, BlockData*> SamplingMap;
};
}
