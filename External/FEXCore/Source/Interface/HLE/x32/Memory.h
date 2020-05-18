#pragma once
#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE::x32 {
  uint32_t Mmap(FEXCore::Core::InternalThreadState *Thread, void *addr, size_t length, int prot, int flags, int fd, off_t offset);
}
