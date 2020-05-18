#pragma once

#include "Interface/HLE/Syscalls.h"

#include <poll.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE::x32 {
  struct iovec_32 {
    uint32_t iov_base;
    uint32_t iov_len;
  };

  uint32_t Writev(FEXCore::Core::InternalThreadState *Thread, int fd, const struct iovec_32 *iov, int iovcnt);
}
