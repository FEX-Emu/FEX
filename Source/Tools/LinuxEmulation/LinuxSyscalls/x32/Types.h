// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#pragma once

#include <FEXCore/Utils/CompilerDefs.h>

#include <cstddef>
#include <linux/types.h>
#include <asm/ipcbuf.h>
#include <asm/msgbuf.h>
#include <asm/sembuf.h>
#include <asm/shmbuf.h>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <linux/mqueue.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/times.h>
#include <sys/timex.h>
#include <sys/uio.h>
#include <time.h>
#include <type_traits>
#include <utime.h>

#include "LinuxSyscalls/Types.h"

namespace FEX::HLE::x32 {

// Basic types to make tracking easier
using compat_ulong_t = uint32_t;
using compat_long_t = int32_t;
using compat_uptr_t = uint32_t;
using compat_size_t = uint32_t;
using compat_off_t = uint32_t;
using compat_pid_t = int32_t;
using compat_dev_t = uint16_t;
using compat_ino_t = uint32_t;
using compat_mode_t = uint16_t;
using compat_nlink_t = uint16_t;
using compat_uid_t = uint16_t;
using compat_gid_t = uint16_t;
using compat_old_sigset_t = uint32_t;
using old_time32_t = int32_t;
using compat_clock_t = int32_t;
using fd_set32 = uint32_t;

// Can't use using with aligned attributes, clang doesn't honour it
typedef FEX_ALIGNED(4) uint64_t compat_uint64_t;
typedef FEX_ALIGNED(4) int64_t compat_int64_t;
typedef FEX_ALIGNED(4) int64_t compat_loff_t;

template<typename T>
class compat_ptr {
protected:
  static compat_ptr FromAddress(uint32_t In) {
    compat_ptr<T> ret;
    ret.Ptr = In;
    return ret;
  }

  compat_ptr() = default;

public:
  template<typename T2 = T, typename = std::enable_if<!std::is_same<T2, void>::value, T2>>
  T2& operator*() const {
    return *Interpret();
  }

  T* operator->() {
    return Interpret();
  }

  // In the case of non-void type, we can index the pointer
  template<typename T2 = T, typename = std::enable_if<!std::is_same<T2, void>::value, T2>>
  T2& operator[](size_t idx) const {
    return *reinterpret_cast<T2*>(Ptr + sizeof(T2) * idx);
  }

  // In the case of void type, we need to trivially convert
  template<typename T2 = T, typename = std::enable_if<std::is_same<T2, void>::value, T2>>
  operator T2*() const {
    return reinterpret_cast<T2*>(Ptr);
  }

  operator T*() const {
    return Interpret();
  }

  explicit operator bool() const noexcept {
    return !!Ptr;
  }

  explicit operator uintptr_t() const {
    return Ptr;
  }

  uint32_t Ptr;

private:
  T* Interpret() const {
    return reinterpret_cast<T*>(Ptr);
  }
};
static_assert(std::is_trivial<compat_ptr<void>>::value, "Needs to be trivial");
static_assert(sizeof(compat_ptr<void>) == 4, "Incorrect size");

/**
 * Helper class to import a compat_ptr from a native pointer or raw address.
 *
 * Adding these custom constructors to compat_ptr itself would trigger clang's -Wpacked-non-pod warnings.
 */
template<typename T>
class auto_compat_ptr : public compat_ptr<T> {

public:
  auto_compat_ptr(uint32_t In)
    : compat_ptr<T> {compat_ptr<T>::FromAddress(In)} {}
  auto_compat_ptr(T* In)
    : compat_ptr<T> {compat_ptr<T>::FromAddress(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(In)))} {}
};

template<typename T>
auto_compat_ptr(T*) -> auto_compat_ptr<T>;

/**
 * @name timespec32
 *
 * This is a timespec implementation that matches 32bit linux implementation
 * Provides conversation operators for the host version
 * @{ */

struct FEX_ANNOTATE("alias-x86_32-timespec") FEX_ANNOTATE("fex-match") timespec32 {
  int32_t tv_sec;
  int32_t tv_nsec;

  timespec32() = default;

  operator timespec() const {
    timespec spec {};
    spec.tv_sec = tv_sec;
    spec.tv_nsec = tv_nsec;
    return spec;
  }

  timespec32(const struct timespec& spec) {
    tv_sec = spec.tv_sec;
    tv_nsec = spec.tv_nsec;
  }
};

static_assert(std::is_trivial<timespec32>::value, "Needs to be trivial");
static_assert(sizeof(timespec32) == 8, "Incorrect size");
/**  @} */

/**
 * @name timeval32
 *
 * This is a timeval implementation that matches 32bit linux implementation
 * Provides conversation operators for the host version
 * @{ */

struct FEX_ANNOTATE("alias-x86_32-timeval") FEX_ANNOTATE("fex-match") timeval32 {
  int32_t tv_sec;
  int32_t tv_usec;

  timeval32() = delete;

  operator timeval() const {
    timeval spec {};
    spec.tv_sec = tv_sec;
    spec.tv_usec = tv_usec;
    return spec;
  }

  timeval32(const struct timeval& spec) {
    tv_sec = spec.tv_sec;
    tv_usec = spec.tv_usec;
  }
};
/**  @} */

static_assert(std::is_trivial<timeval32>::value, "Needs to be trivial");
static_assert(sizeof(timeval32) == 8, "Incorrect size");

/**
 * @name itimerval32
 *
 * This is a itimerval implementation that matches 32bit linux implementation
 * Provides conversation operators for the host version
 * @{ */

struct FEX_ANNOTATE("alias-x86_32-itimerval") FEX_ANNOTATE("fex-match") itimerval32 {
  FEX::HLE::x32::timeval32 it_interval;
  FEX::HLE::x32::timeval32 it_value;

  itimerval32() = delete;

  operator itimerval() const {
    itimerval spec {};
    spec.it_interval = it_interval;
    spec.it_value = it_value;
    return spec;
  }

  itimerval32(const struct itimerval& spec)
    : it_interval {spec.it_interval}
    , it_value {spec.it_value} {}
};
/**  @} */

static_assert(std::is_trivial<itimerval32>::value, "Needs to be trivial");
static_assert(sizeof(itimerval32) == 16, "Incorrect size");

/**
 * @name iovec32
 *
 * This is a iovec implementation that matches 32bit linux implementation
 * Provides conversation operators for the host version
 * @{ */

struct FEX_ANNOTATE("alias-x86_32-iovec") FEX_ANNOTATE("fex-match") iovec32 {
  uint32_t iov_base;
  uint32_t iov_len;

  iovec32() = delete;

  operator iovec() const {
    iovec vec {};
    vec.iov_base = reinterpret_cast<void*>(iov_base);
    vec.iov_len = iov_len;
    return vec;
  }

  iovec32(const struct iovec& vec) {
    iov_base = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(vec.iov_base));
    iov_len = vec.iov_len;
  }
};

