#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"

#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/timex.h>
#include <unistd.h>
#include <utime.h>

namespace FEX::HLE::x32 {
  void RegisterTime() {
    REGISTER_SYSCALL_IMPL_X32(gettimeofday, [](FEXCore::Core::InternalThreadState *Thread, timeval32 *tv, struct timezone *tz) -> uint64_t {
      struct timeval tv64{};
      struct timeval *tv_ptr{};
      if (tv) {
        tv_ptr = &tv64;
      }

      uint64_t Result = ::gettimeofday(tv_ptr, tz);

      if (tv) {
        *tv = tv64;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(nanosleep, [](FEXCore::Core::InternalThreadState *Thread, const timespec32 *req, timespec32 *rem) -> uint64_t {
      struct timespec req64{};
      struct timespec rem64{};

      struct timespec *rem64_ptr{};
      req64 = *req;
      if (rem) {
        rem64 = *rem;
        rem64_ptr = &rem64;
      }
      uint64_t Result = ::nanosleep(&req64, &rem64);
      if (rem) {
        *rem = rem64;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(clock_gettime, [](FEXCore::Core::InternalThreadState *Thread, clockid_t clk_id, timespec32 *tp) -> uint64_t {
      struct timespec tp64{};
      uint64_t Result = ::clock_gettime(clk_id, &tp64);
      if (tp) {
        *tp = tp64;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(clock_getres, [](FEXCore::Core::InternalThreadState *Thread, clockid_t clk_id, timespec32 *tp) -> uint64_t {
      struct timespec tp64{};
      uint64_t Result = ::clock_getres(clk_id, &tp64);
      if (tp) {
        *tp = tp64;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(clock_nanosleep, [](FEXCore::Core::InternalThreadState *Thread, clockid_t clockid, int flags, const timespec32 *request, timespec32 *remain) -> uint64_t {
      struct timespec req64{};
      struct timespec rem64{};
      struct timespec *rem64_ptr{};

      req64 = *request;
      if (remain) {
        rem64 = *remain;
        rem64_ptr = &rem64;
      }
      uint64_t Result = ::clock_nanosleep(clockid, flags, &req64, rem64_ptr);
      if (remain) {
        *remain = rem64;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(clock_settime, [](FEXCore::Core::InternalThreadState *Thread, clockid_t clockid, const timespec32 *tp) -> uint64_t {
      struct timespec tp64{};
      tp64 = *tp;
      uint64_t Result = ::clock_settime(clockid, &tp64);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(clock_gettime64, [](FEXCore::Core::InternalThreadState *Thread, clockid_t clk_id, timespec *tp) -> uint64_t {
      uint64_t Result = ::clock_gettime(clk_id, tp);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(clock_settime64, [](FEXCore::Core::InternalThreadState *Thread, clockid_t clockid, const struct timespec *tp) -> uint64_t {
      uint64_t Result = ::clock_settime(clockid, tp);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(clock_getres_time64, [](FEXCore::Core::InternalThreadState *Thread, clockid_t clk_id, timespec *tp) -> uint64_t {
      uint64_t Result = ::clock_getres(clk_id, tp);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(clock_nanosleep_time64, [](FEXCore::Core::InternalThreadState *Thread, clockid_t clockid, int flags, const struct timespec *request, struct timespec *remain) -> uint64_t {
      uint64_t Result = ::clock_nanosleep(clockid, flags, request, remain);
      SYSCALL_ERRNO();
    });
  }
}
