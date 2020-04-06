#pragma once
#include <stddef.h>
#include <stdint.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {
  uint64_t Ioctl(FEXCore::Core::InternalThreadState *Thread, int fd, uint64_t request, void *args);
}
