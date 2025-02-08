// SPDX-License-Identifier: MIT
#include "Windows/Common/Profiler.h"

#include <FEXCore/fextl/fmt.h>
#include <FEXCore/Utils/LogManager.h>

#include <ntstatus.h>
#include <windef.h>
#include <winternl.h>
#include <winnt.h>
#include <wine/debug.h>

namespace FEX::Windows {
uint32_t StatAlloc::FrontendAllocateSlots(uint32_t NewSize) {
  if (CurrentSize == MAX_STATS_SIZE) {
    LogMan::Msg::DFmt("Ran out of slots. Can't allocate more");
    return CurrentSize;
  }

  MEMORY_FEX_STATS_SHM_INFORMATION Info {
    .shm_base = nullptr,
    .map_size = std::min(CurrentSize * 2, MAX_STATS_SIZE),
    .max_size = MAX_STATS_SIZE,
  };
  size_t Length {};
  auto Result = NtQueryVirtualMemory(NtCurrentProcess(), nullptr, MemoryFexStatsShm, &Info, sizeof(Info), &Length);
  if (!Result) {
    CurrentSize = Info.map_size;
  }

  return CurrentSize;
}

StatAlloc::StatAlloc(FEXCore::Profiler::AppType AppType) {
  MEMORY_FEX_STATS_SHM_INFORMATION Info {
    .shm_base = nullptr,
    .map_size = 4096,
    .max_size = MAX_STATS_SIZE,
  };
  size_t Length {};
  auto Result = NtQueryVirtualMemory(NtCurrentProcess(), nullptr, MemoryFexStatsShm, &Info, sizeof(Info), &Length);
  if (!Result) {
    CurrentSize = Info.map_size;
    Base = Info.shm_base;
    SaveHeader(AppType);
    return;
  }
}

} // namespace FEX::Windows
