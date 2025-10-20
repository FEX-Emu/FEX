// SPDX-License-Identifier: MIT
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
__attribute__((naked)) uint64_t linux_getpid() {
  asm volatile(R"(
  mov x8, 172;
  svc #0;
  ret;
  )" ::
                 : "r0", "r8");
}

uint32_t StatAlloc::FrontendAllocateSlots(uint32_t NewSize) {
  if (CurrentSize == MAX_STATS_SIZE || !UsingNTQueryPath) {
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

StatAlloc::StatAlloc(FEXCore::SHMStats::AppType AppType) {
  // Try wine+fex magic path.

  {
    MEMORY_FEX_STATS_SHM_INFORMATION Info {
      .shm_base = nullptr,
      .map_size = FEXCore::Utils::FEX_PAGE_SIZE,
      .max_size = MAX_STATS_SIZE,
    };
    size_t Length {};
    auto Result = NtQueryVirtualMemory(NtCurrentProcess(), nullptr, MemoryFexStatsShm, &Info, sizeof(Info), &Length);
    if (!Result) {
      UsingNTQueryPath = true;
      CurrentSize = Info.map_size;
      Base = Info.shm_base;
      SaveHeader(AppType);
      return;
    }
  }
  CurrentSize = MAX_STATS_SIZE;

  auto handle = CreateFile(fextl::fmt::format("/dev/shm/fex-{}-stats", linux_getpid()).c_str(), GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

  // Create the section mapping for the file handle for the full size.
  HANDLE SectionMapping;
  LARGE_INTEGER SectionSize {{MAX_STATS_SIZE}};
  auto Result = NtCreateSection(&SectionMapping, SECTION_EXTEND_SIZE | SECTION_MAP_READ | SECTION_MAP_WRITE, nullptr, &SectionSize,
                                PAGE_READWRITE, SEC_COMMIT, handle);
  if (Result != 0) {
    CloseHandle(handle);
    return;
  }

  // Section mapping is used from now on.
  CloseHandle(handle);

  // Now actually map the view of the section.
  Base = 0;
  size_t FullSize = MAX_STATS_SIZE;
  Result = NtMapViewOfSection(SectionMapping, NtCurrentProcess(), &Base, 0, 0, nullptr, &FullSize, ViewUnmap, MEM_RESERVE | MEM_TOP_DOWN,
                              PAGE_READWRITE);
  if (Result != 0) {
    CloseHandle(SectionMapping);
    return;
  }

  // Once WINE supports NtExtendSection and SECTION_EXTEND_SIZE correctly then we can map/commit a single page, map the full MAX_STATS_SIZE
  // view as reserved, and extend the view using NtExtendSection.
  SaveHeader(AppType);
}
StatAlloc::~StatAlloc() {
  DeleteFile(fextl::fmt::format("/dev/shm/fex-{}-stats", linux_getpid()).c_str());
}

} // namespace FEX::Windows
