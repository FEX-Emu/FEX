#pragma once
#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {
  uint64_t Mmap(FEXCore::Core::InternalThreadState *Thread, void *addr, size_t length, int prot, int flags, int fd, off_t offset);
  uint64_t Mprotect(FEXCore::Core::InternalThreadState *Thread, void *addr, size_t len, int prot);
  uint64_t Munmap(FEXCore::Core::InternalThreadState *Thread, void *addr, size_t length);
  uint64_t Brk(FEXCore::Core::InternalThreadState *Thread, void *addr);
  uint64_t Mremap(FEXCore::Core::InternalThreadState *Thread, void *old_address, size_t old_size, size_t new_size, int flags, void *new_address);
  uint64_t Msync(FEXCore::Core::InternalThreadState *Thread, void *addr, size_t length, int32_t flags);
  uint64_t Mincore(FEXCore::Core::InternalThreadState *Thread, void *addr, size_t length, uint8_t *vec);
  uint64_t Madvise(FEXCore::Core::InternalThreadState *Thread, void *addr, size_t length, int32_t advice);
}
