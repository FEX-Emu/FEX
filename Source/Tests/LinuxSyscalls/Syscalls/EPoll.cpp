#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"

#include <stdint.h>
#include <sys/epoll.h>

namespace FEX::HLE {
  void RegisterEpoll() {

    REGISTER_SYSCALL_IMPL(epoll_create, [](FEXCore::Core::InternalThreadState *Thread, int size) -> uint64_t {
      uint64_t Result = epoll_create(size);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(epoll_create1, [](FEXCore::Core::InternalThreadState *Thread, int flags) -> uint64_t {
      uint64_t Result = epoll_create1(flags);
      SYSCALL_ERRNO();
    });
  }
}
