// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: Copyright (C) 2021 Alexandre Julliard

#pragma once

#include <ntstatus.h>
#include <winternl.h>

typedef UINT64 unixlib_handle_t;

namespace FEX::Windows::Unixlib {
using CallDispatcher = NTSTATUS(WINAPI*)(unixlib_handle_t, unsigned int, void*);

inline unixlib_handle_t* Handle;
inline CallDispatcher Dispatcher;

void Init(HMODULE NtDll);

inline NTSTATUS Call(unsigned int code, void* args) {
  if (!Dispatcher) {
    return STATUS_NOT_SUPPORTED;
  }
  return Dispatcher(*Handle, code, args);
}
} // namespace FEX::Windows::Unixlib

#define WINE_UNIX_CALL(code, args) FEX::Windows::Unixlib::Call((code), (args))