static_assert(std::is_trivial<iovec32>::value, "Needs to be trivial");
static_assert(sizeof(iovec32) == 8, "Incorrect size");
/**  @} */

struct FEX_ANNOTATE("alias-x86_32-cmsghdr") FEX_ANNOTATE("fex-match") cmsghdr32 {
  uint32_t cmsg_len;
  int32_t cmsg_level;
  int32_t cmsg_type;
  char cmsg_data[];
};

static_assert(std::is_trivial<cmsghdr32>::value, "Needs to be trivial");
static_assert(sizeof(cmsghdr32) == 12, "Incorrect size");

struct FEX_ANNOTATE("alias-x86_32-msghdr") FEX_ANNOTATE("fex-match") msghdr32 {
  compat_ptr<void> msg_name;
  socklen_t msg_namelen;

  compat_ptr<iovec32> msg_iov;
  compat_size_t msg_iovlen;

  compat_ptr<void> msg_control;
  compat_size_t msg_controllen;
  int32_t msg_flags;
};

static_assert(std::is_trivial<msghdr32>::value, "Needs to be trivial");
static_assert(sizeof(msghdr32) == 28, "Incorrect size");

struct FEX_ANNOTATE("alias-x86_32-mmsghdr") FEX_ANNOTATE("fex-match") mmsghdr_32 {
  msghdr32 msg_hdr;
  uint32_t msg_len;
};

static_assert(std::is_trivial<mmsghdr_32>::value, "Needs to be trivial");
static_assert(sizeof(mmsghdr_32) == 32, "Incorrect size");

struct FEX_ANNOTATE("alias-x86_32-stack_t") FEX_ANNOTATE("fex-match") stack_t32 {
  compat_ptr<void> ss_sp;
  int32_t ss_flags;
  compat_size_t ss_size;

  stack_t32() = delete;

  operator stack_t() const {
    stack_t ss {};
    ss.ss_sp = ss_sp;
    ss.ss_flags = ss_flags;
    ss.ss_size = ss_size;
    return ss;
  }

  stack_t32(const stack_t& ss)
    : ss_sp {auto_compat_ptr {ss.ss_sp}} {
    ss_flags = ss.ss_flags;
    ss_size = ss.ss_size;
  }
};

static_assert(std::is_trivial<stack_t32>::value, "Needs to be trivial");
static_assert(sizeof(stack_t32) == 12, "Incorrect size");

struct
  // This does not match the glibc implementation of stat
  // Matches the definition of `struct compat_stat` in `arch/x86/include/asm/compat.h`
  FEX_ANNOTATE("fex-match") oldstat32 {
  uint16_t st_dev;
  uint16_t st_ino;
  uint16_t st_mode;
  uint16_t st_nlink;

  uint16_t st_uid;
  uint16_t st_gid;
  uint16_t st_rdev;

  uint32_t st_size;
  uint32_t st_atime_;
  uint32_t st_mtime_;
  uint32_t st_ctime_;

  oldstat32() = delete;

  oldstat32(const struct stat& host) {
#define COPY(x) x = host.x
    const uint32_t MINORBITS = 20;
    const uint32_t MINORMASK = (1U << MINORBITS) - 1;
    auto EncodeOld = [](dev_t dev) -> uint16_t {
      // This is a bit weird
      return ((dev >> MINORBITS) << 8) | (dev & MINORMASK);
    };

    st_dev = EncodeOld(host.st_dev);
    COPY(st_ino);
    COPY(st_mode);
    COPY(st_nlink);

    COPY(st_uid);
    COPY(st_gid);
    st_rdev = EncodeOld(host.st_rdev);

    COPY(st_size);

    st_atime_ = host.st_atim.tv_sec;
    st_mtime_ = host.st_mtime;
    st_ctime_ = host.st_ctime;
#undef COPY
  }
};
static_assert(std::is_trivial<oldstat32>::value, "Needs to be trivial");
static_assert(sizeof(oldstat32) == 32, "Incorrect size");

struct
  // This does not match the glibc implementation of stat
  // Matches the definition of `struct compat_stat` in `arch/x86/include/asm/compat.h`
  FEX_ANNOTATE("fex-match") stat32 {
  compat_dev_t st_dev;
  uint16_t __pad1;
  compat_ino_t st_ino;
  compat_mode_t st_mode;
  compat_nlink_t st_nlink;

  compat_uid_t st_uid;
  compat_gid_t st_gid;
  compat_dev_t st_rdev;

  uint16_t __pad2;
  uint32_t st_size;
  uint32_t st_blksize;
  uint32_t st_blocks; /* Number 512-byte blocks allocated. */
  uint32_t st_atime_;
  uint32_t fex_st_atime_nsec;
  uint32_t st_mtime_;
  uint32_t fex_st_mtime_nsec;
  uint32_t st_ctime_;
  uint32_t fex_st_ctime_nsec;
  uint32_t __unused4;
  uint32_t __unused5;

  stat32() = delete;

  stat32(const struct stat& host) {
#define COPY(x) x = host.x
    COPY(st_dev);
    COPY(st_ino);
    COPY(st_mode);
    COPY(st_nlink);

    COPY(st_uid);
    COPY(st_gid);
    COPY(st_rdev);

    COPY(st_size);
    COPY(st_blksize);
    COPY(st_blocks);

    st_atime_ = host.st_atim.tv_sec;
    fex_st_atime_nsec = host.st_atim.tv_nsec;

    st_mtime_ = host.st_mtime;
    fex_st_mtime_nsec = host.st_mtim.tv_nsec;

    st_ctime_ = host.st_ctime;
    fex_st_ctime_nsec = host.st_ctim.tv_nsec;
#undef COPY
    __pad1 = __pad2 = __unused4 = __unused5 = 0;
  }
};
static_assert(std::is_trivial<stat32>::value, "Needs to be trivial");
static_assert(sizeof(stat32) == 64, "Incorrect size");

