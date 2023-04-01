#pragma once
#include <FEXCore/Utils/CompilerDefs.h>

#include <cstddef>
#include <cstdint>
#include <sys/types.h>

namespace FEXCore::Allocator {
  using MMAP_Hook = void*(*)(void*, size_t, int, int, int, off_t);
  using MUNMAP_Hook = int(*)(void*, size_t);

  using MALLOC_Hook = void *(*)(size_t);
  using CALLOC_Hook = void *(*)(size_t, size_t);
  using MEMALIGN_Hook = void *(*)(size_t, size_t);
  using VALLOC_Hook = void *(*)(size_t);
  using POSIX_MEMALIGN_Hook = int (*)(void**, size_t, size_t);
  using REALLOC_Hook = void *(*)(void*, size_t);
  using FREE_Hook = void(*)(void*);
  using MALLOC_USABLE_SIZE_Hook = size_t (*)(void*);
  using ALIGNED_ALLOC_Hook = void *(*)(size_t, size_t);

  FEX_DEFAULT_VISIBILITY extern MMAP_Hook mmap;
  FEX_DEFAULT_VISIBILITY extern MUNMAP_Hook munmap;
  FEX_DEFAULT_VISIBILITY extern MALLOC_Hook malloc;
  FEX_DEFAULT_VISIBILITY extern CALLOC_Hook calloc;
  FEX_DEFAULT_VISIBILITY extern MEMALIGN_Hook memalign;
  FEX_DEFAULT_VISIBILITY extern VALLOC_Hook valloc;
  FEX_DEFAULT_VISIBILITY extern POSIX_MEMALIGN_Hook posix_memalign;
  FEX_DEFAULT_VISIBILITY extern REALLOC_Hook realloc;
  FEX_DEFAULT_VISIBILITY extern FREE_Hook free;
  FEX_DEFAULT_VISIBILITY extern MALLOC_USABLE_SIZE_Hook malloc_usable_size;
  FEX_DEFAULT_VISIBILITY extern ALIGNED_ALLOC_Hook aligned_alloc;

  struct FEXAllocOperators {
    FEXAllocOperators() = default;

    void *operator new(size_t size) {
      return FEXCore::Allocator::malloc(size);
    }

    void operator delete(void *ptr) {
      return FEXCore::Allocator::free(ptr);
    }
  };
}
