// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
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
} // namespace FEX::Windows::UnixLib
