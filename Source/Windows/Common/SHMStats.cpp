// SPDX-License-Identifier: MIT
#include "Windows/Common/FEXUnixLib.h"
#include "Windows/Common/SHMStats.h"

#include <FEXCore/fextl/fmt.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/TypeDefines.h>

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

  auto Result = UnixLib::AllocateSHMSlots(Base, std::min(NewSize, MAX_STATS_SIZE), MAX_STATS_SIZE);

  // Return the new size allocated, if it happened to change.
  return Result.MappedSize;
}

StatAlloc::StatAlloc(FEXCore::SHMStats::AppType AppType) {
  auto Result = UnixLib::AllocateSHMSlots(Base, FEXCore::Utils::FEX_PAGE_SIZE, MAX_STATS_SIZE);

  if (Result.SHMBase) {
    CurrentSize = Result.MappedSize;
    Base = Result.SHMBase;
    SaveHeader(AppType);
    return;
  }
}

StatAlloc::~StatAlloc() {
  UnixLib::DeleteSHMStatsFile();
}

} // namespace FEX::Windows
