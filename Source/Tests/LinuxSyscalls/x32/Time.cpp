/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Types.h"

#include "Tests/LinuxSyscalls/x64/Syscalls.h"

#include <bits/types/clockid_t.h>
#include <stdint.h>
#include <syscall.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/timex.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

ARG_TO_STR(FEX::HLE::x32::compat_ptr<FEX::HLE::x32::timespec32>, "%lx")
ARG_TO_STR(FEX::HLE::x32::compat_ptr<FEX::HLE::x32::timex32>, "%lx")

struct timespec;
namespace FEXCore::Core {
  struct CpuStateFrame;
}

namespace FEX::HLE::x32 {
  void RegisterTime() {

    REGISTER_SYSCALL_IMPL_X32(time, [](FEXCore::Core::CpuStateFrame *Frame, FEX::HLE::x32::old_time32_t *tloc) -> uint64_t {
      time_t Host{};
      uint64_t Result = ::time(&Host);

      if (tloc) {
        // On 32-bit this truncates
        *tloc = (FEX::HLE::x32::old_time32_t)Host;
      }

      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(times, [](FEXCore::Core::CpuStateFrame *Frame, struct FEX::HLE::x32::compat_tms *buf) -> uint64_t {
      struct tms Host{};
      uint64_t Result = ::times(&Host);
      if (buf) {
        *buf = Host;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(utime, [](FEXCore::Core::CpuStateFrame *Frame, char* filename, const FEX::HLE::x32::old_utimbuf32* times) -> uint64_t {
      struct utimbuf Host{};
      struct utimbuf *Host_p{};
      if (times) {
        Host = *times;
        Host_p = &Host;
      }
      uint64_t Result = ::utime(filename, Host_p);
      SYSCALL_ERRNO();
    });

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

    REGISTER_SYSCALL_IMPL_X32(settimeofday, [](FEXCore::Core::CpuStateFrame *Frame, const timeval32 *tv, const struct timezone *tz) -> uint64_t {
      struct timeval tv64{};
      struct timeval *tv_ptr{};
      if (tv) {
        tv64 = *tv;
        tv_ptr = &tv64;
      }

      const uint64_t Result = ::settimeofday(tv_ptr, tz);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(nanosleep, [](FEXCore::Core::CpuStateFrame *Frame, const timespec32 *req, timespec32 *rem) -> uint64_t {
      struct timespec rem64{};
      struct timespec *rem64_ptr{};

      if (rem) {
        rem64 = *rem;
        rem64_ptr = &rem64;
      }

      uint64_t Result = 0;
      if (req) {
        const struct timespec req64 = *req;
        Result = ::nanosleep(&req64, rem64_ptr);
      } else {
        Result = ::nanosleep(nullptr, rem64_ptr);
      }

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
      struct timespec rem64{};
      struct timespec *rem64_ptr{};

      if (remain) {
        rem64 = *remain;
        rem64_ptr = &rem64;
      }

      uint64_t Result = 0;
      if (request) {
        const struct timespec req64 = *request;
        Result = ::clock_nanosleep(clockid, flags, &req64, rem64_ptr);
      } else {
        Result = ::clock_nanosleep(clockid, flags, nullptr, rem64_ptr);
      }

      if (remain) {
        *remain = rem64;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(clock_settime, [](FEXCore::Core::CpuStateFrame *Frame, clockid_t clockid, const timespec32 *tp) -> uint64_t {
      uint64_t Result = 0;
      if (tp) {
        const struct timespec tp64 = *tp;
        Result = ::clock_settime(clockid, &tp64);
      } else {
        Result = ::clock_settime(clockid, nullptr);
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(futimesat, [](FEXCore::Core::CpuStateFrame *Frame, int dirfd, const char *pathname, const timeval32 times[2]) -> uint64_t {
      uint64_t Result = 0;
      if (times) {
        struct timeval times64[2]{};
        times64[0] = times[0];
        times64[1] = times[1];
        Result = ::futimesat(dirfd, pathname, times64);
      } else {
        Result = ::futimesat(dirfd, pathname, nullptr);
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(utimensat, [](FEXCore::Core::CpuStateFrame *Frame, int dirfd, const char *pathname, const compat_ptr<timespec32> times, int flags) -> uint64_t {
      uint64_t Result = 0;
      if (times) {
        timespec times64[2]{};
        times64[0] = times[0];
        times64[1] = times[1];
        Result = ::syscall(SYSCALL_DEF(utimensat), dirfd, pathname, times64, flags);
      } else {
        Result = ::syscall(SYSCALL_DEF(utimensat), dirfd, pathname, nullptr, flags);
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL(clock_gettime64, clock_gettime, [](FEXCore::Core::CpuStateFrame *Frame, clockid_t clk_id, timespec *tp) -> uint64_t {
      uint64_t Result = ::clock_gettime(clk_id, tp);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL(clock_adjtime64, clock_adjtime, [](FEXCore::Core::CpuStateFrame *Frame, clockid_t clk_id, struct timex *buf) -> uint64_t {
      uint64_t Result = ::clock_adjtime(clk_id, buf);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL(clock_settime64, clock_settime, [](FEXCore::Core::CpuStateFrame *Frame, clockid_t clockid, const struct timespec *tp) -> uint64_t {
      uint64_t Result = ::clock_settime(clockid, tp);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL(clock_getres_time64, clock_getres, [](FEXCore::Core::CpuStateFrame *Frame, clockid_t clk_id, timespec *tp) -> uint64_t {
      uint64_t Result = ::clock_getres(clk_id, tp);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL(clock_nanosleep_time64, clock_nanosleep, [](FEXCore::Core::CpuStateFrame *Frame, clockid_t clockid, int flags, const struct timespec *request, struct timespec *remain) -> uint64_t {
      uint64_t Result = ::clock_nanosleep(clockid, flags, request, remain);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL(utimensat_time64, utimensat, [](FEXCore::Core::CpuStateFrame *Frame, int dirfd, const char *pathname, const struct timespec times[2], int flags) -> uint64_t {
      uint64_t Result = ::utimensat(dirfd, pathname, times, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(utimes, [](FEXCore::Core::CpuStateFrame *Frame, const char *filename, const timeval32 times[2]) -> uint64_t {
      uint64_t Result = 0;
      if (times) {
        struct timeval times64[2]{};
        times64[0] = times[0];
        times64[1] = times[1];
        Result = ::utimes(filename, times64);
      } else {
        Result = ::utimes(filename, nullptr);
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(adjtimex, [](FEXCore::Core::CpuStateFrame *Frame, compat_ptr<FEX::HLE::x32::timex32> buf) -> uint64_t {
      struct timex Host{};
      Host = *buf;
      uint64_t Result = ::adjtimex(&Host);
      if (Result != -1) {
        *buf = Host;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(clock_adjtime, [](FEXCore::Core::CpuStateFrame *Frame, clockid_t clk_id, compat_ptr<FEX::HLE::x32::timex32> buf) -> uint64_t {
      struct timex Host{};
      Host = *buf;
      uint64_t Result = ::clock_adjtime(clk_id, &Host);
      if (Result != -1) {
        *buf = Host;
      }
      SYSCALL_ERRNO();
    });
  }
}