struct
  // This does not match the glibc implementation of stat
  // Matches the definition of `struct stat64` in `x86_64-linux-gnu/asm/stat.h`
  FEX_ANNOTATE("fex-match") FEX_PACKED stat64_32 {
  compat_uint64_t st_dev;
  uint8_t __pad0[4];
  uint32_t __st_ino;

  uint32_t st_mode;
  uint32_t st_nlink;

  uint32_t st_uid;
  uint32_t st_gid;

  compat_uint64_t st_rdev;
  uint8_t __pad3[4];
  compat_int64_t st_size;
  uint32_t st_blksize;
  compat_uint64_t st_blocks; /* Number 512-byte blocks allocated. */
  uint32_t st_atime_;
  uint32_t fex_st_atime_nsec;
  uint32_t st_mtime_;
  uint32_t fex_st_mtime_nsec;
  uint32_t st_ctime_;
  uint32_t fex_st_ctime_nsec;
  compat_uint64_t st_ino;

  stat64_32() = delete;

  stat64_32(const struct stat& host) {
#define COPY(x) x = host.x
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

    __st_ino = host.st_ino;

    st_atime_ = host.st_atim.tv_sec;
    fex_st_atime_nsec = host.st_atim.tv_nsec;

    st_mtime_ = host.st_mtime;
    fex_st_mtime_nsec = host.st_mtim.tv_nsec;

    st_ctime_ = host.st_ctime;
    fex_st_ctime_nsec = host.st_ctim.tv_nsec;
#undef COPY
  }

#ifndef stat64
  stat64_32(const struct stat64& host) {
#define COPY(x) x = host.x
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

    __st_ino = host.st_ino;

    st_atime_ = host.st_atim.tv_sec;
    fex_st_atime_nsec = host.st_atim.tv_nsec;

    st_mtime_ = host.st_mtime;
    fex_st_mtime_nsec = host.st_mtim.tv_nsec;

    st_ctime_ = host.st_ctime;
    fex_st_ctime_nsec = host.st_ctim.tv_nsec;
#undef COPY
  }
#endif
};
static_assert(std::is_trivial<stat64_32>::value, "Needs to be trivial");
static_assert(sizeof(stat64_32) == 96, "Incorrect size");

struct FEX_PACKED FEX_ALIGNED(4) FEX_ANNOTATE("alias-x86_32-statfs64") FEX_ANNOTATE("fex-match") statfs64_32 {
  uint32_t f_type;
  uint32_t f_bsize;
  compat_uint64_t f_blocks;
  compat_uint64_t f_bfree;
  compat_uint64_t f_bavail;
  compat_uint64_t f_files;
  compat_uint64_t f_ffree;
  __kernel_fsid_t f_fsid;
  uint32_t f_namelen;
  uint32_t f_frsize;
  uint32_t f_flags;
  uint32_t pad[4];

  statfs64_32() = delete;

  statfs64_32(const struct statfs& host) {
#define COPY(x) x = host.x
    COPY(f_type);
    COPY(f_bsize);
    COPY(f_blocks);
    COPY(f_bfree);
    COPY(f_bavail);
    COPY(f_files);
    COPY(f_ffree);
    COPY(f_namelen);
    COPY(f_frsize);
    COPY(f_flags);

    memcpy(&f_fsid, &host.f_fsid, sizeof(f_fsid));
#undef COPY
  }

#ifndef statfs64
  statfs64_32(const struct statfs64& host) {
#define COPY(x) x = host.x
    COPY(f_type);
    COPY(f_bsize);
    COPY(f_blocks);
    COPY(f_bfree);
    COPY(f_bavail);
    COPY(f_files);
    COPY(f_ffree);
    COPY(f_namelen);
    COPY(f_frsize);
    COPY(f_flags);

    memcpy(&f_fsid, &host.f_fsid, sizeof(f_fsid));
#undef COPY
  }
#endif
};
static_assert(std::is_trivial<statfs64_32>::value, "Needs to be trivial");
static_assert(sizeof(statfs64_32) == 84, "Incorrect size");

struct FEX_ANNOTATE("alias-x86_32-statfs") FEX_ANNOTATE("fex-match") statfs32_32 {
  int32_t f_type;
  int32_t f_bsize;
  int32_t f_blocks;
  int32_t f_bfree;
  int32_t f_bavail;
  int32_t f_files;
  int32_t f_ffree;
  __kernel_fsid_t f_fsid;
  int32_t f_namelen;
  int32_t f_frsize;
  int32_t f_flags;
  int32_t pad[4];

  statfs32_32() = delete;

  statfs32_32(const struct statfs& host) {
#define COPY(x) x = host.x
    COPY(f_type);
    COPY(f_bsize);
    COPY(f_blocks);
    COPY(f_bfree);
    COPY(f_bavail);
    COPY(f_files);
    COPY(f_ffree);
    COPY(f_namelen);
    COPY(f_frsize);
    COPY(f_flags);

    memcpy(&f_fsid, &host.f_fsid, sizeof(f_fsid));
#undef COPY
  }

#ifndef statfs64
  statfs32_32(struct statfs64 host) {
#define COPY(x) x = host.x
    COPY(f_type);
    COPY(f_bsize);
    COPY(f_blocks);
    COPY(f_bfree);
    COPY(f_bavail);
    COPY(f_files);
    COPY(f_ffree);
    COPY(f_namelen);
    COPY(f_frsize);
    COPY(f_flags);

    memcpy(&f_fsid, &host.f_fsid, sizeof(f_fsid));
#undef COPY
  }
#endif
};
static_assert(std::is_trivial<statfs32_32>::value, "Needs to be trivial");
static_assert(sizeof(statfs32_32) == 64, "Incorrect size");

struct FEX_ANNOTATE("alias-x86_32-flock") FEX_ANNOTATE("fex-match") flock_32 {
  int16_t l_type;
  int16_t l_whence;
  int32_t l_start;
  int32_t l_len;
  int32_t l_pid;

  flock_32() = delete;

  flock_32(const struct flock& host) {
    l_type = host.l_type;
    l_whence = host.l_whence;
    l_start = host.l_start;
    l_len = host.l_len;
    l_pid = host.l_pid;
  }

  operator struct flock() const {
    struct flock res {};
    res.l_type = l_type;
    res.l_whence = l_whence;
    res.l_start = l_start;
    res.l_len = l_len;
    res.l_pid = l_pid;
    return res;
  }
};

static_assert(std::is_trivial<flock_32>::value, "Needs to be trivial");
static_assert(sizeof(flock_32) == 16, "Incorrect size");

// glibc doesn't pack flock64 while the kernel does
// This does not match glibc flock64 definition
// Matches the definition of `struct compat_flock64` in `arch/x86/include/asm/compat.h`
struct FEX_ANNOTATE("fex-match") FEX_PACKED flock64_32 {
  int16_t l_type;
  int16_t l_whence;
  compat_loff_t l_start;
  compat_loff_t l_len;
  compat_pid_t l_pid;

  flock64_32() = delete;

  flock64_32(const struct flock& host) {
    l_type = host.l_type;
    l_whence = host.l_whence;
    l_start = host.l_start;
    l_len = host.l_len;
    l_pid = host.l_pid;
  }

  operator struct flock() const {
    struct flock res {};
    res.l_type = l_type;
    res.l_whence = l_whence;
    res.l_start = l_start;
    res.l_len = l_len;
    res.l_pid = l_pid;
    return res;
  }
};
static_assert(std::is_trivial<flock64_32>::value, "Needs to be trivial");
static_assert(sizeof(flock64_32) == 24, "Incorrect size");

// There is no public definition of this struct
// Matches the definition of `struct linux_dirent` in fs/readdir.c
struct FEX_ANNOTATE("fex-match") linux_dirent {
  compat_uint64_t d_ino;
  compat_int64_t d_off;
  uint16_t d_reclen;
  uint8_t _pad[6];
  char d_name[];
};
static_assert(std::is_trivial<linux_dirent>::value, "Needs to be trivial");
static_assert(sizeof(linux_dirent) == 24, "Incorrect size");

