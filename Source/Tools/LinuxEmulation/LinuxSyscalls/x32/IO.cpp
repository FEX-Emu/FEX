// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x64/Syscalls.h"
#include "LinuxSyscalls/x32/Syscalls.h"
#include "LinuxSyscalls/x32/Types.h"

#include <linux/aio_abi.h>
#include <stdint.h>
#include <syscall.h>
#include <unistd.h>

namespace FEX::HLE::x32 {
void RegisterIO(FEX::HLE::SyscallHandler* Handler) {
  REGISTER_SYSCALL_IMPL_X32(io_getevents,
                            [](FEXCore::Core::CpuStateFrame* Frame, aio_context_t ctx_id, long min_nr, long nr, struct io_event* events,
                               struct timespec32* timeout) -> uint64_t {
                              struct timespec* timeout_ptr {};
                              struct timespec tp64 {};
                              if (timeout) {
                                FaultSafeUserMemAccess::VerifyIsReadable(timeout, sizeof(*timeout));
                                tp64 = *timeout;
                                timeout_ptr = &tp64;
                              }

                              uint64_t Result = ::syscall(SYSCALL_DEF(io_getevents), ctx_id, min_nr, nr, events, timeout_ptr);
                              SYSCALL_ERRNO();
                            });

  REGISTER_SYSCALL_IMPL_X32(io_pgetevents,
                            [](FEXCore::Core::CpuStateFrame* Frame, aio_context_t ctx_id, long min_nr, long nr, struct io_event* events,
                               struct timespec32* timeout, const struct io_sigset* usig) -> uint64_t {
                              struct timespec* timeout_ptr {};
                              struct timespec tp64 {};
                              if (timeout) {
                                FaultSafeUserMemAccess::VerifyIsReadable(timeout, sizeof(*timeout));
                                tp64 = *timeout;
                                timeout_ptr = &tp64;
                              }

                              uint64_t Result = ::syscall(SYSCALL_DEF(io_pgetevents), ctx_id, min_nr, nr, events, timeout_ptr, usig);
                              SYSCALL_ERRNO();
                            });
}
} // namespace FEX::HLE::x32
