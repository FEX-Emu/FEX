/*
$info$
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"

#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/timex.h>
#include <unistd.h>
#include <utime.h>

namespace FEX::HLE::x64 {
  void RegisterTime() {
    REGISTER_SYSCALL_IMPL_X64(gettimeofday, [](FEXCore::Core::CpuStateFrame *Frame, struct timeval *tv, struct timezone *tz) -> uint64_t {
      uint64_t Result = ::gettimeofday(tv, tz);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(nanosleep, [](FEXCore::Core::CpuStateFrame *Frame, const struct timespec *req, struct timespec *rem) -> uint64_t {
      uint64_t Result = ::nanosleep(req, rem);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(clock_gettime, [](FEXCore::Core::CpuStateFrame *Frame, clockid_t clk_id, struct timespec *tp) -> uint64_t {
      uint64_t Result = ::clock_gettime(clk_id, tp);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(clock_getres, [](FEXCore::Core::CpuStateFrame *Frame, clockid_t clk_id, struct timespec *tp) -> uint64_t {
      uint64_t Result = ::clock_getres(clk_id, tp);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(clock_nanosleep, [](FEXCore::Core::CpuStateFrame *Frame, clockid_t clockid, int flags, const struct timespec *request, struct timespec *remain) -> uint64_t {
      uint64_t Result = ::clock_nanosleep(clockid, flags, request, remain);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(clock_settime, [](FEXCore::Core::CpuStateFrame *Frame, clockid_t clockid, const struct timespec *tp) -> uint64_t {
      uint64_t Result = ::clock_settime(clockid, tp);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(settimeofday, [](FEXCore::Core::CpuStateFrame *Frame, const struct timeval *tv, const struct timezone *tz) -> uint64_t {
      uint64_t Result = ::settimeofday(tv, tz);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(utimes, [](FEXCore::Core::CpuStateFrame *Frame, const char *filename, const struct timeval times[2]) -> uint64_t {
      uint64_t Result = ::utimes(filename, times);
      SYSCALL_ERRNO();
    });
  }
}

