#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"
#include "Interface/HLE/x32/Syscalls.h"

#include <linux/aio_abi.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {

  void RegisterIO() {
    REGISTER_SYSCALL_IMPL(iopl, [](FEXCore::Core::InternalThreadState *Thread, int level) -> uint64_t {
      // Just claim we don't have permission
      return -EPERM;
    });

    REGISTER_SYSCALL_IMPL(ioperm, [](FEXCore::Core::InternalThreadState *Thread, unsigned long from, unsigned long num, int turn_on) -> uint64_t {
      // ioperm not available on our architecture
      return -EPERM;
    });

    REGISTER_SYSCALL_IMPL(io_setup, [](FEXCore::Core::InternalThreadState *Thread, unsigned nr_events, aio_context_t *ctx_idp) -> uint64_t {
      uint64_t Result = ::syscall(SYS_io_setup, nr_events, ctx_idp);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(io_destroy, [](FEXCore::Core::InternalThreadState *Thread, aio_context_t ctx_id) -> uint64_t {
      uint64_t Result = ::syscall(SYS_io_destroy, ctx_id);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(io_getevents, [](FEXCore::Core::InternalThreadState *Thread, aio_context_t ctx_id, long min_nr, long nr, struct io_event *events, struct timespec *timeout) -> uint64_t {
      uint64_t Result = ::syscall(SYS_io_getevents, ctx_id, min_nr, nr, events, timeout);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(io_submit, [](FEXCore::Core::InternalThreadState *Thread, aio_context_t ctx_id, long nr, struct iocb **iocbpp) -> uint64_t {
      uint64_t Result = ::syscall(SYS_io_submit, ctx_id, nr, iocbpp);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(io_cancel, [](FEXCore::Core::InternalThreadState *Thread, aio_context_t ctx_id, struct iocb *iocb, struct io_event *result) -> uint64_t {
      uint64_t Result = ::syscall(SYS_io_cancel, ctx_id, iocb, result);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(ioprio_set, [](FEXCore::Core::InternalThreadState *Thread, int which, int who) -> uint64_t {
      uint64_t Result = ::syscall(SYS_ioprio_set, which, who);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(ioprio_get, [](FEXCore::Core::InternalThreadState *Thread, int which, int who, int ioprio) -> uint64_t {
      uint64_t Result = ::syscall(SYS_ioprio_get, which, who, ioprio);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(io_pgetevents, [](FEXCore::Core::InternalThreadState *Thread, aio_context_t ctx_id, long min_nr, long nr, struct io_event *events, struct timespec *timeout, const struct io_sigset  *usig) -> uint64_t {
      uint64_t Result = ::syscall(SYS_io_pgetevents, ctx_id, min_nr, nr, events, timeout);
      SYSCALL_ERRNO();
    });
  }
}
