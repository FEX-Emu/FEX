/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"

#include <linux/aio_abi.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace FEX::HLE {
  void RegisterIO() {
    REGISTER_SYSCALL_IMPL_PASS(iopl, [](FEXCore::Core::CpuStateFrame *Frame, int level) -> uint64_t {
      // Just claim we don't have permission
      return -EPERM;
    });

    REGISTER_SYSCALL_IMPL_PASS(ioperm, [](FEXCore::Core::CpuStateFrame *Frame, unsigned long from, unsigned long num, int turn_on) -> uint64_t {
      // ioperm not available on our architecture
      return -EPERM;
    });

    REGISTER_SYSCALL_IMPL_PASS(io_setup, [](FEXCore::Core::CpuStateFrame *Frame, unsigned nr_events, aio_context_t *ctx_idp) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(io_setup), nr_events, ctx_idp);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(io_destroy, [](FEXCore::Core::CpuStateFrame *Frame, aio_context_t ctx_id) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(io_destroy), ctx_id);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(io_submit, [](FEXCore::Core::CpuStateFrame *Frame, aio_context_t ctx_id, long nr, struct iocb **iocbpp) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(io_submit), ctx_id, nr, iocbpp);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(io_cancel, [](FEXCore::Core::CpuStateFrame *Frame, aio_context_t ctx_id, struct iocb *iocb, struct io_event *result) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(io_cancel), ctx_id, iocb, result);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(ioprio_set, [](FEXCore::Core::CpuStateFrame *Frame, int which, int who) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(ioprio_set), which, who);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(ioprio_get, [](FEXCore::Core::CpuStateFrame *Frame, int which, int who, int ioprio) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(ioprio_get), which, who, ioprio);
      SYSCALL_ERRNO();
    });
  }
}