// There is no public definition of this struct
// Matches the definition of `struct compat_linux_dirent` in fs/readdir.c
struct FEX_ANNOTATE("fex-match") linux_dirent_32 {
  compat_ulong_t d_ino;
  compat_ulong_t d_off;
  uint16_t d_reclen;
  char d_name[1];
  /* Has hidden null character and d_type */
};
static_assert(std::is_trivial<linux_dirent_32>::value, "Needs to be trivial");
static_assert(sizeof(linux_dirent_32) == 12, "Incorrect size");

// There is no public definition of this struct
// Matches the definition of `struct linux_dirent64` in include/linux/dirent.h
struct FEX_ANNOTATE("fex-match") linux_dirent_64 {
  compat_uint64_t d_ino;
  compat_uint64_t d_off;
  uint16_t d_reclen;
  uint8_t d_type;
  uint8_t _pad[5];
  char d_name[];
};
static_assert(std::is_trivial<linux_dirent_64>::value, "Needs to be trivial");
static_assert(sizeof(linux_dirent_64) == 24, "Incorrect size");

// There is no public definition of this struct
// Matches `struct compat_sigset_argpack`
struct FEX_ANNOTATE("fex-match") sigset_argpack32 {
  compat_ptr<uint64_t> sigset;
  compat_size_t size;
};

static_assert(std::is_trivial<sigset_argpack32>::value, "Needs to be trivial");
static_assert(sizeof(sigset_argpack32) == 8, "Incorrect size");

struct FEX_ANNOTATE("alias-x86_32-rusage") FEX_ANNOTATE("fex-match") rusage_32 {
  timeval32 ru_utime;
  timeval32 ru_stime;
  union {
    compat_long_t ru_maxrss;
    compat_long_t __ru_maxrss_word;
  };
  union {
    compat_long_t ru_ixrss;
    compat_long_t __ru_ixrss_word;
  };
  union {
    compat_long_t ru_idrss;
    compat_long_t __ru_idrss_word;
  };
  union {
    compat_long_t ru_isrss;
    compat_long_t __ru_isrss_word;
  };
  union {
    compat_long_t ru_minflt;
    compat_long_t __ru_minflt_word;
  };
  union {
    compat_long_t ru_majflt;
    compat_long_t __ru_majflt_word;
  };
  union {
    compat_long_t ru_nswap;
    compat_long_t __ru_nswap_word;
  };
  union {
    compat_long_t ru_inblock;
    compat_long_t __ru_inblock_word;
  };
  union {
    compat_long_t ru_oublock;
    compat_long_t __ru_oublock_word;
  };
  union {
    compat_long_t ru_msgsnd;
    compat_long_t __ru_msgsnd_word;
  };
  union {
    compat_long_t ru_msgrcv;
    compat_long_t __ru_msgrcv_word;
  };
  union {
    compat_long_t ru_nsignals;
    compat_long_t __ru_nsignals_word;
  };
  union {
    compat_long_t ru_nvcsw;
    compat_long_t __ru_nvcsw_word;
  };
  union {
    compat_long_t ru_nivcsw;
    compat_long_t __ru_nivcsw_word;
  };

  rusage_32() = delete;
  rusage_32(const struct rusage& usage)
    : ru_utime {usage.ru_utime}
    , ru_stime {usage.ru_stime} {
    // These only truncate
    ru_maxrss = usage.ru_maxrss;
    ru_ixrss = usage.ru_ixrss;
    ru_idrss = usage.ru_idrss;
    ru_isrss = usage.ru_isrss;
    ru_minflt = usage.ru_minflt;
    ru_majflt = usage.ru_majflt;
    ru_nswap = usage.ru_nswap;
    ru_inblock = usage.ru_inblock;
    ru_oublock = usage.ru_oublock;
    ru_msgsnd = usage.ru_msgsnd;
    ru_msgrcv = usage.ru_msgrcv;
    ru_nsignals = usage.ru_nsignals;
    ru_nvcsw = usage.ru_nvcsw;
    ru_nivcsw = usage.ru_nivcsw;
  }

  operator struct rusage() const {
    struct rusage usage {};
    usage.ru_utime = ru_utime;
    usage.ru_stime = ru_stime;
    usage.ru_maxrss = ru_maxrss;
    usage.ru_ixrss = ru_ixrss;
    usage.ru_idrss = ru_idrss;
    usage.ru_isrss = ru_isrss;
    usage.ru_minflt = ru_minflt;
    usage.ru_majflt = ru_majflt;
    usage.ru_nswap = ru_nswap;
    usage.ru_inblock = ru_inblock;
    usage.ru_oublock = ru_oublock;
    usage.ru_msgsnd = ru_msgsnd;
    usage.ru_msgrcv = ru_msgrcv;
    usage.ru_nsignals = ru_nsignals;
    usage.ru_nvcsw = ru_nvcsw;
    usage.ru_nivcsw = ru_nivcsw;

    return usage;
  }
};
static_assert(std::is_trivial<rusage_32>::value, "Needs to be trivial");
static_assert(sizeof(rusage_32) == 72, "Incorrect size");

struct FEX_PACKED FEX_ANNOTATE("fex-match") OldGuestSigAction_32 {
  FEX::HLE::x32::compat_ptr<void> handler_32;
  uint32_t sa_mask;
  uint32_t sa_flags;
  FEX::HLE::x32::compat_ptr<void> restorer_32;

  OldGuestSigAction_32() = delete;

  operator FEX::HLE::GuestSigAction() const {
    FEX::HLE::GuestSigAction action {};

    action.sigaction_handler.handler = reinterpret_cast<decltype(action.sigaction_handler.handler)>(handler_32.Ptr);
    action.sa_flags = sa_flags;
    action.restorer = reinterpret_cast<decltype(action.restorer)>(restorer_32.Ptr);
    action.sa_mask.Val = sa_mask;
    return action;
  }

  OldGuestSigAction_32(const FEX::HLE::GuestSigAction& action)
    : handler_32 {auto_compat_ptr {reinterpret_cast<void*>(action.sigaction_handler.handler)}}
    , restorer_32 {auto_compat_ptr {reinterpret_cast<void*>(action.restorer)}} {
    sa_flags = action.sa_flags;
    sa_mask = action.sa_mask.Val;
  }
};

static_assert(std::is_trivial<OldGuestSigAction_32>::value, "Needs to be trivial");
static_assert(sizeof(OldGuestSigAction_32) == 16, "Incorrect size");

// This definition isn't public
// This is for rt_sigaction
// Matches the definition for `struct compat_sigaction` in `include/linux/compat.h`
struct FEX_PACKED FEX_ANNOTATE("fex-match") GuestSigAction_32 {
  FEX::HLE::x32::compat_ptr<void> handler_32;

