// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/Utils/CompilerDefs.h>

#ifndef ENABLE_JEMALLOC
#include <stdlib.h>
#include <malloc.h>
#endif

#ifdef _WIN32
#define NTDDI_VERSION 0x0A000005
#include <memoryapi.h>
#else
#include <sys/mman.h>
#endif

#include <new>
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
#ifdef _WIN32
  inline void *VirtualAlloc(void* Base, size_t Size, bool Execute = false) {
#ifdef _M_ARM_64EC
    MEM_EXTENDED_PARAMETER Parameter{};
    if (Execute) {
      Parameter.Type = MemExtendedParameterAttributeFlags;
      Parameter.ULong64 = MEM_EXTENDED_PARAMETER_EC_CODE;
    };
    return ::VirtualAlloc2(nullptr, Base, Size, MEM_COMMIT | (Base ? MEM_RESERVE : 0), Execute ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE,
                          &Parameter, Execute ? 1 : 0);
#else
    return ::VirtualAlloc(Base, Size, MEM_COMMIT | (Base ? MEM_RESERVE : 0), Execute ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE);
#endif
  }

  inline void *VirtualAlloc(size_t Size, bool Execute = false) {
    return VirtualAlloc(nullptr, Size, Execute);
  }

  inline void VirtualFree(void *Ptr, size_t Size) {
    ::VirtualFree(Ptr, Size, MEM_RELEASE);
  }
  inline void VirtualDontNeed(void *Ptr, size_t Size) {
    // Match madvise behaviour as best as we can here.
    // Protections are ignored but still required to be valid.
    ::VirtualAlloc(Ptr, Size, MEM_RESET, PAGE_NOACCESS);
  }

#else
  using MMAP_Hook = void*(*)(void*, size_t, int, int, int, off_t);
  using MUNMAP_Hook = int(*)(void*, size_t);

  FEX_DEFAULT_VISIBILITY extern MMAP_Hook mmap;
  FEX_DEFAULT_VISIBILITY extern MUNMAP_Hook munmap;

  inline void *VirtualAlloc(size_t Size, bool Execute = false) {
    return FEXCore::Allocator::mmap(nullptr, Size, PROT_READ | PROT_WRITE | (Execute ? PROT_EXEC : 0), MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  }

  inline void *VirtualAlloc(void *Base, size_t Size, bool Execute = false) {
    return FEXCore::Allocator::mmap(Base, Size, PROT_READ | PROT_WRITE | (Execute ? PROT_EXEC : 0), MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  }

  inline void VirtualFree(void *Ptr, size_t Size) {
    FEXCore::Allocator::munmap(Ptr, Size);
  }
  inline void VirtualDontNeed(void *Ptr, size_t Size) {
    ::madvise(reinterpret_cast<void*>(Ptr), Size, MADV_DONTNEED);
  }
#endif

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
#elif defined(_WIN32)
  inline void *malloc(size_t size) { return ::malloc(size); }
  inline void *calloc(size_t n, size_t size) { return ::calloc(n, size); }
  inline void *memalign(size_t align, size_t s) { return ::_aligned_malloc(s, align); }
  inline void *valloc(size_t size)
  {
    return ::_aligned_malloc(size, 4096);
  }
  inline int posix_memalign(void** r, size_t a, size_t s) {
    void* ptr = _aligned_malloc(s, a);
    if (ptr) {
      *r = ptr;
    }
    return errno;
  }
  inline void *realloc(void* ptr, size_t size) { return ::realloc(ptr, size); }
  inline void free(void* ptr) { return ::free(ptr); }
  inline size_t malloc_usable_size(void *ptr) { return ::_msize(ptr); }
  inline void *aligned_alloc(size_t a, size_t s) { return ::_aligned_malloc(s, a); }
  inline void aligned_free(void* ptr) { return ::_aligned_free(ptr); }
#else
  inline void *malloc(size_t size) { return ::malloc(size); }
  inline void *calloc(size_t n, size_t size) { return ::calloc(n, size); }
  inline void *memalign(size_t align, size_t s) { return ::memalign(align, s); }
  inline void *valloc(size_t size)
  {
#ifdef __ANDROID__
    return ::aligned_alloc(4096, size);
#else
    return ::valloc(size);
#endif
  }
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

    void *operator new(size_t size, std::align_val_t align) {
      return FEXCore::Allocator::aligned_alloc(static_cast<size_t>(align), size);
    }

    void operator delete(void *ptr) {
      return FEXCore::Allocator::free(ptr);
    }

    void operator delete(void *ptr, std::align_val_t align) {
      return FEXCore::Allocator::aligned_free(ptr);
    }
  };
}
