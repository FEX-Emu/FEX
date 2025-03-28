// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x32/Syscalls.h"
#include "LinuxSyscalls/x32/Types.h"

#include "LinuxSyscalls/x64/Syscalls.h"

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
void RegisterTime(FEX::HLE::SyscallHandler* Handler) {

  REGISTER_SYSCALL_IMPL_X32(time, [](FEXCore::Core::CpuStateFrame* Frame, FEX::HLE::x32::old_time32_t* tloc) -> uint64_t {
    time_t Host {};
    uint64_t Result = ::time(&Host);

    if (tloc) {
      FaultSafeUserMemAccess::VerifyIsWritable(tloc, sizeof(*tloc));
      // On 32-bit this truncates
      *tloc = (FEX::HLE::x32::old_time32_t)Host;
    }

    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(times, [](FEXCore::Core::CpuStateFrame* Frame, struct FEX::HLE::x32::compat_tms* buf) -> uint64_t {
    struct tms Host {};
    uint64_t Result = ::times(&Host);
    if (buf) {
      FaultSafeUserMemAccess::VerifyIsWritable(buf, sizeof(*buf));
      *buf = Host;
    }
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(utime, [](FEXCore::Core::CpuStateFrame* Frame, char* filename, const FEX::HLE::x32::old_utimbuf32* times) -> uint64_t {
    struct utimbuf Host {};
    struct utimbuf* Host_p {};
    if (times) {
      FaultSafeUserMemAccess::VerifyIsReadable(times, sizeof(*times));
      Host = *times;
      Host_p = &Host;
    }
    uint64_t Result = ::utime(filename, Host_p);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(gettimeofday, [](FEXCore::Core::CpuStateFrame* Frame, timeval32* tv, struct timezone* tz) -> uint64_t {
    struct timeval tv64 {};
    struct timeval* tv_ptr {};
    if (tv) {
      tv_ptr = &tv64;
    }

    uint64_t Result = ::gettimeofday(tv_ptr, tz);

    if (tv) {
      FaultSafeUserMemAccess::VerifyIsWritable(tv, sizeof(*tv));
      *tv = tv64;
    }
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(settimeofday, [](FEXCore::Core::CpuStateFrame* Frame, const timeval32* tv, const struct timezone* tz) -> uint64_t {
    struct timeval tv64 {};
    struct timeval* tv_ptr {};
    if (tv) {
      FaultSafeUserMemAccess::VerifyIsReadable(tv, sizeof(*tv));
      tv64 = *tv;
      tv_ptr = &tv64;
    }

    const uint64_t Result = ::settimeofday(tv_ptr, tz);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(nanosleep, [](FEXCore::Core::CpuStateFrame* Frame, const timespec32* req, timespec32* rem) -> uint64_t {
    struct timespec rem64 {};
    struct timespec* rem64_ptr {};

    if (rem) {
      FaultSafeUserMemAccess::VerifyIsReadable(rem, sizeof(*rem));
      rem64 = *rem;
      rem64_ptr = &rem64;
    }

    uint64_t Result = 0;
    if (req) {
      FaultSafeUserMemAccess::VerifyIsReadable(req, sizeof(*req));
      const struct timespec req64 = *req;
      Result = ::nanosleep(&req64, rem64_ptr);
    } else {
      Result = ::nanosleep(nullptr, rem64_ptr);
    }

    if (rem) {
      FaultSafeUserMemAccess::VerifyIsWritable(rem, sizeof(*rem));
      *rem = rem64;
    }
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(clock_gettime, [](FEXCore::Core::CpuStateFrame* Frame, clockid_t clk_id, timespec32* tp) -> uint64_t {
    struct timespec tp64 {};
    uint64_t Result = ::clock_gettime(clk_id, &tp64);
    if (tp) {
      FaultSafeUserMemAccess::VerifyIsWritable(tp, sizeof(*tp));
      *tp = tp64;
    }
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(clock_getres, [](FEXCore::Core::CpuStateFrame* Frame, clockid_t clk_id, timespec32* tp) -> uint64_t {
    struct timespec tp64 {};
    uint64_t Result = ::clock_getres(clk_id, &tp64);
    if (tp) {
      FaultSafeUserMemAccess::VerifyIsWritable(tp, sizeof(*tp));
      *tp = tp64;
    }
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(
    clock_nanosleep, [](FEXCore::Core::CpuStateFrame* Frame, clockid_t clockid, int flags, const timespec32* request, timespec32* remain) -> uint64_t {
      struct timespec req64 {};
      struct timespec* req64_ptr {};

      struct timespec rem64 {};
      struct timespec* rem64_ptr {};

      if (request) {
        FaultSafeUserMemAccess::VerifyIsReadable(request, sizeof(*request));
        req64 = *request;
        req64_ptr = &req64;
      }

      if (remain) {
        FaultSafeUserMemAccess::VerifyIsReadable(remain, sizeof(*remain));
        rem64 = *remain;
        rem64_ptr = &rem64;
      }

      // Can't use glibc helper here since it does additional validation and data munging that breaks games.
      uint64_t Result = ::syscall(SYSCALL_DEF(clock_nanosleep), clockid, flags, req64_ptr, rem64_ptr);

      if (remain && (flags & TIMER_ABSTIME) == 0) {
        FaultSafeUserMemAccess::VerifyIsWritable(remain, sizeof(*remain));
        // Remain is completely ignored if TIMER_ABSTIME is set.
        *remain = rem64;
      }
      SYSCALL_ERRNO();
    });

  REGISTER_SYSCALL_IMPL_X32(clock_settime, [](FEXCore::Core::CpuStateFrame* Frame, clockid_t clockid, const timespec32* tp) -> uint64_t {
    if (!tp) {
      // clock_settime is required to pass a timespec.
      return -EFAULT;
    }

    uint64_t Result = 0;
    FaultSafeUserMemAccess::VerifyIsReadable(tp, sizeof(*tp));
    const struct timespec tp64 = *tp;
    Result = ::clock_settime(clockid, &tp64);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(futimesat, [](FEXCore::Core::CpuStateFrame* Frame, int dirfd, const char* pathname, const timeval32 times[2]) -> uint64_t {
    return FEX::HLE::futimesat_compat<timeval32>(dirfd, pathname, times);
  });

  REGISTER_SYSCALL_IMPL_X32(
    utimensat, [](FEXCore::Core::CpuStateFrame* Frame, int dirfd, const char* pathname, const compat_ptr<timespec32> times, int flags) -> uint64_t {
      uint64_t Result = 0;
      if (times) {
        FaultSafeUserMemAccess::VerifyIsReadable(times, sizeof(timeval32) * 2);
        timespec times64[2] {};
        times64[0] = times[0];
        times64[1] = times[1];
        Result = ::syscall(SYSCALL_DEF(utimensat), dirfd, pathname, times64, flags);
      } else {
        Result = ::syscall(SYSCALL_DEF(utimensat), dirfd, pathname, nullptr, flags);
      }
      SYSCALL_ERRNO();
    });

  REGISTER_SYSCALL_IMPL_X32(utimes, [](FEXCore::Core::CpuStateFrame* Frame, const char* filename, const timeval32 times[2]) -> uint64_t {
    uint64_t Result = 0;
    if (times) {
      FaultSafeUserMemAccess::VerifyIsReadable(times, sizeof(timeval32) * 2);
      struct timeval times64[2] {};
      times64[0] = times[0];
      times64[1] = times[1];
      Result = ::utimes(filename, times64);
    } else {
      Result = ::utimes(filename, nullptr);
    }
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(adjtimex, [](FEXCore::Core::CpuStateFrame* Frame, compat_ptr<FEX::HLE::x32::timex32> buf) -> uint64_t {
    FaultSafeUserMemAccess::VerifyIsReadable(buf, sizeof(*buf));
    struct timex Host {};
    Host = *buf;
    uint64_t Result = ::adjtimex(&Host);
    if (Result != -1) {
      FaultSafeUserMemAccess::VerifyIsWritable(buf, sizeof(*buf));
      *buf = Host;
    }
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(clock_adjtime,
                            [](FEXCore::Core::CpuStateFrame* Frame, clockid_t clk_id, compat_ptr<FEX::HLE::x32::timex32> buf) -> uint64_t {
                              FaultSafeUserMemAccess::VerifyIsReadable(buf, sizeof(*buf));
                              struct timex Host {};
                              Host = *buf;
                              uint64_t Result = ::clock_adjtime(clk_id, &Host);
                              if (Result != -1) {
                                FaultSafeUserMemAccess::VerifyIsWritable(buf, sizeof(*buf));
                                *buf = Host;
                              }
                              SYSCALL_ERRNO();
                            });
}
} // namespace FEX::HLE::x32