  uint32_t sa_flags;
  FEX::HLE::x32::compat_ptr<void> restorer_32;
  FEX::HLE::GuestSAMask sa_mask;

  GuestSigAction_32() = delete;

  operator FEX::HLE::GuestSigAction() const {
    FEX::HLE::GuestSigAction action {};

    action.sigaction_handler.handler = reinterpret_cast<decltype(action.sigaction_handler.handler)>(handler_32.Ptr);
    action.sa_flags = sa_flags;
    action.restorer = reinterpret_cast<decltype(action.restorer)>(restorer_32.Ptr);
    action.sa_mask = sa_mask;
    return action;
  }

  GuestSigAction_32(const FEX::HLE::GuestSigAction& action)
    : handler_32 {auto_compat_ptr {reinterpret_cast<void*>(action.sigaction_handler.handler)}}
    , restorer_32 {auto_compat_ptr {reinterpret_cast<void*>(action.restorer)}} {
    sa_flags = action.sa_flags;
    sa_mask = action.sa_mask;
  }
};

static_assert(std::is_trivial<GuestSigAction_32>::value, "Needs to be trivial");
static_assert(sizeof(GuestSigAction_32) == 20, "Incorrect size");

struct FEX_ANNOTATE("alias-x86_32-tms") FEX_ANNOTATE("fex-match") compat_tms {
  compat_clock_t tms_utime;
  compat_clock_t tms_stime;
  compat_clock_t tms_cutime;
  compat_clock_t tms_cstime;

  compat_tms() = delete;
  operator tms() const {
    tms val {};
    val.tms_utime = tms_utime;
    val.tms_stime = tms_stime;
    val.tms_cutime = tms_cutime;
    val.tms_cstime = tms_cstime;
    return val;
  }
  compat_tms(const struct tms& val) {
    tms_utime = val.tms_utime;
    tms_stime = val.tms_stime;
    tms_cutime = val.tms_cutime;
    tms_cstime = val.tms_cstime;
  }
};

static_assert(std::is_trivial<compat_tms>::value, "Needs to be trivial");
static_assert(sizeof(compat_tms) == 16, "Incorrect size");

struct FEX_ANNOTATE("alias-x86_32-utimbuf") FEX_ANNOTATE("fex-match") old_utimbuf32 {
  old_time32_t actime;
  old_time32_t modtime;

  old_utimbuf32() = delete;
  operator utimbuf() const {
    utimbuf val {};
    val.actime = actime;
    val.modtime = modtime;
    return val;
  }

  old_utimbuf32(const struct utimbuf& val) {
    actime = val.actime;
    modtime = val.modtime;
  }
};

static_assert(std::is_trivial<old_utimbuf32>::value, "Needs to be trivial");
static_assert(sizeof(old_utimbuf32) == 8, "Incorrect size");

struct FEX_ANNOTATE("alias-x86_32-itimerspec") FEX_ANNOTATE("fex-match") old_itimerspec32 {
  timespec32 it_interval;
  timespec32 it_value;

  old_itimerspec32() = delete;
  operator itimerspec() const {
    itimerspec val {};
    val.it_interval = it_interval;
    val.it_value = it_value;
    return val;
  }

  old_itimerspec32(const struct itimerspec& val)
    : it_interval {val.it_interval}
    , it_value {val.it_value} {}
};

static_assert(std::is_trivial<old_itimerspec32>::value, "Needs to be trivial");
static_assert(sizeof(old_itimerspec32) == 16, "Incorrect size");

template<bool Signed>
struct FEX_ANNOTATE("alias-x86_32-rlimit") FEX_ANNOTATE("fex-match") rlimit32 {
  uint32_t rlim_cur;
  uint32_t rlim_max;
  rlimit32() = delete;

  operator rlimit() const {
    static_assert(Signed == false, "Signed variant doesn't exist");
    rlimit val {};

    val.rlim_cur = rlim_cur;
    val.rlim_max = rlim_max;

    if (val.rlim_cur == ~0U) {
      val.rlim_cur = ~0UL;
    }
    if (val.rlim_max == ~0U) {
      val.rlim_max = ~0UL;
    }

    return val;
  }

  rlimit32(const struct rlimit& val) {
    constexpr uint32_t Limit = Signed ? 0x7FFF'FFFF : 0xFFFF'FFFF;
    if (val.rlim_cur > Limit) {
      rlim_cur = Limit;
    } else {
      rlim_cur = val.rlim_cur;
    }

    if (val.rlim_max > Limit) {
      rlim_max = Limit;
    } else {
      rlim_max = val.rlim_max;
    }
  }
};

static_assert(std::is_trivial<rlimit32<true>>::value, "Needs to be trivial");
static_assert(sizeof(rlimit32<true>) == 8, "Incorrect size");

struct FEX_ANNOTATE("alias-x86_32-timex") FEX_ANNOTATE("fex-match") timex32 {
  uint32_t modes;
  compat_long_t offset;
  compat_long_t freq;
  compat_long_t maxerror;
  compat_long_t esterror;
  int32_t status;
  compat_long_t constant;
  compat_long_t precision;
  compat_long_t tolerance;
  timeval32 time;
  compat_long_t tick;
  compat_long_t ppsfreq;
  compat_long_t jitter;
  int32_t shift;
  compat_long_t stabil;
  compat_long_t jitcnt;
  compat_long_t calcnt;
  compat_long_t errcnt;
  compat_long_t stbcnt;

  int32_t tai;

  // Padding
  int32_t : 32;
  int32_t : 32;
  int32_t : 32;
  int32_t : 32;
  int32_t : 32;
  int32_t : 32;
  int32_t : 32;
  int32_t : 32;
  int32_t : 32;
  int32_t : 32;
  int32_t : 32;

  timex32() = delete;

  operator timex() const {
    timex val {};
    val.modes = modes;
    val.offset = offset;
    val.freq = freq;
    val.maxerror = maxerror;
    val.esterror = esterror;
    val.status = status;
    val.constant = constant;
    val.precision = precision;
    val.tolerance = tolerance;
    val.time = time;
    val.tick = tick;
    val.ppsfreq = ppsfreq;
    val.jitter = jitter;
    val.shift = shift;
    val.stabil = stabil;
    val.jitcnt = jitcnt;
    val.calcnt = calcnt;
    val.errcnt = errcnt;
    val.stbcnt = stbcnt;
    val.tai = tai;
    return val;
  }

  timex32(const struct timex& val)
    : time {val.time} {
    modes = val.modes;
    offset = val.offset;
    freq = val.freq;
    maxerror = val.maxerror;
    esterror = val.esterror;
    status = val.status;
    constant = val.constant;
    precision = val.precision;
    tolerance = val.tolerance;
    tick = val.tick;
    ppsfreq = val.ppsfreq;
    jitter = val.jitter;
    shift = val.shift;
    stabil = val.stabil;
    jitcnt = val.jitcnt;
    calcnt = val.calcnt;
    errcnt = val.errcnt;
    stbcnt = val.stbcnt;
    tai = val.tai;
  }
};

