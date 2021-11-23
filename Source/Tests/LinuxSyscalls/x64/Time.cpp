/*
$info$
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/Types.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"

#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/timex.h>
#include <unistd.h>
#include <utime.h>

namespace FEX::HLE::x64 {
  void RegisterTime() {
    REGISTER_SYSCALL_IMPL_X64_PASS(time, [](FEXCore::Core::CpuStateFrame *Frame, time_t *tloc) -> uint64_t {
      uint64_t Result = ::time(tloc);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(times, [](FEXCore::Core::CpuStateFrame *Frame, struct tms *buf) -> uint64_t {
      uint64_t Result = ::times(buf);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(utime, [](FEXCore::Core::CpuStateFrame *Frame, char* filename, const struct utimbuf* times) -> uint64_t {
      uint64_t Result = ::utime(filename, times);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(gettimeofday, [](FEXCore::Core::CpuStateFrame *Frame, struct timeval *tv, struct timezone *tz) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(gettimeofday), tv, tz);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(nanosleep, [](FEXCore::Core::CpuStateFrame *Frame, const struct timespec *req, struct timespec *rem) -> uint64_t {
      uint64_t Result = ::nanosleep(req, rem);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(clock_gettime, [](FEXCore::Core::CpuStateFrame *Frame, clockid_t clk_id, struct timespec *tp) -> uint64_t {
      uint64_t Result = ::clock_gettime(clk_id, tp);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(clock_getres, [](FEXCore::Core::CpuStateFrame *Frame, clockid_t clk_id, struct timespec *tp) -> uint64_t {
      uint64_t Result = ::clock_getres(clk_id, tp);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(clock_nanosleep, [](FEXCore::Core::CpuStateFrame *Frame, clockid_t clockid, int flags, const struct timespec *request, struct timespec *remain) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(clock_nanosleep), clockid, flags, request, remain);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(clock_settime, [](FEXCore::Core::CpuStateFrame *Frame, clockid_t clockid, const struct timespec *tp) -> uint64_t {
      uint64_t Result = ::clock_settime(clockid, tp);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(settimeofday, [](FEXCore::Core::CpuStateFrame *Frame, const struct timeval *tv, const struct timezone *tz) -> uint64_t {
      uint64_t Result = ::settimeofday(tv, tz);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(utimes, [](FEXCore::Core::CpuStateFrame *Frame, const char *filename, const struct timeval times[2]) -> uint64_t {
      uint64_t Result = ::utimes(filename, times);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(getitimer, [](FEXCore::Core::CpuStateFrame *Frame, int which, struct itimerval *curr_value) -> uint64_t {
      uint64_t Result = ::getitimer(which, curr_value);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(setitimer, [](FEXCore::Core::CpuStateFrame *Frame, int which, const struct itimerval *new_value, struct itimerval *old_value) -> uint64_t {
      uint64_t Result = ::setitimer(which, new_value, old_value);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(timer_settime, [](FEXCore::Core::CpuStateFrame *Frame, kernel_timer_t timerid, int flags, const struct itimerspec *new_value, struct itimerspec *old_value) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(timer_settime), timerid, flags, new_value, old_value);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(timer_gettime, [](FEXCore::Core::CpuStateFrame *Frame, kernel_timer_t timerid, struct itimerspec *curr_value) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(timer_gettime), timerid, curr_value);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(adjtimex, [](FEXCore::Core::CpuStateFrame *Frame, struct timex *buf) -> uint64_t {
      uint64_t Result = ::adjtimex(buf);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(clock_adjtime, [](FEXCore::Core::CpuStateFrame *Frame, clockid_t clk_id, struct timex *buf) -> uint64_t {
      uint64_t Result = ::clock_adjtime(clk_id, buf);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(timer_create, [](FEXCore::Core::CpuStateFrame *Frame, clockid_t clockid, struct sigevent *sevp, kernel_timer_t *timerid) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(timer_create), clockid, sevp, timerid);
      SYSCALL_ERRNO();
    });
  }
}

