// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: Copyright (C) 2021 Alexandre Julliard

#pragma once

#include <winternl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef UINT64 unixlib_handle_t;

extern NTSTATUS(WINAPI* __wine_unix_call_dispatcher)(unixlib_handle_t, unsigned int, void*);

static inline NTSTATUS __wine_unix_call(unixlib_handle_t handle, unsigned int code, void* args) {
  return __wine_unix_call_dispatcher(handle, code, args);
}

#ifdef __cplusplus
}
#endif
