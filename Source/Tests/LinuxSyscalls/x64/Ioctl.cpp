#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"

#include <stddef.h>
#include <stdint.h>
#include <sys/ioctl.h>

namespace FEX::HLE::x64 {
  void RegisterIoctl() {
    REGISTER_SYSCALL_IMPL_X64(ioctl, [](FEXCore::Core::InternalThreadState *Thread, int fd, uint64_t request, void *args) -> uint64_t {
      uint64_t Result = ::ioctl(fd, request, args);
      SYSCALL_ERRNO();
    });
  }
}
