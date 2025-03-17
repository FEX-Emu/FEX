// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#pragma once

#include "LinuxSyscalls/Types.h"
#include <FEXCore/Utils/CompilerDefs.h>

#include <linux/types.h>
#include <asm/ipcbuf.h>
#include <asm/posix_types.h>
#include <asm/sembuf.h>
#include <cstdint>
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

  operator struct ipc64_perm() const {
    struct ipc64_perm perm {};
    perm.key = key;
    perm.uid = uid;
    perm.gid = gid;
    perm.cuid = cuid;
    perm.cgid = cgid;
    perm.mode = mode;
    perm.seq = seq;
    return perm;
  }

  ipc_perm_64(struct ipc64_perm perm) {
    key = perm.key;
    uid = perm.uid;
    gid = perm.gid;
    cuid = perm.cuid;
    cgid = perm.cgid;
    mode = perm.mode;
    seq = perm.seq;
    _pad1 = _pad2 = 0;
  }
};

static_assert(std::is_trivial<ipc_perm_64>::value, "Needs to be trivial");
static_assert(sizeof(ipc_perm_64) == 48, "Incorrect size");

// Matches the definition x86/include/uapi/asm/sembuf.h
struct FEX_ANNOTATE("alias-x86_64-semid64_ds") FEX_ANNOTATE("fex-match") semid_ds_64 {
  FEX::HLE::x64::ipc_perm_64 sem_perm;
  time_t sem_otime;
  uint64_t __unused1;
  time_t sem_ctime;
  uint64_t __unused2;
  uint64_t sem_nsems;
  uint64_t __unused3;
  uint64_t __unused4;

  semid_ds_64() = delete;

  operator struct semid64_ds() const {
    struct semid64_ds buf {};
    buf.sem_perm = sem_perm;

    buf.sem_otime = sem_otime;
    buf.sem_ctime = sem_ctime;
    buf.sem_nsems = sem_nsems;
    return buf;
  }

  semid_ds_64(struct semid64_ds buf)
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
  FEX::HLE::x64::semid_ds_64* buf;
  unsigned short* array;
  struct fex_seminfo* __buf;
  void* __pad;
};

static_assert(std::is_trivial<FEX::HLE::x64::semun>::value, "Needs to be trivial");
static_assert(sizeof(FEX::HLE::x64::semun) == 8, "Incorrect size");

struct FEX_ANNOTATE("fex-match") FEX_PACKED guest_stat {
  uint64_t st_dev;
  uint64_t st_ino;
  uint64_t st_nlink;

  unsigned int st_mode;
  unsigned int st_uid;
  unsigned int st_gid;
  unsigned int __pad0;
  uint64_t st_rdev;
  int64_t st_size;
  int64_t st_blksize;
  int64_t st_blocks; /* Number 512-byte blocks allocated. */

  uint64_t st_atime_;
  uint64_t fex_st_atime_nsec;
  uint64_t st_mtime_;
  uint64_t fex_st_mtime_nsec;
  uint64_t st_ctime_;
  uint64_t fex_st_ctime_nsec;
  int64_t unused[3];

  guest_stat() = delete;
  operator struct stat() const {
    struct stat val {};
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
    val.st_atim.tv_nsec = fex_st_atime_nsec;

    val.st_mtim.tv_sec = st_mtime_;
    val.st_mtim.tv_nsec = fex_st_mtime_nsec;

    val.st_ctim.tv_sec = st_ctime_;
    val.st_ctim.tv_nsec = fex_st_ctime_nsec;
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
    fex_st_atime_nsec = val.st_atim.tv_nsec;

    st_mtime_ = val.st_mtime;
    fex_st_mtime_nsec = val.st_mtim.tv_nsec;

    st_ctime_ = val.st_ctime;
    fex_st_ctime_nsec = val.st_ctim.tv_nsec;
#undef COPY
    __pad0 = 0;
  }
};

// Original definition in `arch/x86/include/uapi/asm/stat.h` for future excavation
static_assert(std::is_trivial<FEX::HLE::x64::guest_stat>::value, "Needs to be trivial");
static_assert(sizeof(FEX::HLE::x64::guest_stat) == 144, "Incorrect size");

// There is no public definition of this struct
// Matches the definition of `struct linux_dirent` in fs/readdir.c
struct FEX_ANNOTATE("fex-match") linux_dirent {
  uint64_t d_ino;
  uint64_t d_off;
  uint16_t d_reclen;
  char d_name[1];
  /* Has hidden null character and d_type */
};
static_assert(std::is_trivial<linux_dirent>::value, "Needs to be trivial");
static_assert(offsetof(linux_dirent, d_ino) == 0, "Incorrect offset");
static_assert(offsetof(linux_dirent, d_off) == 8, "Incorrect offset");
static_assert(offsetof(linux_dirent, d_reclen) == 16, "Incorrect offset");
static_assert(offsetof(linux_dirent, d_name) == 18, "Incorrect offset");
static_assert(sizeof(linux_dirent) == 24, "Incorrect size");

// There is no public definition of this struct
// Matches the definition of `struct linux_dirent64` in include/linux/dirent.h
struct FEX_ANNOTATE("fex-match") FEX_PACKED linux_dirent_64 {
  uint64_t d_ino;
  uint64_t d_off;
  uint16_t d_reclen;
  uint8_t d_type;
  char d_name[];
};
static_assert(std::is_trivial<linux_dirent_64>::value, "Needs to be trivial");
static_assert(sizeof(linux_dirent_64) == 19, "Incorrect size");
} // namespace FEX::HLE::x64
