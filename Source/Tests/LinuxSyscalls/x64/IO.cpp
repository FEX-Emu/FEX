/*
$info$
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"

#include <linux/aio_abi.h>
#include <stdint.h>
#include <syscall.h>
#include <unistd.h>

namespace FEXCore::Core {
  struct CpuStateFrame;
}

namespace FEX::HLE::x64 {
  void RegisterIO() {
    REGISTER_SYSCALL_IMPL_X64_PASS(io_getevents, [](FEXCore::Core::CpuStateFrame *Frame, aio_context_t ctx_id, long min_nr, long nr, struct io_event *events, struct timespec *timeout) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(io_getevents), ctx_id, min_nr, nr, events, timeout);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(io_pgetevents, [](FEXCore::Core::CpuStateFrame *Frame, aio_context_t ctx_id, long min_nr, long nr, struct io_event *events, struct timespec *timeout, const struct io_sigset  *usig) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(io_pgetevents), ctx_id, min_nr, nr, events, timeout);
      SYSCALL_ERRNO();
    });
  }
}
