// SPDX-License-Identifier: MIT
#include <cstdint>
#include <type_traits>

#include "FEXUnixLib.h"

// Equivalent to C++23's std::to_underlying.
template<typename Enum>
[[nodiscard]]
constexpr std::underlying_type_t<Enum> ToUnderlying(Enum e) noexcept {
  return static_cast<std::underlying_type_t<Enum>>(e);
}

using NTSTATUS = int32_t;
using unixlib_entry_t = NTSTATUS (*)(void*);

constexpr NTSTATUS STATUS_SUCCESS = 0;
constexpr NTSTATUS STATUS_NOT_SUPPORTED = 0xC00000BB;
constexpr NTSTATUS STATUS_INTERNAL_ERROR = 0xC00000E5;

extern "C" const unixlib_entry_t __wine_unix_call_funcs[] = {};

static_assert(sizeof(__wine_unix_call_funcs) / sizeof(__wine_unix_call_funcs[0]) == ToUnderlying(FEXUnixLibFunctions::MAX));
