// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/fextl/allocator.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/vector.h>
#include <FEXCore/Utils/Allocator.h>

#include <cstddef>
#include <sys/types.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace Alloc {
// HostAllocator is just a page pased slab allocator
// Similar to mmap and munmap only mapping at the page level
class HostAllocator {
public:
  HostAllocator() = default;
  virtual ~HostAllocator() = default;
  virtual void* AllocateSlab(size_t Size) = 0;
  virtual void DeallocateSlab(void* Ptr, size_t Size) = 0;

  virtual void* Mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset) {
    return nullptr;
  }
  virtual int Munmap(void* addr, size_t length) {
    return -1;
  }

  virtual void LockBeforeFork(FEXCore::Core::InternalThreadState* Thread) {}
  virtual void UnlockAfterFork(FEXCore::Core::InternalThreadState* Thread, bool Child) {}
};

class GlobalAllocator {
public:
  HostAllocator* Alloc {};
  GlobalAllocator(HostAllocator* _Alloc)
    : Alloc {_Alloc} {}

  virtual ~GlobalAllocator() = default;
  virtual void* malloc(size_t Size) = 0;
  virtual void* calloc(size_t num, size_t size) = 0;
  virtual void* realloc(void* ptr, size_t size) = 0;
  virtual void* memalign(size_t alignment, size_t size) = 0;
  virtual void free(void* ptr) = 0;
};
} // namespace Alloc

namespace Alloc::OSAllocator {
fextl::unique_ptr<Alloc::HostAllocator> Create64BitAllocator();
fextl::unique_ptr<Alloc::HostAllocator> Create64BitAllocatorWithRegions(fextl::vector<FEXCore::Allocator::MemoryRegion>& Regions);
} // namespace Alloc::OSAllocator
