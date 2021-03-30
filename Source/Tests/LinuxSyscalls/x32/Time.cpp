/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"

#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/timex.h>
#include <unistd.h>
#include <utime.h>

ARG_TO_STR(FEX::HLE::x32::compat_ptr<FEX::HLE::x32::timespec32>, "%lx")

namespace FEX::HLE::x32 {
  void RegisterTime() {
    REGISTER_SYSCALL_IMPL_X32(gettimeofday, [](FEXCore::Core::CpuStateFrame *Frame, timeval32 *tv, struct timezone *tz) -> uint64_t {
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

    REGISTER_SYSCALL_IMPL_X32(nanosleep, [](FEXCore::Core::CpuStateFrame *Frame, const timespec32 *req, timespec32 *rem) -> uint64_t {
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

    REGISTER_SYSCALL_IMPL_X32(clock_gettime, [](FEXCore::Core::CpuStateFrame *Frame, clockid_t clk_id, timespec32 *tp) -> uint64_t {
      struct timespec tp64{};
      uint64_t Result = ::clock_gettime(clk_id, &tp64);
      if (tp) {
        *tp = tp64;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(clock_getres, [](FEXCore::Core::CpuStateFrame *Frame, clockid_t clk_id, timespec32 *tp) -> uint64_t {
      struct timespec tp64{};
      uint64_t Result = ::clock_getres(clk_id, &tp64);
      if (tp) {
        *tp = tp64;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(clock_nanosleep, [](FEXCore::Core::CpuStateFrame *Frame, clockid_t clockid, int flags, const timespec32 *request, timespec32 *remain) -> uint64_t {
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

    REGISTER_SYSCALL_IMPL_X32(clock_settime, [](FEXCore::Core::CpuStateFrame *Frame, clockid_t clockid, const timespec32 *tp) -> uint64_t {
      struct timespec tp64{};
      tp64 = *tp;
      uint64_t Result = ::clock_settime(clockid, &tp64);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(utimensat, [](FEXCore::Core::CpuStateFrame *Frame, int dirfd, const char *pathname, const compat_ptr<timespec32> times, int flags) -> uint64_t {
      timespec times64[2]{};
      times64[0] = times[0];
      times64[1] = times[1];

      uint64_t Result = ::syscall(SYS_utimensat, dirfd, pathname, times64, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(clock_gettime64, [](FEXCore::Core::CpuStateFrame *Frame, clockid_t clk_id, timespec *tp) -> uint64_t {
      uint64_t Result = ::clock_gettime(clk_id, tp);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(clock_adjtime64, [](FEXCore::Core::CpuStateFrame *Frame, clockid_t clk_id, struct timex *buf) -> uint64_t {
      uint64_t Result = ::clock_adjtime(clk_id, buf);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(clock_settime64, [](FEXCore::Core::CpuStateFrame *Frame, clockid_t clockid, const struct timespec *tp) -> uint64_t {
      uint64_t Result = ::clock_settime(clockid, tp);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(clock_getres_time64, [](FEXCore::Core::CpuStateFrame *Frame, clockid_t clk_id, timespec *tp) -> uint64_t {
      uint64_t Result = ::clock_getres(clk_id, tp);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(clock_nanosleep_time64, [](FEXCore::Core::CpuStateFrame *Frame, clockid_t clockid, int flags, const struct timespec *request, struct timespec *remain) -> uint64_t {
      uint64_t Result = ::clock_nanosleep(clockid, flags, request, remain);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(utimensat_time64, [](FEXCore::Core::CpuStateFrame *Frame, int dirfd, const char *pathname, const struct timespec times[2], int flags) -> uint64_t {
      uint64_t Result = ::utimensat(dirfd, pathname, times, flags);
      SYSCALL_ERRNO();
    });
  }
}
