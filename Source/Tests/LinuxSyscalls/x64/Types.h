/*
$info$
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#pragma once

#include <FEXCore/Utils/CompilerDefs.h>

#include <asm/ipcbuf.h>
#include <asm/posix_types.h>
#include <asm/sembuf.h>
#include <cstdint>
#include <sys/sem.h>
#include <sys/stat.h>
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

  // Matches the definition x86/include/uapi/asm/sembuf.h
  struct
  FEX_ANNOTATE("alias-x86_64-semid64_ds")
  FEX_ANNOTATE("fex-match")
  semid_ds_64 {
    FEX::HLE::x64::ipc_perm_64	sem_perm;
    time_t sem_otime;
    uint64_t __unused1;
    time_t sem_ctime;
    uint64_t __unused2;
    uint64_t sem_nsems;
    uint64_t __unused3;
    uint64_t __unused4;

    semid_ds_64() = delete;

    operator struct semid_ds() const {
      struct semid_ds buf{};
      buf.sem_perm = sem_perm;

      buf.sem_otime = sem_otime;
      buf.sem_ctime = sem_ctime;
      buf.sem_nsems = sem_nsems;
      return buf;
    }

    semid_ds_64(struct semid_ds buf)
      : sem_perm {buf.sem_perm} {
      sem_otime = buf.sem_otime;
      sem_ctime = buf.sem_ctime;
      sem_nsems = buf.sem_nsems;
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

  struct
  FEX_ANNOTATE("fex-match")
  FEX_PACKED
  guest_stat {
    __kernel_ulong_t  st_dev;
    __kernel_ulong_t  st_ino;
    __kernel_ulong_t  st_nlink;

    unsigned int    st_mode;
    unsigned int    st_uid;
    unsigned int    st_gid;
    unsigned int    __pad0;
    __kernel_ulong_t  st_rdev;
    __kernel_long_t   st_size;
    __kernel_long_t   st_blksize;
    __kernel_long_t   st_blocks;  /* Number 512-byte blocks allocated. */

    __kernel_ulong_t  st_atime_;
    __kernel_ulong_t  st_atime_nsec;
    __kernel_ulong_t  st_mtime_;
    __kernel_ulong_t  st_mtime_nsec;
    __kernel_ulong_t  st_ctime_;
    __kernel_ulong_t  st_ctime_nsec;
    __kernel_long_t   __unused[3];

    guest_stat() = delete;
    operator struct stat() const {
      struct stat val{};
#define COPY(x) val.x = x
    COPY(st_dev);
    COPY(st_ino);
    COPY(st_nlink);

    COPY(st_mode);
    COPY(st_uid);
    COPY(st_gid);

    COPY(st_rdev);
    COPY(st_size);
    COPY(st_blksize);
    COPY(st_blocks);

    val.st_atim.tv_sec = st_atime_;
    val.st_atim.tv_nsec = st_atime_nsec;

    val.st_mtim.tv_sec = st_mtime_;
    val.st_mtim.tv_nsec = st_mtime_nsec;

    val.st_ctim.tv_sec = st_ctime_;
    val.st_ctim.tv_nsec= st_ctime_nsec;
#undef COPY
      return val;
    }

    guest_stat(struct stat val) {
#define COPY(x) x = val.x
    COPY(st_dev);
    COPY(st_ino);
    COPY(st_nlink);

    COPY(st_mode);
    COPY(st_uid);
    COPY(st_gid);

    COPY(st_rdev);
    COPY(st_size);
    COPY(st_blksize);
    COPY(st_blocks);

    st_atime_ = val.st_atim.tv_sec;
    st_atime_nsec = val.st_atim.tv_nsec;

    st_mtime_ = val.st_mtime;
    st_mtime_nsec = val.st_mtim.tv_nsec;

    st_ctime_ = val.st_ctime;
    st_ctime_nsec = val.st_ctim.tv_nsec;
#undef COPY
    }
  };

  // Original definition in `arch/x86/include/uapi/asm/stat.h` for future excavation
  static_assert(std::is_trivial<FEX::HLE::x64::guest_stat>::value, "Needs to be trivial");
  static_assert(sizeof(FEX::HLE::x64::guest_stat) == 144, "Incorrect size");
}
