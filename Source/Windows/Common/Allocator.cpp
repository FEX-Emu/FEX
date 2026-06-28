// SPDX-License-Identifier: MIT
#include <FEXCore/Utils/AllocatorHooks.h>
#include <FEXCore/Utils/TypeDefines.h>
#include "Windows/Common/FEXUnixLib.h"

#include <array>
#include <chrono>
#include <libloaderapi.h>
#include <sysinfoapi.h>
#include <synchapi.h>
#include <windef.h>
#include <winternl.h>
#include <winnt.h>
#include <wine/debug.h>

namespace FEX::Windows::Allocator {
void SetupHooks(HMODULE ntdll) {
  FEXCore::Allocator::HookPtrs Ptrs {};

  Ptrs = {
    .VirtualName = UnixLib::VirtualName,
    .VirtualTHPControl = UnixLib::VirtualTHPControl,
  };

  SYSTEM_INFO system_info {};
  GetSystemInfo(&system_info);
  FEXCore::Allocator::SetupHooks(system_info.dwPageSize, Ptrs);
}
} // namespace FEX::Windows::Allocator
