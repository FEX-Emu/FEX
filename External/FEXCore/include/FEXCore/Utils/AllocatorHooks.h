#pragma once
#include <FEXCore/Utils/CompilerDefs.h>

#ifndef ENABLE_JEMALLOC
#include <stdlib.h>
#include <malloc.h>
#endif

#include <cstddef>
#include <cstdint>
#include <sys/types.h>

extern "C" {
// jemalloc defines nothrow on its internal C function signatures.
#ifdef ENABLE_JEMALLOC
#define JEMALLOC_NOTHROW __attribute__((nothrow))
  // Forward declare jemalloc functions so we don't need to pull in the jemalloc header in to the public API.
  FEX_DEFAULT_VISIBILITY JEMALLOC_NOTHROW extern void *je_malloc(size_t size);
  FEX_DEFAULT_VISIBILITY JEMALLOC_NOTHROW extern void *je_calloc(size_t n, size_t size);
  FEX_DEFAULT_VISIBILITY JEMALLOC_NOTHROW extern void *je_memalign(size_t align, size_t s);
  FEX_DEFAULT_VISIBILITY JEMALLOC_NOTHROW extern void *je_valloc(size_t size);
  FEX_DEFAULT_VISIBILITY JEMALLOC_NOTHROW extern int je_posix_memalign(void** r, size_t a, size_t s);
  FEX_DEFAULT_VISIBILITY JEMALLOC_NOTHROW extern void *je_realloc(void* ptr, size_t size);
  FEX_DEFAULT_VISIBILITY JEMALLOC_NOTHROW extern void je_free(void* ptr);
  FEX_DEFAULT_VISIBILITY JEMALLOC_NOTHROW extern size_t je_malloc_usable_size(void *ptr);
  FEX_DEFAULT_VISIBILITY JEMALLOC_NOTHROW extern void *je_aligned_alloc(size_t a, size_t s);
#undef JEMALLOC_NOTHROW
#endif
}

namespace FEXCore::Allocator {
  using MMAP_Hook = void*(*)(void*, size_t, int, int, int, off_t);
  using MUNMAP_Hook = int(*)(void*, size_t);

  FEX_DEFAULT_VISIBILITY extern MMAP_Hook mmap;
  FEX_DEFAULT_VISIBILITY extern MUNMAP_Hook munmap;

  // Memory allocation routines aliased to jemalloc functions.
#ifdef ENABLE_JEMALLOC
  inline void *malloc(size_t size) { return ::je_malloc(size); }
  inline void *calloc(size_t n, size_t size) { return ::je_calloc(n, size); }
  inline void *memalign(size_t align, size_t s) { return ::je_memalign(align, s); }
  inline void *valloc(size_t size) { return ::je_valloc(size); }
  inline int posix_memalign(void** r, size_t a, size_t s) { return ::je_posix_memalign(r, a, s); }
  inline void *realloc(void* ptr, size_t size) { return ::je_realloc(ptr, size); }
  inline void free(void* ptr) { return ::je_free(ptr); }
  inline size_t malloc_usable_size(void *ptr) { return ::je_malloc_usable_size(ptr); }
  inline void *aligned_alloc(size_t a, size_t s) { return ::je_aligned_alloc(a, s); }
  inline void aligned_free(void* ptr) { return ::je_free(ptr); }
#else
  inline void *malloc(size_t size) { return ::malloc(size); }
  inline void *calloc(size_t n, size_t size) { return ::calloc(n, size); }
  inline void *memalign(size_t align, size_t s) { return ::memalign(align, s); }
  inline void *valloc(size_t size) { return ::valloc(size); }
  inline int posix_memalign(void** r, size_t a, size_t s) { return ::posix_memalign(r, a, s); }
  inline void *realloc(void* ptr, size_t size) { return ::realloc(ptr, size); }
  inline void free(void* ptr) { return ::free(ptr); }
  inline size_t malloc_usable_size(void *ptr) { return ::malloc_usable_size(ptr); }
  inline void *aligned_alloc(size_t a, size_t s) { return ::aligned_alloc(a, s); }
  inline void aligned_free(void* ptr) { return ::free(ptr); }
#endif

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
