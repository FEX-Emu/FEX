// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>

enum class FEXUnixLibFunctions : uint32_t {
  SetHardwareTSOControl,
  MAX,
};

// Structures for passing arguments to unix library handlers.
// This must match between wow64, arm64ec, and Linux.
struct FEXUnixLib_SetHardwareTSOControlArgs {
  bool Enable;
};
static_assert(sizeof(FEXUnixLib_SetHardwareTSOControlArgs) == 1);
