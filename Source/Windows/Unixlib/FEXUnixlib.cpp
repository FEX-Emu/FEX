// SPDX-License-Identifier: MIT

#include "FEXUnixlib.h"

#include <cstdint>
#include <cerrno>
#include <string>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <unistd.h>

using NTSTATUS = int32_t;
using unixlib_entry_t = NTSTATUS (*)(void*);

#define STATUS_SUCCESS ((NTSTATUS)0x00000000)
#define STATUS_NOT_SUPPORTED ((NTSTATUS)0xC00000BB)
#define STATUS_INTERNAL_ERROR ((NTSTATUS)0xC00000E5)

#ifndef PR_GET_MEM_MODEL
#define PR_GET_MEM_MODEL 0x6d4d444c
#endif
#ifndef PR_SET_MEM_MODEL
#define PR_SET_MEM_MODEL 0x4d4d444c
#endif
#ifndef PR_SET_MEM_MODEL_DEFAULT
#define PR_SET_MEM_MODEL_DEFAULT 0
#endif
#ifndef PR_SET_MEM_MODEL_TSO
#define PR_SET_MEM_MODEL_TSO 1
#endif
#ifndef PR_ARM64_SET_UNALIGN_ATOMIC
#define PR_ARM64_SET_UNALIGN_ATOMIC 0x46455849
#endif
#ifndef PR_SET_VMA
#define PR_SET_VMA 0x53564d41
#endif
#ifndef PR_SET_VMA_ANON_NAME
#define PR_SET_VMA_ANON_NAME 0
#endif

static constexpr size_t FEX_STATS_SHM_MAX_SIZE = 128 * 1024 * 1024;
static void* stats_shm_base = nullptr;

static NTSTATUS unix_set_hardware_tso(void* args) {
  auto* params = static_cast<FexSetHardwareTSOParams*>(args);
  if (params->enable) {
    int ret = prctl(PR_GET_MEM_MODEL, 0, 0, 0, 0);
    if (ret == PR_SET_MEM_MODEL_DEFAULT) {
      return prctl(PR_SET_MEM_MODEL, PR_SET_MEM_MODEL_TSO, 0, 0, 0) ? STATUS_NOT_SUPPORTED : STATUS_SUCCESS;
    }
    return ret == PR_SET_MEM_MODEL_TSO ? STATUS_SUCCESS : STATUS_NOT_SUPPORTED;
  }

  prctl(PR_SET_MEM_MODEL, PR_SET_MEM_MODEL_DEFAULT, 0, 0, 0);
  return STATUS_SUCCESS;
}

static NTSTATUS unix_set_unalign_atomic(void* args) {
  auto* params = static_cast<FexSetUnalignAtomicParams*>(args);
  return prctl(PR_ARM64_SET_UNALIGN_ATOMIC, params->flags, 0, 0, 0) ? STATUS_NOT_SUPPORTED : STATUS_SUCCESS;
}

static NTSTATUS unix_get_stats_shm(void* args) {
  auto* params = static_cast<FexStatsSHMParams*>(args);

  auto name = "fex-" + std::to_string(getpid()) + "-stats";
  int oflag = O_RDWR;

  if (!stats_shm_base) {
    stats_shm_base = mmap(nullptr, FEX_STATS_SHM_MAX_SIZE, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (stats_shm_base == MAP_FAILED) {
      stats_shm_base = nullptr;
      return STATUS_INTERNAL_ERROR;
    }
    oflag |= O_CREAT | O_TRUNC;
  }

  int fd = shm_open(name.c_str(), oflag, S_IRWXU | S_IRWXG | S_IRWXO);
  if (fd == -1) {
    return STATUS_INTERNAL_ERROR;
  }

  if (ftruncate(fd, params->map_size)) {
    close(fd);
    return STATUS_INTERNAL_ERROR;
  }

  if (mmap(stats_shm_base, params->map_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0) == MAP_FAILED) {
    close(fd);
    return STATUS_INTERNAL_ERROR;
  }

  close(fd);

  params->shm_base = stats_shm_base;
  return STATUS_SUCCESS;
}

static NTSTATUS unix_madvise(void* args) {
  auto* params = static_cast<FexMadviseParams*>(args);
  madvise(params->addr, params->size, params->advise);
  return STATUS_SUCCESS;
}

static NTSTATUS unix_prctl_set_vma(void* args) {
  auto* params = static_cast<FexPrctlSetVMAParams*>(args);
  params->result = prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME, reinterpret_cast<unsigned long>(params->addr), params->size, params->name);
  return STATUS_SUCCESS;
}

extern "C" const unixlib_entry_t __wine_unix_call_funcs[] = {
  unix_set_hardware_tso, unix_set_unalign_atomic, unix_get_stats_shm, unix_madvise, unix_prctl_set_vma,
};

static_assert(sizeof(__wine_unix_call_funcs) / sizeof(__wine_unix_call_funcs[0]) == fex_unix_func_count);
