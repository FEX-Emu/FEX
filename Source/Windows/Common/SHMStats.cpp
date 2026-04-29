// SPDX-License-Identifier: MIT
#include "Windows/Common/SHMStats.h"

#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/TypeDefines.h>

#include <ntstatus.h>
#include <windef.h>
#include <winternl.h>
#include <winnt.h>
#include <wine/unixlib.h>

#include "Unixlib/FEXUnixlib.h"

namespace FEX::Windows {

uint32_t StatAlloc::FrontendAllocateSlots(uint32_t NewSize) {
  if (CurrentSize == MAX_STATS_SIZE) {
    LogMan::Msg::DFmt("Ran out of slots. Can't allocate more");
    return CurrentSize;
  }

  FexStatsSHMParams Params {
    .shm_base = nullptr,
    .map_size = std::min(CurrentSize * 2, MAX_STATS_SIZE),
    .max_size = MAX_STATS_SIZE,
  };
  if (!WINE_UNIX_CALL(fex_unix_get_stats_shm, &Params)) {
    CurrentSize = Params.map_size;
  }

  return CurrentSize;
}

StatAlloc::StatAlloc(FEXCore::SHMStats::AppType AppType) {
  FexStatsSHMParams Params {
    .shm_base = nullptr,
    .map_size = FEXCore::Utils::FEX_PAGE_SIZE,
    .max_size = MAX_STATS_SIZE,
  };
  if (WINE_UNIX_CALL(fex_unix_get_stats_shm, &Params)) {
    return;
  }

  CurrentSize = Params.map_size;
  Base = Params.shm_base;
  SaveHeader(AppType);
}

StatAlloc::~StatAlloc() {}

} // namespace FEX::Windows
