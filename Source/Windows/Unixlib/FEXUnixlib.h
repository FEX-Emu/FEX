// SPDX-License-Identifier: MIT
#pragma once

#include <cstddef>
#include <cstdint>

enum FexUnixFuncs {
  fex_unix_set_hardware_tso,
  fex_unix_set_unalign_atomic,
  fex_unix_get_stats_shm,
  fex_unix_madvise,
  fex_unix_prctl_set_vma,
  fex_unix_func_count,
};

struct FexSetHardwareTSOParams {
  int enable;
};

// These match the prctl flag values for PR_ARM64_SET_UNALIGN_ATOMIC
static constexpr uint64_t FEX_UNALIGN_ATOMIC_EMULATE = 1ULL << 0;
static constexpr uint64_t FEX_UNALIGN_ATOMIC_BACKPATCH = 1ULL << 1;
static constexpr uint64_t FEX_UNALIGN_ATOMIC_STRICT_SPLIT_LOCKS = 1ULL << 2;

struct FexSetUnalignAtomicParams {
  uint64_t flags;
};

struct FexStatsSHMParams {
  void* shm_base;
  uint32_t map_size;
  uint32_t max_size;
};

struct FexMadviseParams {
  void* addr;
  size_t size;
  int advise;
};

struct FexPrctlSetVMAParams {
  void* addr;
  size_t size;
  const char* name;
  int64_t result;
};
