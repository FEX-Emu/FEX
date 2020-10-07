#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"
#include "Interface/HLE/x32/Syscalls.h"

#include <stddef.h>
#include <stdint.h>
#include <sys/ioctl.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {
  void RegisterIoctl() {
    REGISTER_SYSCALL_IMPL(ioctl, [](FEXCore::Core::InternalThreadState *Thread, int fd, uint64_t request, void *args) -> uint64_t {
      uint64_t Result = ::ioctl(fd, request, args);
      SYSCALL_ERRNO();
    });
  }
}
