#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <poll.h>
#include <signal.h>

TEST_CASE("poll") {
  // poll can return EFAULT if first argument is pointed to invalid pointer.
  // Using mmap specifically for allocating with PROT_NONE.
  struct pollfd* invalid_fds =
    reinterpret_cast<struct pollfd*>(mmap(nullptr, sysconf(_SC_PAGESIZE), PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
  auto ret = ::syscall(SYS_poll, invalid_fds, 1, 0);
  REQUIRE(ret == -1);
  CHECK(errno == EFAULT);
}

TEST_CASE("ppoll") {
  // ppoll can return EFAULT for arguments 1, 3, 4.
  // Using mmap specifically for allocating with PROT_NONE.
  struct pollfd* invalid_fds =
    reinterpret_cast<struct pollfd*>(mmap(nullptr, sysconf(_SC_PAGESIZE), PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
  struct timespec* invalid_timespec =
    reinterpret_cast<struct timespec*>(mmap(nullptr, sysconf(_SC_PAGESIZE), PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
  sigset_t* invalid_sigset = reinterpret_cast<sigset_t*>(mmap(nullptr, sysconf(_SC_PAGESIZE), PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));

  SECTION("invalid fds") {
    auto ret = ::syscall(SYS_ppoll, invalid_fds, 1, 0, nullptr, nullptr);
    REQUIRE(ret == -1);
    CHECK(errno == EFAULT);
  }

  SECTION("invalid timespec") {
    struct pollfd valid_fds {
      .fd = STDOUT_FILENO,
      .events = 0,
      .revents = 0,
    };
    auto ret = ::syscall(SYS_ppoll, &valid_fds, 1, invalid_timespec, nullptr, sizeof(uint64_t));
    REQUIRE(ret == -1);
    CHECK(errno == EFAULT);
  }

  SECTION("invalid sigset") {
    struct pollfd valid_fds {
      .fd = STDOUT_FILENO,
      .events = 0,
      .revents = 0,
    };

    struct timespec valid_ts {};
    auto ret = ::syscall(SYS_ppoll, &valid_fds, 1, &valid_ts, invalid_sigset, sizeof(uint64_t));
    REQUIRE(ret == -1);
    CHECK(errno == EFAULT);
  }

  SECTION("valid configuration") {
    struct pollfd valid_fds {
      .fd = STDOUT_FILENO,
      .events = 0,
      .revents = 0,
    };

    struct timespec valid_ts {};
    sigset_t valid_sigset {};
    sigemptyset(&valid_sigset);
    auto ret = ::syscall(SYS_ppoll, &valid_fds, 1, &valid_ts, &valid_sigset, sizeof(uint64_t));
    REQUIRE(ret == 0);
  }

  SECTION("invalid timespec write-back") {
    struct pollfd valid_fds {
      .fd = STDOUT_FILENO,
      .events = 0,
      .revents = 0,
    };

    // Kernel will read timespec, but it then can't write the result back.
    mprotect(invalid_timespec, sysconf(_SC_PAGESIZE), PROT_READ | PROT_WRITE);
    invalid_timespec->tv_sec = 1;
    mprotect(invalid_timespec, sysconf(_SC_PAGESIZE), PROT_READ);

    sigset_t valid_sigset {};
    sigemptyset(&valid_sigset);
    auto ret = ::syscall(SYS_ppoll, &valid_fds, 1, invalid_timespec, &valid_sigset, sizeof(uint64_t));
    REQUIRE(ret == 0);
    CHECK(invalid_timespec->tv_sec == 1);
  }
}

struct timespec64 {
  uint64_t tv_sec, tv_nsec;
};

static const timespec64 readonly_ts {
  .tv_sec = 1,
  .tv_nsec = 0,
};


TEST_CASE("ppoll_64") {
#ifndef SYS_ppoll_time64
#define SYS_ppoll_time64 SYS_ppoll
#endif
  // ppoll can return EFAULT for arguments 1, 3, 4
  // Using mmap specifically for allocating with PROT_NONE.
  struct pollfd* invalid_fds =
    reinterpret_cast<struct pollfd*>(mmap(nullptr, sysconf(_SC_PAGESIZE), PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
  timespec64* invalid_timespec =
    reinterpret_cast<timespec64*>(mmap(nullptr, sysconf(_SC_PAGESIZE), PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
  sigset_t* invalid_sigset = reinterpret_cast<sigset_t*>(mmap(nullptr, sysconf(_SC_PAGESIZE), PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));

  SECTION("invalid fds") {
    auto ret = ::syscall(SYS_ppoll_time64, invalid_fds, 1, 0, nullptr, nullptr);
    REQUIRE(ret == -1);
    CHECK(errno == EFAULT);
  }

  SECTION("invalid timespec") {
    struct pollfd valid_fds {
      .fd = STDOUT_FILENO,
      .events = 0,
      .revents = 0,
    };
    auto ret = ::syscall(SYS_ppoll_time64, &valid_fds, 1, invalid_timespec, nullptr, sizeof(uint64_t));
    REQUIRE(ret == -1);
    CHECK(errno == EFAULT);
  }

  SECTION("invalid sigset") {
    struct pollfd valid_fds {
      .fd = STDOUT_FILENO,
      .events = 0,
      .revents = 0,
    };

    timespec64 valid_ts {};
    auto ret = ::syscall(SYS_ppoll_time64, &valid_fds, 1, &valid_ts, invalid_sigset, sizeof(uint64_t));
    REQUIRE(ret == -1);
    CHECK(errno == EFAULT);
  }

  SECTION("valid configuration") {
    struct pollfd valid_fds {
      .fd = STDOUT_FILENO,
      .events = 0,
      .revents = 0,
    };

    timespec64 valid_ts {};
    sigset_t valid_sigset {};
    sigemptyset(&valid_sigset);
    auto ret = ::syscall(SYS_ppoll_time64, &valid_fds, 1, &valid_ts, &valid_sigset, sizeof(uint64_t));
    REQUIRE(ret == 0);
  }

  SECTION("invalid timespec write-back") {
    struct pollfd valid_fds {
      .fd = STDOUT_FILENO,
      .events = 0,
      .revents = 0,
    };

    // Kernel will read timespec, but it then can't write the result back.
    sigset_t valid_sigset {};
    sigemptyset(&valid_sigset);
    auto ret = ::syscall(SYS_ppoll_time64, &valid_fds, 1, &readonly_ts, &valid_sigset, sizeof(uint64_t));
    REQUIRE(ret == 0);
    CHECK(readonly_ts.tv_sec == 1);
  }
}
