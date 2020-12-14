#pragma once

#include <bits/types/stack_t.h>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/uio.h>
#include <time.h>
#include <type_traits>

namespace FEX::HLE::x32 {

// Basic types to make tracking easier
using compat_ulong_t = uint32_t;
using compat_uptr_t = uint32_t;
using compat_size_t = uint32_t;

template<typename T>
class compat_ptr {
public:
  compat_ptr() = delete;
  compat_ptr(uint32_t In) : Ptr {In} {}
  compat_ptr(T *In) : Ptr {static_cast<uint32_t>(reinterpret_cast<uintptr_t>(In))} {}

  T operator*() {
    return *Interpret();
  }

  T *operator->() {
    return Interpret();
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

struct timespec32 {
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

struct timeval32 {
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

struct iovec32 {
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

struct cmsghdr32 {
  uint32_t cmsg_len;
  int32_t cmsg_level;
  int32_t cmsg_type;
  char    cmsg_data[0];
};

static_assert(std::is_trivial<cmsghdr32>::value, "Needs to be trivial");
static_assert(sizeof(cmsghdr32) == 12, "Incorrect size");

struct msghdr32 {
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

struct stack_t32 {
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

struct stat32 {
  uint32_t st_dev;
  uint32_t st_ino;
  uint32_t st_nlink;

  uint16_t st_mode;
  uint16_t st_uid;
  uint16_t st_gid;
  uint16_t __pad0;

  uint32_t st_rdev;
  uint32_t st_size;
  uint32_t st_blksize;
  uint32_t st_blocks;  /* Number 512-byte blocks allocated. */
  uint32_t st_atime_;
  uint32_t st_atime_nsec;
  uint32_t st_mtime_;
  uint32_t st_mtime_nsec;
  uint32_t st_ctime_;
  uint32_t st_ctime_nsec;
  uint32_t __unused[3];

  stat32() = delete;

  stat32(struct stat host) {
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
static_assert(sizeof(stat32) == 72, "Incorrect size");

struct __attribute__((packed)) stat64_32 {
  uint64_t st_dev;
  uint32_t pad0;
  uint32_t __st_ino;

  uint32_t st_mode;
  uint32_t st_nlink;

  uint32_t st_uid;
  uint32_t st_gid;

  uint64_t st_rdev;
  uint32_t pad3;
  int64_t st_size;
  uint32_t st_blksize;
  uint64_t st_blocks;  /* Number 512-byte blocks allocated. */
  uint32_t st_atime_;
  uint32_t st_atime_nsec;
  uint32_t st_mtime_;
  uint32_t st_mtime_nsec;
  uint32_t st_ctime_;
  uint32_t st_ctime_nsec;
  uint64_t st_ino;

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

struct __attribute__((packed,aligned(4))) statfs64_32 {
  uint32_t f_type;
  uint32_t f_bsize;
  uint64_t f_blocks;
  uint64_t f_bfree;
  uint64_t f_bavail;
  uint64_t f_files;
  uint64_t f_ffree;
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

struct flock_32 {
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
struct flock64_32 {
  int16_t l_type;
  int16_t l_whence;
  int32_t l_start;
  int32_t l_len;
  int32_t l_pid;

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
static_assert(sizeof(flock64_32) == 16, "Incorrect size");

struct linux_dirent {
  uint64_t d_ino;
  int64_t  d_off;
  uint16_t d_reclen;
  uint8_t _pad[6];
  char d_name[];
};
static_assert(std::is_trivial<linux_dirent>::value, "Needs to be trivial");
static_assert(sizeof(linux_dirent) == 24, "Incorrect size");

struct linux_dirent_32 {
  uint32_t d_ino;
  int32_t d_off;
  uint16_t d_reclen;
  uint8_t _pad[2];
  char d_name[];
  /* Has hidden null character and d_type */
};
static_assert(std::is_trivial<linux_dirent_32>::value, "Needs to be trivial");
static_assert(sizeof(linux_dirent_32) == 12, "Incorrect size");

struct linux_dirent_64 {
  uint64_t d_ino;
  uint64_t d_off;
  uint16_t d_reclen;
  uint8_t  d_type;
  uint8_t _pad[5];
  char d_name[];
};
static_assert(std::is_trivial<linux_dirent_64>::value, "Needs to be trivial");
static_assert(sizeof(linux_dirent_64) == 24, "Incorrect size");

}
