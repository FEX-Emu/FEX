#include "Interface/HLE/Syscalls.h"

#include <stdint.h>
#include <sys/epoll.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {
  uint64_t EPoll_Ctl(FEXCore::Core::InternalThreadState *Thread, int epfd, int op, int fd, void *event) {
    uint64_t Result = epoll_ctl(epfd, op, fd, reinterpret_cast<struct epoll_event*>(event));
    SYSCALL_ERRNO();
  }

  uint64_t EPoll_Pwait(FEXCore::Core::InternalThreadState *Thread, int epfd, void *events, int maxevent, int timeout, const void* sigmask) {
    uint64_t Result = epoll_pwait(
      epfd,
      reinterpret_cast<struct epoll_event*>(events),
      maxevent,
      timeout,
      reinterpret_cast<const sigset_t*>(sigmask));
    SYSCALL_ERRNO();
  }

  uint64_t EPoll_Create1(FEXCore::Core::InternalThreadState *Thread, int flags) {
    uint64_t Result = epoll_create1(flags);
    SYSCALL_ERRNO();
  }
}
