#include "Interface/Core/BlockSamplingData.h"
#include "LogManager.h"
#include <cstring>
#include <fstream>

namespace FEXCore {
  void BlockSamplingData::DumpBlockData() {
    std::fstream Output;
    Output.open("output.csv", std::fstream::out | std::fstream::binary);

    if (!Output.is_open())
      return;

    Output << "Entry, Min, Max, Total, Calls, Average" << std::endl;

    for (auto it : SamplingMap) {
      if (!it.second->TotalCalls)
        continue;

      Output << "0x" << std::hex << it.first
             << ", " << std::dec << it.second->Min
             << ", " << std::dec << it.second->Max
             << ", " << std::dec << it.second->TotalTime
             << ", " << std::dec << it.second->TotalCalls
             << ", " << std::dec << ((double)it.second->TotalTime / (double)it.second->TotalCalls)
             << std::endl;
    }
    Output.close();
    LogMan::Msg::D("Dumped %d blocks of sampling data", SamplingMap.size());
  }

  BlockSamplingData::BlockData *BlockSamplingData::GetBlockData(uint64_t RIP) {
    auto it = SamplingMap.find(RIP);
    if (it != SamplingMap.end()) {
      return it->second;
    }
    BlockData *NewData = new BlockData{};
    memset(NewData, 0, sizeof(BlockData));
    NewData->Min = ~0ULL;
    SamplingMap[RIP] = NewData;
    return NewData;
  }

  BlockSamplingData::~BlockSamplingData() {
    DumpBlockData();
    for (auto it : SamplingMap) {
      delete it.second;
    }
    SamplingMap.clear();
  }
}
