// SPDX-License-Identifier: MIT
#include <FEXCore/Utils/AllocatorHooks.h>
#include <FEXCore/Utils/TypeDefines.h>

#include <sysinfoapi.h>
#include <windef.h>
#include <winternl.h>
#include <wine/unixlib.h>

#include "Unixlib/FEXUnixlib.h"

#define MADV_HUGEPAGE 14
#define MADV_NOHUGEPAGE 15

namespace FEX::Windows::Allocator {

static void VirtualName(const char* Name, const void* Ptr, size_t Size) {
  static bool Supports {true};
  if (Supports) {
    FexPrctlSetVMAParams Params {
      .addr = const_cast<void*>(Ptr),
      .size = Size,
      .name = Name,
    };
    if (WINE_UNIX_CALL(fex_unix_prctl_set_vma, &Params) || Params.result != 0) {
      Supports = false;
    }
  }
}

static void VirtualTHPControl(const void* Ptr, size_t Size, FEXCore::Allocator::THPControl Control) {
  FexMadviseParams Params {
    .addr = const_cast<void*>(Ptr),
    .size = Size,
    .advise = Control == FEXCore::Allocator::THPControl::Disable ? MADV_NOHUGEPAGE : MADV_HUGEPAGE,
  };
  WINE_UNIX_CALL(fex_unix_madvise, &Params);
}

void SetupHooks(bool IsWine) {
  if (!IsWine) {
    return;
  }

  SYSTEM_INFO system_info {};
  GetSystemInfo(&system_info);
  FEXCore::Allocator::SetupHooks(system_info.dwPageSize, {
                                                           .VirtualName = VirtualName,
                                                           .VirtualTHPControl = VirtualTHPControl,
                                                         });
}
} // namespace FEX::Windows::Allocator
