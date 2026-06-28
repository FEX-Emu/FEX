// SPDX-License-Identifier: MIT
#pragma once
#include <cstddef>
#include <cstdint>

enum class FEXUnixLibFunctions : uint32_t {
  SetHardwareTSOControl,
  SetKernelUnalignedAtomicControl,
  Madvise,
  SetVMAName,
  GetSHMStatsVMA,
  DeleteSHMStatsFile,
  MAX,
};

// Structures for passing arguments to unix library handlers.
// This must match between wow64, arm64ec, and Linux.
struct FEXUnixLib_SetHardwareTSOControlArgs {
  bool Enable;
};
static_assert(sizeof(FEXUnixLib_SetHardwareTSOControlArgs) == 1);

// These match the prctl flag values for PR_ARM64_SET_UNALIGN_ATOMIC
static constexpr uint64_t FEX_UNALIGN_ATOMIC_EMULATE = 1ULL << 0;
static constexpr uint64_t FEX_UNALIGN_ATOMIC_BACKPATCH = 1ULL << 1;
static constexpr uint64_t FEX_UNALIGN_ATOMIC_STRICT_SPLIT_LOCKS = 1ULL << 2;

struct FEXUnixLib_SetKernelUnalignedAtomicControl {
  uint64_t Flags;
};
static_assert(sizeof(FEXUnixLib_SetKernelUnalignedAtomicControl) == 8);

struct FEXUnixLib_Madvise {
  const void* Addr;
  size_t Size;
  int32_t Advise;
  uint32_t pad;
};
static_assert(sizeof(FEXUnixLib_Madvise) == 24);

struct FEXUnixLib_SetVMAName {
  const void* Addr;
  size_t Size;
  const char* Name;
};
static_assert(sizeof(FEXUnixLib_SetVMAName) == 24);

struct FEXUnixLib_GetSHMStatsVMA {
  void* SHMBase;
  uint32_t MapSize;
  uint32_t MaxSize;
};
static_assert(sizeof(FEXUnixLib_GetSHMStatsVMA) == 16);
