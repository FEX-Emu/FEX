// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include "wine/unixlib.h"

namespace FEX::Windows::UnixLib {
extern decltype(__wine_unix_call_dispatcher) UnixCallDispatcher;
extern unixlib_handle_t UnixLibHandle;

inline NTSTATUS Call(uint32_t code, void* args) {
  if (!UnixCallDispatcher) {
    return STATUS_NOT_SUPPORTED;
  }

  return UnixCallDispatcher(UnixLibHandle, code, args);
}

bool Init(HMODULE NtDll);
} // namespace FEX::Windows::UnixLib
