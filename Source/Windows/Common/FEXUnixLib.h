// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <FEXCore/Utils/AllocatorHooks.h>
#include <FEXCore/Utils/EnumUtils.h>
#include "../UnixLib/FEXUnixLib.h"
#include "wine/unixlib.h"

namespace FEX::Windows::UnixLib {
extern decltype(__wine_unix_call_dispatcher) UnixCallDispatcher;
extern unixlib_handle_t UnixLibHandle;

inline NTSTATUS Call(FEXUnixLibFunctions code, void* args) {
  if (!UnixCallDispatcher) {
    return STATUS_NOT_SUPPORTED;
  }

  return UnixCallDispatcher(UnixLibHandle, FEXCore::ToUnderlying(code), args);
}

bool Init(HMODULE NtDll);

/**
 * @brief Tries to enable hardware TSO if supported.
 *
 * @return true if hardware TSO is supported and enabled.
 */
bool TryEnableHardwareTSO();

/**
 * @brief Tries to enable kernel unaligned atomic handling if supported.
 *
 * @param Flags Which unaligned types to enable
 *
 * @return true if the flags were enabled
 */
bool SetKernelUnalignedAtomicControl(uint64_t Flags);

void VirtualTHPControl(const void* Ptr, size_t Size, FEXCore::Allocator::THPControl Control);
void VirtualName(const char* Name, const void* Ptr, size_t Size);

struct SHMSlotResult {
  void* SHMBase;
  uint32_t MappedSize;
};
SHMSlotResult AllocateSHMSlots(void* SHMBase, uint32_t MapSize, uint32_t MaxSize);
void DeleteSHMStatsFile();
} // namespace FEX::Windows::UnixLib
