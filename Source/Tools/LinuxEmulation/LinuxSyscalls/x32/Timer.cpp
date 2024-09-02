// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/Types.h"
#include "LinuxSyscalls/x32/Syscalls.h"
#include "LinuxSyscalls/x32/Types.h"

#include "LinuxSyscalls/x64/Syscalls.h"

#include <stdint.h>
#include <syscall.h>
#include <sys/time.h>
#include <unistd.h>

namespace FEXCore::Core {
struct CpuStateFrame;
}

ARG_TO_STR(FEX::HLE::x32::compat_ptr<FEX::HLE::x32::sigevent32>, "%lx")

namespace FEX::HLE::x32 {
void RegisterTimer(FEX::HLE::SyscallHandler* Handler) {
  REGISTER_SYSCALL_IMPL_X32(timer_settime,
                            [](FEXCore::Core::CpuStateFrame* Frame, kernel_timer_t timerid, int flags,
                               const FEX::HLE::x32::old_itimerspec32* new_value, FEX::HLE::x32::old_itimerspec32* old_value) -> uint64_t {
                              itimerspec new_value_host {};
                              itimerspec old_value_host {};
                              itimerspec* old_value_host_p {};

                              FaultSafeUserMemAccess::VerifyIsReadable(new_value, sizeof(*new_value));
                              new_value_host = *new_value;
                              if (old_value) {
                                old_value_host_p = &old_value_host;
                              }
                              uint64_t Result = ::syscall(SYSCALL_DEF(timer_settime), timerid, flags, &new_value_host, old_value_host_p);
                              if (Result != -1 && old_value) {
                                FaultSafeUserMemAccess::VerifyIsWritable(old_value, sizeof(*old_value));
                                *old_value = old_value_host;
                              }
                              SYSCALL_ERRNO();
                            });

  REGISTER_SYSCALL_IMPL_X32(
    timer_gettime, [](FEXCore::Core::CpuStateFrame* Frame, kernel_timer_t timerid, FEX::HLE::x32::old_itimerspec32* curr_value) -> uint64_t {
      itimerspec curr_value_host {};
      uint64_t Result = ::syscall(SYSCALL_DEF(timer_gettime), timerid, curr_value_host);
      FaultSafeUserMemAccess::VerifyIsWritable(curr_value, sizeof(*curr_value));
      *curr_value = curr_value_host;
      SYSCALL_ERRNO();
    });

  REGISTER_SYSCALL_IMPL_X32(getitimer, [](FEXCore::Core::CpuStateFrame* Frame, int which, FEX::HLE::x32::itimerval32* curr_value) -> uint64_t {
    itimerval val {};
    itimerval* val_p {};
    if (curr_value) {
      val_p = &val;
    }
    uint64_t Result = ::getitimer(which, val_p);
    if (curr_value) {
      FaultSafeUserMemAccess::VerifyIsWritable(curr_value, sizeof(*curr_value));
      *curr_value = val;
    }
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(setitimer,
                            [](FEXCore::Core::CpuStateFrame* Frame, int which, const FEX::HLE::x32::itimerval32* new_value,
                               FEX::HLE::x32::itimerval32* old_value) -> uint64_t {
                              itimerval val {};
                              itimerval old {};
                              itimerval* val_p {};
                              itimerval* old_p {};

                              if (new_value) {
                                FaultSafeUserMemAccess::VerifyIsReadable(new_value, sizeof(*new_value));
                                val = *new_value;
                                val_p = &val;
                              }

                              if (old_value) {
                                old_p = &old;
                              }

                              uint64_t Result = ::setitimer(which, val_p, old_p);

                              if (old_value) {
                                FaultSafeUserMemAccess::VerifyIsWritable(old_value, sizeof(*old_value));
                                *old_value = old;
                              }
                              SYSCALL_ERRNO();
                            });

  REGISTER_SYSCALL_IMPL_X32(
    timer_create,
    [](FEXCore::Core::CpuStateFrame* Frame, clockid_t clockid, compat_ptr<FEX::HLE::x32::sigevent32> sevp, kernel_timer_t* timerid) -> uint64_t {
      FaultSafeUserMemAccess::VerifyIsReadable(sevp, sizeof(*sevp));
      sigevent Host = *sevp;
      uint64_t Result = ::syscall(SYSCALL_DEF(timer_create), clockid, &Host, timerid);
      SYSCALL_ERRNO();
    });
}
} // namespace FEX::HLE::x32
