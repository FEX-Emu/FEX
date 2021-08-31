/*
$info$
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#pragma once

#include <cstdint>
#include <sys/sem.h>
#include <type_traits>

namespace FEX::HLE::x64 {
using kernel_old_time_t = int64_t;
using kernel_ulong_t = uint64_t;
using __time_t = time_t;

  struct ipc_perm_64 {
    uint32_t key;
    uint32_t uid;
    uint32_t gid;
    uint32_t cuid;
    uint32_t cgid;
    uint16_t mode;
    uint16_t _pad1;
    uint16_t seq;
    uint16_t _pad2;
    kernel_ulong_t _pad[2];

    ipc_perm_64() = delete;

    operator struct ipc_perm() const {
      struct ipc_perm perm;
      perm.__key = key;
      perm.uid   = uid;
      perm.gid   = gid;
      perm.cuid  = cuid;
      perm.cgid  = cgid;
      perm.mode  = mode;
      perm.__seq = seq;
      return perm;
    }

    ipc_perm_64(struct ipc_perm perm) {
      key  = perm.__key;
      uid  = perm.uid;
      gid  = perm.gid;
      cuid = perm.cuid;
      cgid = perm.cgid;
      mode = perm.mode;
      seq  = perm.__seq;
    }
  };

  static_assert(std::is_trivial<ipc_perm_64>::value, "Needs to be trivial");
  static_assert(sizeof(ipc_perm_64) == 48, "Incorrect size");

  struct semid_ds_64 {
    FEX::HLE::x64::ipc_perm_64	sem_perm;
    time_t sem_otime;
    time_t sem_otime_high;
    time_t sem_ctime;
    time_t sem_ctime_high;
    uint64_t sem_nsems;
    uint64_t _pad[2];

    semid_ds_64() = delete;

    operator struct semid_ds() const {
      struct semid_ds buf{};
      buf.sem_perm = sem_perm;

      buf.sem_otime = sem_otime;
      buf.sem_ctime = sem_ctime;
      buf.sem_nsems = sem_nsems;

#ifndef _M_ARM_64
      // AArch64 doesn't have these legacy high variables
      buf.__sem_otime_high = sem_otime_high;
      buf.__sem_ctime_high = sem_ctime_high;
#endif

      // sem_base, sem_pending, sem_pending_last, undo doesn't exist in the definition
      // Kernel doesn't return anything in them
      return buf;
    }

    semid_ds_64(struct semid_ds buf)
      : sem_perm {buf.sem_perm} {
      sem_otime = buf.sem_otime;
      sem_ctime = buf.sem_ctime;
      sem_nsems = buf.sem_nsems;

#ifndef _M_ARM_64
      // AArch64 doesn't have these legacy high variables
      sem_otime_high = buf.__sem_otime_high;
      sem_ctime_high = buf.__sem_ctime_high;
#endif
    }
  };

  static_assert(std::is_trivial<FEX::HLE::x64::semid_ds_64>::value, "Needs to be trivial");
  static_assert(sizeof(FEX::HLE::x64::semid_ds_64) == 104, "Incorrect size");

  union semun {
    int val;
    FEX::HLE::x64::semid_ds_64 *buf;
    unsigned short *array;
    struct seminfo *__buf;
    void *__pad;
  };

  static_assert(std::is_trivial<FEX::HLE::x64::semun>::value, "Needs to be trivial");
  static_assert(sizeof(FEX::HLE::x64::semun) == 8, "Incorrect size");
}
