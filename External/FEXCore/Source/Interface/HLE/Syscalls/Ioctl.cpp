#include "Interface/HLE/Syscalls.h"

#include <stddef.h>
#include <stdint.h>
#include <sys/ioctl.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {
  uint64_t Ioctl(FEXCore::Core::InternalThreadState *Thread, int fd, uint64_t request, void *args) {
    uint64_t Result = ::ioctl(fd, request, args);
    SYSCALL_ERRNO();
  }
}
