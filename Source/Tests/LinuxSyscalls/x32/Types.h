/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#pragma once

#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/Utils/CompilerDefs.h>

#include <bits/types/stack_t.h>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/uio.h>
#include <time.h>
#include <type_traits>

namespace FEX::HLE::x32 {

// Basic types to make tracking easier
using compat_ulong_t = uint32_t;
using compat_long_t = int32_t;
using compat_uptr_t = uint32_t;
using compat_size_t = uint32_t;
using compat_off_t = uint32_t;
using compat_loff_t = int64_t;
using compat_pid_t = int32_t;
using compat_dev_t = uint16_t;
using compat_ino_t = uint32_t;
using compat_mode_t = uint16_t;
using compat_nlink_t = uint16_t;
using compat_uid_t = uint16_t;
using compat_gid_t = uint16_t;
using compat_old_sigset_t = uint32_t;

// Can't use using with aligned attributes, clang doesn't honour it
typedef FEX_ALIGNED(4) uint64_t compat_uint64_t;
typedef FEX_ALIGNED(4) int64_t compat_int64_t;

template<typename T>
class compat_ptr {
public:
  compat_ptr() = delete;
  compat_ptr(uint32_t In) : Ptr {In} {}
  compat_ptr(T *In) : Ptr {static_cast<uint32_t>(reinterpret_cast<uintptr_t>(In))} {}

  template<typename T2 = T,
    typename = std::enable_if<!std::is_same<T2, void>::value, T2>>
  T2& operator*() const {
    return *Interpret();
  }

  T *operator->() {
    return Interpret();
  }

  // In the case of non-void type, we can index the pointer
  template<typename T2 = T,
    typename = std::enable_if<!std::is_same<T2, void>::value, T2>>
  T2 &operator[](size_t idx) const {
    return *reinterpret_cast<T2*>(Ptr + sizeof(T2) * idx);
  }

