#include "Utils/Allocator/HostAllocator.h"
#include <FEXCore/Utils/Allocator.h>
#include <sys/mman.h>
#ifdef ENABLE_JEMALLOC
#include <jemalloc/jemalloc.h>
#endif
#include <memory>
#include <malloc.h>

extern "C" {
  typedef void* (*mmap_hook_type)(
            void *addr, size_t length, int prot, int flags,
            int fd, off_t offset);
  typedef int (*munmap_hook_type)(void *addr, size_t length);

#ifdef ENABLE_JEMALLOC
  extern mmap_hook_type __mmap_hook;
  extern munmap_hook_type __munmap_hook;
#endif
}

namespace FEXCore::Allocator {
  MMAP_Hook mmap {::mmap};
  MUNMAP_Hook munmap {::munmap};
#ifdef ENABLE_JEMALLOC
  MALLOC_Hook malloc {::je_malloc};
  REALLOC_Hook realloc {::je_realloc};
  FREE_Hook free {::je_free};
#else
  MALLOC_Hook malloc {::malloc};
  REALLOC_Hook realloc {::realloc};
  FREE_Hook free {::free};
#endif

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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  void SetupHooks() {
    Alloc64 = Alloc::OSAllocator::Create64BitAllocator();
#ifdef ENABLE_JEMALLOC
    __mmap_hook   = FEX_mmap;
    __munmap_hook = FEX_munmap;
#endif
    FEXCore::Allocator::mmap = FEX_mmap;
    FEXCore::Allocator::munmap = FEX_munmap;
  }

  void ClearHooks() {
#ifdef ENABLE_JEMALLOC
    __mmap_hook   = ::mmap;
    __munmap_hook = ::munmap;
#endif
    FEXCore::Allocator::mmap = ::mmap;
    FEXCore::Allocator::munmap = ::munmap;

    // XXX: This is currently a leak.
    // We can't work around this yet until static initializers that allocate memory are completely removed from our codebase
    // Luckily we only remove this on process shutdown, so the kernel will do the cleanup for us
    Alloc64.release();
  }
#pragma GCC diagnostic pop

}