static_assert(std::is_trivial<timex32>::value, "Needs to be trivial");
static_assert(sizeof(timex32) == 128, "Incorrect size");

union FEX_ANNOTATE("alias-x86_32-sigval") FEX_ANNOTATE("fex-match") sigval32 {
  int sival_int;
  compat_ptr<void> sival_ptr;

  sigval32() = delete;

  operator sigval() const {
    sigval val {};
    val.sival_ptr = sival_ptr;
    return val;
  }

  sigval32(sigval val) {
    sival_ptr = auto_compat_ptr {val.sival_ptr};
  }
};

static_assert(std::is_trivial<sigval32>::value, "Needs to be trivial");
static_assert(sizeof(sigval32) == 4, "Incorrect size");

constexpr size_t FEX_SIGEV_MAX_SIZE = 64;
constexpr size_t FEX_SIGEV_PAD_SIZE = (FEX_SIGEV_MAX_SIZE - (sizeof(int32_t) * 2 + sizeof(sigval32))) / sizeof(int32_t);

struct FEX_ANNOTATE("fex-match") sigevent32 {
  FEX::HLE::x32::sigval32 sigev_value;
  int sigev_signo;
  int sigev_notify;
  union {
    int _pad[FEX_SIGEV_PAD_SIZE];
    int _tid;
    struct {
      uint32_t _function;
      uint32_t _attribute;
    } _sigev_thread;
  } _sigev_un;

  sigevent32() = delete;

// For older build environments
#ifndef sigev_notify_thread_id
#define sigev_notify_thread_id _sigev_un._tid
#endif

  operator sigevent() const {
    sigevent val {};
    val.sigev_value = sigev_value;
    val.sigev_signo = sigev_signo;
    val.sigev_notify = sigev_notify;

    if (sigev_notify == SIGEV_THREAD_ID) {
      val.sigev_notify_thread_id = _sigev_un._tid;
    } else if (sigev_notify == SIGEV_THREAD) {
      val.sigev_notify_function = reinterpret_cast<void (*)(sigval)>(_sigev_un._sigev_thread._function);
      val.sigev_notify_attributes = reinterpret_cast<pthread_attr_t*>(_sigev_un._sigev_thread._attribute);
    }
    return val;
  }

  sigevent32(const sigevent& val)
    : sigev_value {val.sigev_value} {
    sigev_signo = val.sigev_signo;
    sigev_notify = val.sigev_notify;

    if (sigev_notify == SIGEV_THREAD_ID) {
      _sigev_un._tid = val.sigev_notify_thread_id;
    } else if (sigev_notify == SIGEV_THREAD) {
      _sigev_un._sigev_thread._function = static_cast<uint32_t>(reinterpret_cast<uint64_t>(val.sigev_notify_function));
      _sigev_un._sigev_thread._attribute = static_cast<uint32_t>(reinterpret_cast<uint64_t>(val.sigev_notify_attributes));
    }
  }
};

static_assert(std::is_trivial<sigval32>::value, "Needs to be trivial");
static_assert(sizeof(sigval32) == 4, "Incorrect size");

struct FEX_ANNOTATE("alias-x86_32-mq_attr") FEX_ANNOTATE("fex-match") mq_attr32 {
  compat_long_t mq_flags;
  compat_long_t mq_maxmsg;
  compat_long_t mq_msgsize;
  compat_long_t mq_curmsgs;
  compat_long_t __pad[4];
  mq_attr32() = delete;

  operator mq_attr() const {
    struct mq_attr val {};
    val.mq_flags = mq_flags;
    val.mq_maxmsg = mq_maxmsg;
    val.mq_msgsize = mq_msgsize;
    val.mq_curmsgs = mq_curmsgs;
    return val;
  }

  mq_attr32(const struct mq_attr& val) {
    mq_flags = val.mq_flags;
    mq_maxmsg = val.mq_maxmsg;
    mq_msgsize = val.mq_msgsize;
    mq_curmsgs = val.mq_curmsgs;
  }
};

static_assert(std::is_trivial<mq_attr32>::value, "Needs to be trivial");
static_assert(sizeof(mq_attr32) == 32, "Incorrect size");

union FEX_ANNOTATE("alias-x86_32-epoll_data_t") FEX_ANNOTATE("fex-match") epoll_data32 {
  compat_ptr<void> ptr;
  int fd;
  uint32_t u32;
  compat_uint64_t u64;
};

struct FEX_PACKED FEX_ANNOTATE("alias-x86_32-epoll_event") FEX_ANNOTATE("fex-match") epoll_event32 {
  uint32_t events;
  epoll_data32 data;

  epoll_event32() = delete;

  operator struct epoll_event() const {
    epoll_event event {};
    event.events = events;
    event.data.u64 = data.u64;
    return event;
  }

  epoll_event32(const struct epoll_event& event)
    : data {auto_compat_ptr<void> {static_cast<uint32_t>(event.data.u64)}} {
    events = event.events;
  }
};
static_assert(std::is_trivial<epoll_event32>::value, "Needs to be trivial");
static_assert(sizeof(epoll_event32) == 12, "Incorrect size");

struct ipc_perm_32 {
  uint32_t key;
  uint16_t uid;
  uint16_t gid;
  uint16_t cuid;
  uint16_t cgid;
  uint16_t mode;
  uint16_t seq;

  ipc_perm_32() = delete;

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

  ipc_perm_32(const struct ipc64_perm& perm) {
    key = perm.key;
    uid = perm.uid;
    gid = perm.gid;
    cuid = perm.cuid;
    cgid = perm.cgid;
    mode = perm.mode;
    seq = perm.seq;
  }
};

