#include "Utils/Allocator/HostAllocator.h"
#include <FEXCore/Utils/Allocator.h>
#include <sys/mman.h>
#include <jemalloc/jemalloc.h>
#include <memory>
#include <malloc.h>

extern "C" {
  typedef void* (*mmap_hook_type)(
            void *addr, size_t length, int prot, int flags,
            int fd, off_t offset);
  typedef int (*munmap_hook_type)(void *addr, size_t length);

  extern mmap_hook_type __mmap_hook;
  extern munmap_hook_type __munmap_hook;
}

namespace FEXCore::Allocator {
  MMAP_Hook mmap {::mmap};
  MUNMAP_Hook munmap {::munmap};
  MALLOC_Hook malloc {::je_malloc};
  REALLOC_Hook realloc {::je_realloc};
  FREE_Hook free {::je_free};

  using GLIBC_MALLOC_Hook = void*(*)(size_t, const void *caller);
  using GLIBC_REALLOC_Hook = void*(*)(void*, size_t, const void *caller);
  using GLIBC_FREE_Hook = void(*)(void*, const void *caller);

  std::unique_ptr<Alloc::HostAllocator> Alloc64{};

  void *FEX_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    void *Result = Alloc64->Mmap(addr, length, prot, flags, fd, offset);
    if (Result >= (void*)-4096) {
      errno = -(uint64_t)Result;
      return (void*)-1;
    }
    return Result;
  }
  int FEX_munmap(void *addr, size_t length) {
    int Result = Alloc64->Munmap(addr, length);

    if (Result != 0) {
      errno = -Result;
      return -1;
    }
    return Result;
  }

  void *FEX_malloc_hook(size_t size, const void *caller) {
    return ::je_malloc(size);
  }

  void *FEX_realloc_hook(void *ptr, size_t size, const void *caller) {
    return ::je_realloc(ptr, size);
  }

  void FEX_free_hook(void *ptr, const void *caller) {
    return ::je_free(ptr);
  }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  void SetupHooks() {
    Alloc64 = Alloc::OSAllocator::Create64BitAllocator();
    __mmap_hook   = FEX_mmap;
    __munmap_hook = FEX_munmap;
    FEXCore::Allocator::mmap = FEX_mmap;
    FEXCore::Allocator::munmap = FEX_munmap;
  }

  void ClearHooks() {
    __mmap_hook   = ::mmap;
    __munmap_hook = ::munmap;
    FEXCore::Allocator::mmap = ::mmap;
    FEXCore::Allocator::munmap = ::munmap;
  }
#pragma GCC diagnostic pop

}

extern "C" {
}