  // In the case of void type, we need to trivially convert
  template<typename T2 = T,
    typename = std::enable_if<std::is_same<T2, void>::value, T2>>
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
 * @name timespec32
 *
 * This is a timespec implementation that matches 32bit linux implementation
 * Provides conversation operators for the host version
 * @{ */

struct
FEX_ANNOTATE("alias-x86_32-timespec")
FEX_ANNOTATE("fex-match")
timespec32 {
  int32_t tv_sec;
  int32_t tv_nsec;

  timespec32() = delete;

  operator timespec() const {
    timespec spec{};
    spec.tv_sec = tv_sec;
    spec.tv_nsec = tv_nsec;
    return spec;
  }

  timespec32(struct timespec spec) {
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

struct
FEX_ANNOTATE("alias-x86_32-timeval")
FEX_ANNOTATE("fex-match")
timeval32 {
  int32_t tv_sec;
  int32_t tv_usec;

  timeval32() = delete;

  operator timeval() const {
    timeval spec{};
    spec.tv_sec = tv_sec;
    spec.tv_usec = tv_usec;
    return spec;
  }

  timeval32(struct timeval spec) {
    tv_sec = spec.tv_sec;
    tv_usec = spec.tv_usec;
  }
};
/**  @} */

static_assert(std::is_trivial<timeval32>::value, "Needs to be trivial");
static_assert(sizeof(timeval32) == 8, "Incorrect size");

/**
 * @name iovec32
 *
 * This is a iovec implementation that matches 32bit linux implementation
 * Provides conversation operators for the host version
 * @{ */

struct
FEX_ANNOTATE("alias-x86_32-iovec")
FEX_ANNOTATE("fex-match")
iovec32 {
  uint32_t iov_base;
  uint32_t iov_len;

  iovec32() = delete;

  operator iovec() const {
    iovec vec{};
    vec.iov_base = reinterpret_cast<void*>(iov_base);
    vec.iov_len = iov_len;
    return vec;
  }

  iovec32(struct iovec vec) {
    iov_base = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(vec.iov_base));
    iov_len = vec.iov_len;
  }
};

static_assert(std::is_trivial<iovec32>::value, "Needs to be trivial");
static_assert(sizeof(iovec32) == 8, "Incorrect size");
/**  @} */

struct
FEX_ANNOTATE("alias-x86_32-cmsghdr")
FEX_ANNOTATE("fex-match")
cmsghdr32 {
  uint32_t cmsg_len;
  int32_t cmsg_level;
  int32_t cmsg_type;
  char    cmsg_data[];
};

static_assert(std::is_trivial<cmsghdr32>::value, "Needs to be trivial");
static_assert(sizeof(cmsghdr32) == 12, "Incorrect size");

struct
FEX_ANNOTATE("alias-x86_32-msghdr")
FEX_ANNOTATE("fex-match")
msghdr32 {
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

struct
FEX_ANNOTATE("alias-x86_32-mmsghdr")
FEX_ANNOTATE("fex-match")
mmsghdr_32 {
  msghdr32 msg_hdr;
  uint32_t msg_len;
};

static_assert(std::is_trivial<mmsghdr_32>::value, "Needs to be trivial");
static_assert(sizeof(mmsghdr_32) == 32, "Incorrect size");

struct
FEX_ANNOTATE("alias-x86_32-stack_t")
FEX_ANNOTATE("fex-match")
stack_t32 {
  compat_ptr<void> ss_sp;
  compat_size_t ss_size;
  int32_t ss_flags;

  stack_t32() = delete;

  operator stack_t() const {
    stack_t ss{};
    ss.ss_sp    = ss_sp;
    ss.ss_size  = ss_size;
    ss.ss_flags = ss_flags;
    return ss;
  }

  stack_t32(stack_t ss)
    : ss_sp {ss.ss_sp} {
    ss_size  = ss.ss_size;
    ss_flags = ss.ss_flags;
  }
};

static_assert(std::is_trivial<stack_t32>::value, "Needs to be trivial");
static_assert(sizeof(stack_t32) == 12, "Incorrect size");

struct
// This does not match the glibc implementation of stat
// Matches the definition of `struct compat_stat` in `arch/x86/include/asm/compat.h`
FEX_ANNOTATE("fex-match")
stat32 {
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
  uint32_t st_blocks;  /* Number 512-byte blocks allocated. */
  uint32_t st_atime_;
  uint32_t st_atime_nsec;
  uint32_t st_mtime_;
  uint32_t st_mtime_nsec;
  uint32_t st_ctime_;
  uint32_t st_ctime_nsec;
  uint32_t __unused4;
  uint32_t __unused5;

  stat32() = delete;

  stat32(struct stat host) {
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
    st_atime_nsec = host.st_atim.tv_nsec;

    st_mtime_ = host.st_mtime;
    st_mtime_nsec = host.st_mtim.tv_nsec;

    st_ctime_ = host.st_ctime;
    st_ctime_nsec = host.st_ctim.tv_nsec;
    #undef COPY
  }
};
static_assert(std::is_trivial<stat32>::value, "Needs to be trivial");
static_assert(sizeof(stat32) == 64, "Incorrect size");

struct
// This does not match the glibc implementation of stat
// Matches the definition of `struct stat64` in `x86_64-linux-gnu/asm/stat.h`
FEX_ANNOTATE("fex-match")
FEX_PACKED
stat64_32 {
  compat_uint64_t st_dev;
  uint8_t  __pad0[4];
  uint32_t __st_ino;

  uint32_t st_mode;
  uint32_t st_nlink;

  uint32_t st_uid;
  uint32_t st_gid;

  compat_uint64_t st_rdev;
  uint8_t  __pad3[4];
  compat_int64_t st_size;
  uint32_t st_blksize;
  compat_uint64_t st_blocks;  /* Number 512-byte blocks allocated. */
  uint32_t st_atime_;
  uint32_t st_atime_nsec;
  uint32_t st_mtime_;
  uint32_t st_mtime_nsec;
  uint32_t st_ctime_;
  uint32_t st_ctime_nsec;
  compat_uint64_t st_ino;

  stat64_32() = delete;

  stat64_32(struct stat host) {
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
    st_atime_nsec = host.st_atim.tv_nsec;

    st_mtime_ = host.st_mtime;
    st_mtime_nsec = host.st_mtim.tv_nsec;

    st_ctime_ = host.st_ctime;
    st_ctime_nsec = host.st_ctim.tv_nsec;
    #undef COPY
  }

  stat64_32(struct stat64 host) {
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
    st_atime_nsec = host.st_atim.tv_nsec;

    st_mtime_ = host.st_mtime;
    st_mtime_nsec = host.st_mtim.tv_nsec;

    st_ctime_ = host.st_ctime;
    st_ctime_nsec = host.st_ctim.tv_nsec;
    #undef COPY
  }
};
static_assert(std::is_trivial<stat64_32>::value, "Needs to be trivial");
static_assert(sizeof(stat64_32) == 96, "Incorrect size");

struct
FEX_PACKED
FEX_ALIGNED(4)
FEX_ANNOTATE("alias-x86_32-statfs64")
FEX_ANNOTATE("fex-match")
statfs64_32 {
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

  statfs64_32(struct statfs host) {
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

  statfs64_32(struct statfs64 host) {
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
};
static_assert(std::is_trivial<statfs64_32>::value, "Needs to be trivial");
static_assert(sizeof(statfs64_32) == 84, "Incorrect size");

struct
FEX_ANNOTATE("alias-x86_32-statfs")
FEX_ANNOTATE("fex-match")
statfs32_32 {
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

  statfs32_32(struct statfs host) {
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
};
static_assert(std::is_trivial<statfs32_32>::value, "Needs to be trivial");
static_assert(sizeof(statfs32_32) == 64, "Incorrect size");

struct
FEX_ANNOTATE("alias-x86_32-flock")
FEX_ANNOTATE("fex-match")
flock_32 {
  int16_t l_type;
  int16_t l_whence;
  int32_t l_start;
  int32_t l_len;
  int32_t l_pid;

  flock_32() = delete;

  flock_32(struct flock host) {
    l_type   = host.l_type;
    l_whence = host.l_whence;
    l_start  = host.l_start;
    l_len    = host.l_len;
    l_pid    = host.l_pid;
  }

  operator struct flock() const {
    struct flock res{};
    res.l_type   = l_type;
    res.l_whence = l_whence;
    res.l_start  = l_start;
    res.l_len    = l_len;
    res.l_pid    = l_pid;
    return res;
  }
};

static_assert(std::is_trivial<flock_32>::value, "Needs to be trivial");
static_assert(sizeof(flock_32) == 16, "Incorrect size");

// glibc doesn't pack flock64 while the kernel does
// This does not match glibc flock64 definition
// Matches the definition of `struct compat_flock64` in `arch/x86/include/asm/compat.h`
struct
FEX_ANNOTATE("fex-match")
FEX_PACKED
flock64_32 {
  int16_t l_type;
  int16_t l_whence;
  compat_loff_t l_start;
  compat_loff_t l_len;
  compat_pid_t l_pid;

  flock64_32() = delete;

  flock64_32(struct flock host) {
    l_type   = host.l_type;
    l_whence = host.l_whence;
    l_start  = host.l_start;
    l_len    = host.l_len;
    l_pid    = host.l_pid;
  }

  operator struct flock() const {
    struct flock res{};
    res.l_type   = l_type;
    res.l_whence = l_whence;
    res.l_start  = l_start;
    res.l_len    = l_len;
    res.l_pid    = l_pid;
    return res;
  }
};
static_assert(std::is_trivial<flock64_32>::value, "Needs to be trivial");
static_assert(sizeof(flock64_32) == 24, "Incorrect size");

// There is no public definition of this struct
// Matches the definition of `struct linux_dirent` in fs/readdir.c
struct
FEX_ANNOTATE("fex-match")
linux_dirent {
  uint64_t d_ino;
  int64_t  d_off;
  uint16_t d_reclen;
  uint8_t _pad[6];
  char d_name[];
};
static_assert(std::is_trivial<linux_dirent>::value, "Needs to be trivial");
static_assert(sizeof(linux_dirent) == 24, "Incorrect size");

// There is no public definition of this struct
// Matches the definition of `struct compat_linux_dirent` in fs/readdir.c
struct
FEX_ANNOTATE("fex-match")
linux_dirent_32 {
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
struct
FEX_ANNOTATE("fex-match")
linux_dirent_64 {
  uint64_t d_ino;
  uint64_t d_off;
  uint16_t d_reclen;
  uint8_t  d_type;
  uint8_t _pad[5];
  char d_name[];
};
static_assert(std::is_trivial<linux_dirent_64>::value, "Needs to be trivial");
static_assert(sizeof(linux_dirent_64) == 24, "Incorrect size");

// There is no public definition of this struct
// Matches `struct compat_sigset_argpack`
struct
FEX_ANNOTATE("fex-match")
sigset_argpack32 {
  compat_ptr<uint64_t> sigset;
  compat_size_t size;
};

static_assert(std::is_trivial<sigset_argpack32>::value, "Needs to be trivial");
static_assert(sizeof(sigset_argpack32) == 8, "Incorrect size");

struct
FEX_ANNOTATE("alias-x86_32-rusage")
FEX_ANNOTATE("fex-match")
rusage_32 {
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
  rusage_32(struct rusage usage)
    : ru_utime { usage.ru_utime }
    , ru_stime { usage.ru_stime } {
    // These only truncate
    ru_maxrss   = usage.ru_maxrss;
    ru_ixrss    = usage.ru_ixrss;
    ru_idrss    = usage.ru_idrss;
    ru_isrss    = usage.ru_isrss;
    ru_minflt   = usage.ru_minflt;
    ru_majflt   = usage.ru_majflt;
    ru_nswap    = usage.ru_nswap;
    ru_inblock  = usage.ru_inblock;
    ru_oublock  = usage.ru_oublock;
    ru_msgsnd   = usage.ru_msgsnd;
    ru_msgrcv   = usage.ru_msgrcv;
    ru_nsignals = usage.ru_nsignals;
    ru_nvcsw    = usage.ru_nvcsw;
    ru_nivcsw   = usage.ru_nivcsw;
  }

  operator struct rusage() const {
    struct rusage usage{};
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

// This definition isn't public
// This is for rt_sigaction
// Matches the definition for `struct compat_sigaction` in `include/linux/compat.h`
struct
FEX_PACKED
FEX_ANNOTATE("fex-match")
GuestSigAction_32 {
  FEX::HLE::x32::compat_ptr<void> handler_32;

  uint32_t sa_flags;
  FEX::HLE::x32::compat_ptr<void> restorer_32;
  FEXCore::GuestSAMask sa_mask;

  GuestSigAction_32() = delete;

  operator FEXCore::GuestSigAction() const {
    FEXCore::GuestSigAction action{};

    action.sigaction_handler.handler = reinterpret_cast<decltype(action.sigaction_handler.handler)>(handler_32.Ptr);
    action.sa_flags = sa_flags;
    action.restorer = reinterpret_cast<decltype(action.restorer)>(restorer_32.Ptr);
    action.sa_mask = sa_mask;
    return action;
  }

  GuestSigAction_32(FEXCore::GuestSigAction action)
    : handler_32 {reinterpret_cast<void*>(action.sigaction_handler.handler)}
    , restorer_32 {reinterpret_cast<void*>(action.restorer)} {
    sa_flags = action.sa_flags;
    sa_mask = action.sa_mask;
  }
};

static_assert(std::is_trivial<GuestSigAction_32>::value, "Needs to be trivial");
static_assert(sizeof(GuestSigAction_32) == 20, "Incorrect size");

}
