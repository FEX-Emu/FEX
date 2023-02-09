
/*
$info$
tags: thunklibs|VDSO
desc: Linux VDSO thunking
$end_info$
*/

#include <stdio.h>
#include <cstring>

#include <sched.h>
#include <sys/time.h>
#include <time.h>

#include "Types.h"
#include "common/Guest.h"

#include "thunkgen_guest_libVDSO.inl"

extern "C" {
time_t __vdso_time(time_t *tloc) __attribute__((alias("fexfn_pack_time")));
int __vdso_gettimeofday(struct timeval *tv, struct timezone *tz) __attribute__((alias("fexfn_pack_gettimeofday")));
int __vdso_clock_gettime(clockid_t, struct timespec *) __attribute__((alias("fexfn_pack_clock_gettime")));
int __vdso_clock_getres(clockid_t, struct timespec *) __attribute__((alias("fexfn_pack_clock_getres")));
int __vdso_getcpu(uint32_t *, uint32_t *) __attribute__((alias("fexfn_pack_getcpu")));

#if __SIZEOF_POINTER__ == 4
int __vdso_clock_gettime64(clockid_t, struct timespec64 *) __attribute__((alias("fexfn_pack_clock_gettime64")));

__attribute__((naked))
int __kernel_vsyscall() {
  asm volatile(R"(
  .intel_syntax noprefix
  int 0x80;
  ret;
  .att_syntax prefix
  )"
  ::: "memory");
}

__attribute__((naked))
void __kernel_sigreturn() {
  asm volatile(R"(
  .intel_syntax noprefix
  pop eax;
  mov eax, 0x77;
  int 0x80;
  nop;
  .att_syntax prefix
  )"
  ::: "memory");
}
__attribute__((naked))
void __kernel_rt_sigreturn() {
  asm volatile(R"(
  .intel_syntax noprefix
  mov eax, 0xad;
  int 0x80;
  .att_syntax prefix
  )"
  ::: "memory");
}

#endif
}
