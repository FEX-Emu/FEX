#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"
#include "Interface/HLE/x32/Syscalls.h"

#include <stdint.h>
#include <sys/epoll.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {

  void RegisterEpoll() {

    REGISTER_SYSCALL_IMPL(epoll_create, [](FEXCore::Core::InternalThreadState *Thread, int size) -> uint64_t {
      uint64_t Result = epoll_create(size);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(epoll_wait, [](FEXCore::Core::InternalThreadState *Thread, int epfd, void *events, int maxevents, int timeout) -> uint64_t {
      uint64_t Result = epoll_wait(epfd, reinterpret_cast<struct epoll_event*>(events), maxevents, timeout);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(epoll_ctl, [](FEXCore::Core::InternalThreadState *Thread, int epfd, int op, int fd, void *event) -> uint64_t {
      uint64_t Result = epoll_ctl(epfd, op, fd, reinterpret_cast<struct epoll_event*>(event));
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(epoll_pwait, [](FEXCore::Core::InternalThreadState *Thread, int epfd, void *events, int maxevent, int timeout, const void* sigmask) -> uint64_t {
      uint64_t Result = epoll_pwait(
        epfd,
        reinterpret_cast<struct epoll_event*>(events),
        maxevent,
        timeout,
        reinterpret_cast<const sigset_t*>(sigmask));
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(epoll_create1, [](FEXCore::Core::InternalThreadState *Thread, int flags) -> uint64_t {
      uint64_t Result = epoll_create1(flags);
      SYSCALL_ERRNO();
    });
  }

 
}