static_assert(std::is_trivial<ipc_perm_32>::value, "Needs to be trivial");
static_assert(sizeof(ipc_perm_32) == 16, "Incorrect size");

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
  compat_ulong_t _pad[2];

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

  ipc_perm_64(const struct ipc64_perm& perm) {
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
static_assert(sizeof(ipc_perm_64) == 36, "Incorrect size");

struct shmid_ds_32 {
  ipc_perm_32 shm_perm;
  int32_t shm_segsz;
  int32_t shm_atime;
  int32_t shm_dtime;
  int32_t shm_ctime;
  uint16_t shm_cpid;
  uint16_t shm_lpid;
  uint16_t shm_nattch;
  uint16_t shm_unused;
  uint32_t shm_unused2;
  uint32_t shm_unused3;

  shmid_ds_32() = delete;

  operator struct shmid64_ds() const {
    struct shmid64_ds buf {};
    buf.shm_perm = shm_perm;

    buf.shm_segsz = shm_segsz;
    buf.shm_atime = shm_atime;
    buf.shm_dtime = shm_dtime;
    buf.shm_ctime = shm_ctime;
    buf.shm_cpid = shm_cpid;
    buf.shm_lpid = shm_lpid;
    buf.shm_nattch = shm_nattch;
    return buf;
  }

  shmid_ds_32(const struct shmid64_ds& buf)
    : shm_perm {buf.shm_perm} {
    shm_segsz = buf.shm_segsz;
    shm_atime = buf.shm_atime;
    shm_dtime = buf.shm_dtime;
    shm_ctime = buf.shm_ctime;
    shm_cpid = buf.shm_cpid;
    shm_lpid = buf.shm_lpid;
    shm_nattch = buf.shm_nattch;
    shm_unused = 0;
    shm_unused2 = 0;
    shm_unused3 = 0;
  }
};

static_assert(std::is_trivial<shmid_ds_32>::value, "Needs to be trivial");
static_assert(sizeof(shmid_ds_32) == 48, "Incorrect size");

struct shmid_ds_64 {
  ipc_perm_64 shm_perm;
  compat_size_t shm_segsz;
  compat_ulong_t shm_atime;
  compat_ulong_t shm_atime_high;
  compat_ulong_t shm_dtime;
  compat_ulong_t shm_dtime_high;
  compat_ulong_t shm_ctime;
  compat_ulong_t shm_ctime_high;
  int32_t shm_cpid;
  int32_t shm_lpid;
  compat_ulong_t shm_nattch;
  compat_ulong_t shm_unused4;
  compat_ulong_t shm_unused5;

  shmid_ds_64() = delete;

  operator struct shmid64_ds() const {
    struct shmid64_ds buf {};
    buf.shm_perm = shm_perm;

    buf.shm_segsz = shm_segsz;
    buf.shm_atime = shm_atime_high;
    buf.shm_atime <<= 32;
    buf.shm_atime |= shm_atime;

    buf.shm_dtime = shm_dtime_high;
    buf.shm_dtime <<= 32;
    buf.shm_dtime |= shm_dtime;

    buf.shm_ctime = shm_ctime_high;
    buf.shm_ctime <<= 32;
    buf.shm_ctime |= shm_ctime;

    buf.shm_cpid = shm_cpid;
    buf.shm_lpid = shm_lpid;
    buf.shm_nattch = shm_nattch;
    return buf;
  }

  shmid_ds_64(const struct shmid64_ds& buf)
    : shm_perm {buf.shm_perm} {
    shm_segsz = buf.shm_segsz;
    shm_atime = buf.shm_atime;
    shm_atime_high = buf.shm_atime >> 32;
    shm_dtime = buf.shm_dtime;
    shm_dtime_high = buf.shm_dtime >> 32;
    shm_ctime = buf.shm_ctime;
    shm_ctime_high = buf.shm_ctime >> 32;
    shm_cpid = buf.shm_cpid;
    shm_lpid = buf.shm_lpid;
    shm_nattch = buf.shm_nattch;
    shm_unused4 = shm_unused5 = 0;
  }
};

static_assert(std::is_trivial<shmid_ds_64>::value, "Needs to be trivial");
static_assert(sizeof(shmid_ds_64) == 84, "Incorrect size");

struct semid_ds_32 {
  struct ipc_perm_32 sem_perm;
  int32_t sem_otime;
  int32_t sem_ctime;
  uint32_t sem_base;
  uint32_t sem_pending;
  uint32_t sem_pending_last;
  uint32_t undo;
  uint16_t sem_nsems;
  uint16_t _pad;

  semid_ds_32() = delete;

  operator struct semid64_ds() const {
    struct semid64_ds buf {};
    buf.sem_perm = sem_perm;

    buf.sem_otime = sem_otime;
    buf.sem_ctime = sem_ctime;
    buf.sem_nsems = sem_nsems;

    // sem_base, sem_pending, sem_pending_last, undo doesn't exist in the definition
    // Kernel doesn't return anything in them
    return buf;
  }

  semid_ds_32(const struct semid64_ds& buf)
    : sem_perm {buf.sem_perm} {
    sem_otime = buf.sem_otime;
    sem_ctime = buf.sem_ctime;
    sem_nsems = buf.sem_nsems;
    sem_base = sem_pending = sem_pending_last = undo = _pad = 0;
  }
};

static_assert(std::is_trivial<semid_ds_32>::value, "Needs to be trivial");
static_assert(sizeof(semid_ds_32) == 44, "Incorrect size");

struct semid_ds_64 {
  struct ipc_perm_64 sem_perm;
  uint32_t sem_otime;
  uint32_t sem_otime_high;
  uint32_t sem_ctime;
  uint32_t sem_ctime_high;
  uint32_t sem_nsems;
  uint32_t _pad[2];

  semid_ds_64() = delete;

  operator struct semid64_ds() const {
    struct semid64_ds buf {};
    buf.sem_perm = sem_perm;

    buf.sem_otime = sem_otime_high;
    buf.sem_otime <<= 32;
    buf.sem_otime |= sem_otime;
    buf.sem_ctime = sem_ctime_high;
    buf.sem_ctime <<= 32;
    buf.sem_ctime |= sem_ctime;
    buf.sem_nsems = sem_nsems;

    // sem_base, sem_pending, sem_pending_last, undo doesn't exist in the definition
    // Kernel doesn't return anything in them
    return buf;
  }

  semid_ds_64(const struct semid64_ds& buf)
    : sem_perm {buf.sem_perm} {
    sem_otime = buf.sem_otime;
    sem_otime_high = buf.sem_otime >> 32;
    sem_ctime = buf.sem_ctime;
    sem_ctime_high = buf.sem_ctime >> 32;
    sem_nsems = buf.sem_nsems;
  }
};

static_assert(std::is_trivial<semid_ds_64>::value, "Needs to be trivial");
static_assert(sizeof(semid_ds_64) == 64, "Incorrect size");

struct msqid_ds_32 {
  struct ipc_perm_32 msg_perm;
  compat_uptr_t msg_first;
  compat_uptr_t msg_last;
  uint32_t msg_stime;
  uint32_t msg_rtime;
  uint32_t msg_ctime;
  uint32_t msg_lcbytes;
  uint32_t msg_lqbytes;
  uint16_t msg_cbytes;
  uint16_t msg_qnum;
  uint16_t msg_qbytes;
  uint16_t msg_lspid;
  uint16_t msg_lrpid;

  msqid_ds_32() = delete;
  operator struct msqid64_ds() const {
    struct msqid64_ds val {};
    // msg_first and msg_last are unused and untouched
    val.msg_perm = msg_perm;
    val.msg_stime = msg_stime;
    val.msg_rtime = msg_rtime;
    val.msg_ctime = msg_ctime;

    val.msg_cbytes = msg_cbytes;
    val.msg_qnum = msg_qnum;
    val.msg_qbytes = msg_qbytes;
    val.msg_lspid = msg_lspid;
    val.msg_lrpid = msg_lrpid;
    return val;
  }

  msqid_ds_32(const struct msqid64_ds& buf)
    : msg_perm {buf.msg_perm} {
    // msg_first and msg_last are unused and untouched
    msg_stime = buf.msg_stime;
    msg_rtime = buf.msg_rtime;
    msg_ctime = buf.msg_ctime;
    if (buf.msg_cbytes > std::numeric_limits<uint16_t>::max()) {
      msg_cbytes = std::numeric_limits<uint16_t>::max();
    } else {
      msg_cbytes = buf.msg_cbytes;
    }
    msg_lcbytes = buf.msg_cbytes;

    if (buf.msg_qnum > std::numeric_limits<uint16_t>::max()) {
      msg_qnum = std::numeric_limits<uint16_t>::max();
    } else {
      msg_qnum = buf.msg_qnum;
    }

    if (buf.msg_cbytes > std::numeric_limits<uint16_t>::max()) {
      msg_cbytes = std::numeric_limits<uint16_t>::max();
    } else {
      msg_cbytes = buf.msg_cbytes;
    }
    msg_lqbytes = buf.msg_qbytes;
    msg_lspid = buf.msg_lspid;
    msg_lrpid = buf.msg_lrpid;
    msg_first = msg_last = msg_qbytes = 0;
  }
};
static_assert(std::is_trivial<msqid_ds_32>::value, "Needs to be trivial");
static_assert(sizeof(msqid_ds_32) == 56, "Incorrect size");

struct msqid_ds_64 {
  struct ipc_perm_64 msg_perm;
  uint32_t msg_stime;
  uint32_t msg_stime_high;
  uint32_t msg_rtime;
  uint32_t msg_rtime_high;
  uint32_t msg_ctime;
  uint32_t msg_ctime_high;
  uint32_t msg_cbytes;
  uint32_t msg_qnum;
  uint32_t msg_qbytes;
  uint32_t msg_lspid;
  uint32_t msg_lrpid;
  uint32_t _pad[2];

  msqid_ds_64() = delete;
  operator struct msqid64_ds() const {
    struct msqid64_ds val {};
    val.msg_perm = msg_perm;
    val.msg_stime = msg_stime_high;
    val.msg_stime <<= 32;
    val.msg_stime |= msg_stime;

    val.msg_rtime = msg_rtime_high;
    val.msg_rtime <<= 32;
    val.msg_rtime |= msg_rtime;

    val.msg_ctime = msg_ctime_high;
    val.msg_ctime <<= 32;
    val.msg_ctime |= msg_ctime;

    val.msg_cbytes = msg_cbytes;
    val.msg_qnum = msg_qnum;
    val.msg_qbytes = msg_qbytes;
    val.msg_lspid = msg_lspid;
    val.msg_lrpid = msg_lrpid;
    return val;
  }

  msqid_ds_64(const struct msqid64_ds& buf)
    : msg_perm {buf.msg_perm} {
    msg_stime = buf.msg_stime;
    msg_stime_high = buf.msg_stime >> 32;
    msg_rtime = buf.msg_rtime;
    msg_rtime_high = buf.msg_rtime >> 32;
    msg_ctime = buf.msg_ctime;
    msg_ctime_high = buf.msg_ctime >> 32;
    msg_cbytes = buf.msg_cbytes;
    msg_qnum = buf.msg_qnum;
    msg_qbytes = buf.msg_qbytes;
    msg_lspid = buf.msg_lspid;
    msg_lrpid = buf.msg_lrpid;
  }
};

static_assert(std::is_trivial<msqid_ds_64>::value, "Needs to be trivial");
static_assert(sizeof(msqid_ds_64) == 88, "Incorrect size");

struct FEX_ANNOTATE("fex-match") shminfo_32 {
  uint32_t shmmax;
  uint32_t shmmin;
  uint32_t shmmni;
  uint32_t shmseg;
  uint32_t shmall;

  shminfo_32() = delete;

  operator struct shminfo() const {
    struct shminfo si {};
    si.shmmax = shmmax;
    si.shmmin = shmmin;
    si.shmmni = shmmni;
    si.shmseg = shmseg;
    si.shmall = shmall;
    return si;
  }

  shminfo_32(const struct shminfo& si) {
    shmmax = si.shmmax;
    shmmin = si.shmmin;
    shmmni = si.shmmni;
    shmseg = si.shmseg;
    shmall = si.shmall;
  }
};

static_assert(std::is_trivial<shminfo_32>::value, "Needs to be trivial");
static_assert(sizeof(shminfo_32) == 20, "Incorrect size");

struct FEX_ANNOTATE("alias-x86_32-shminfo64") FEX_ANNOTATE("fex-match") shminfo_64 {
  compat_ulong_t shmmax;
  compat_ulong_t shmmin;
  compat_ulong_t shmmni;
  compat_ulong_t shmseg;
  compat_ulong_t shmall;
  compat_ulong_t __unused1;
  compat_ulong_t __unused2;
  compat_ulong_t __unused3;
  compat_ulong_t __unused4;

  shminfo_64() = delete;

  operator struct shminfo() const {
    struct shminfo si {};
    si.shmmax = shmmax;
    si.shmmin = shmmin;
    si.shmmni = shmmni;
    si.shmseg = shmseg;
    si.shmall = shmall;
    return si;
  }

  shminfo_64(const struct shminfo& si) {
    shmmax = si.shmmax;
    shmmin = si.shmmin;
    shmmni = si.shmmni;
    shmseg = si.shmseg;
    shmall = si.shmall;
    __unused1 = __unused2 = __unused3 = __unused4 = 0;
  }
};

static_assert(std::is_trivial<shminfo_64>::value, "Needs to be trivial");
static_assert(sizeof(shminfo_64) == 36, "Incorrect size");

struct FEX_ANNOTATE("alias-x86_32-shm_info") FEX_ANNOTATE("fex-match") shm_info_32 {
  int used_ids;
  uint32_t shm_tot;
  uint32_t shm_rss;
  uint32_t shm_swp;
  uint32_t swap_attempts;
  uint32_t swap_successes;

  shm_info_32() = delete;

  shm_info_32(const struct shm_info& si) {
    used_ids = si.used_ids;
    shm_tot = si.shm_tot;
    shm_rss = si.shm_rss;
    shm_swp = si.shm_swp;
    swap_attempts = si.swap_attempts;
    swap_successes = si.swap_successes;
  }
};

static_assert(std::is_trivial<shm_info_32>::value, "Needs to be trivial");
static_assert(sizeof(shm_info_32) == 24, "Incorrect size");

struct FEX_ANNOTATE("fex-match") compat_select_args {
  int nfds;
  compat_ptr<fd_set32> readfds;
  compat_ptr<fd_set32> writefds;
  compat_ptr<fd_set32> exceptfds;
  compat_ptr<struct timeval32> timeout;

  compat_select_args() = delete;
};

static_assert(std::is_trivial_v<compat_select_args>, "Needs to be trivial");
static_assert(sizeof(compat_select_args) == 20, "Incorrect size");

} // namespace FEX::HLE::x32
