#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x32/FD.h"
#include "Interface/Context/Context.h"

#include <fcntl.h>
#include <stdint.h>
#include <sys/file.h>
#include <sys/eventfd.h>
#include <sys/inotify.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/timerfd.h>
#include <sys/uio.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE::x32 {
  uint32_t Writev(FEXCore::Core::InternalThreadState *Thread, int fd, const struct iovec_32 *iov, int iovcnt) {
    std::vector<iovec> Host_iovec(iovcnt);
    for (int i = 0; i < iovcnt; ++i) {
      Host_iovec[i].iov_base = reinterpret_cast<void*>(iov[i].iov_base);
      Host_iovec[i].iov_len = iov[i].iov_len;
    }
    uint64_t Result = ::writev(fd, &Host_iovec.at(0), iovcnt);
    SYSCALL_ERRNO();
  }
}
