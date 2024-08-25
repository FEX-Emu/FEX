// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/Types.h"
#include "LinuxSyscalls/x64/Syscalls.h"

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
void RegisterTime(FEX::HLE::SyscallHandler* Handler) {
  using namespace FEXCore::IR;
  REGISTER_SYSCALL_IMPL_X64_FLAGS(time, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                  [](FEXCore::Core::CpuStateFrame* Frame, time_t* tloc) -> uint64_t {
                                    FaultSafeUserMemAccess::VerifyIsWritableOrNull(tloc, sizeof(time_t));
                                    uint64_t Result = ::time(tloc);
                                    SYSCALL_ERRNO();
                                  });

  REGISTER_SYSCALL_IMPL_X64(utime, [](FEXCore::Core::CpuStateFrame* Frame, const char* filename, const struct utimbuf* times) -> uint64_t {
    FaultSafeUserMemAccess::VerifyIsStringReadable(filename);
    FaultSafeUserMemAccess::VerifyIsReadableOrNull(times, sizeof(utimbuf));
    uint64_t Result = ::utime(filename, times);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X64(utimes, [](FEXCore::Core::CpuStateFrame* Frame, const char* filename, const struct timeval times[2]) -> uint64_t {
    FaultSafeUserMemAccess::VerifyIsStringReadable(filename);
    FaultSafeUserMemAccess::VerifyIsReadableOrNull(times, sizeof(timeval) * 2);
    uint64_t Result = ::utimes(filename, times);
    SYSCALL_ERRNO();
  });
}
} // namespace FEX::HLE::x64
