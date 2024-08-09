// SPDX-License-Identifier: MIT
#define _SECIMP
#define _CRTIMP
#include <cstdint>
#include "../Priv.h"

extern "C" {

#define JEMALLOC_NOTHROW __attribute__((nothrow))
JEMALLOC_NOTHROW extern void* je_malloc(size_t size);
JEMALLOC_NOTHROW extern void* je_calloc(size_t n, size_t size);
JEMALLOC_NOTHROW extern void* je_memalign(size_t align, size_t s);
JEMALLOC_NOTHROW extern void* je_valloc(size_t size);
JEMALLOC_NOTHROW extern int je_posix_memalign(void** r, size_t a, size_t s);
JEMALLOC_NOTHROW extern void* je_realloc(void* ptr, size_t size);
JEMALLOC_NOTHROW extern void je_free(void* ptr);
JEMALLOC_NOTHROW extern size_t je_malloc_usable_size(void* ptr);
JEMALLOC_NOTHROW extern void* je_aligned_alloc(size_t a, size_t s);
#undef JEMALLOC_NOTHROW
}

void* calloc(size_t NumOfElements, size_t SizeOfElements) {
  return je_calloc(NumOfElements, SizeOfElements);
}

void free(void* Memory) {
  je_free(Memory);
}

void* malloc(size_t Size) {
  return je_malloc(Size);
}

void* realloc(void* Memory, size_t NewSize) {
  return je_realloc(Memory, NewSize);
}

DLLEXPORT_FUNC(void*, _aligned_malloc, (size_t Size, size_t Alignment)) {
  return je_aligned_alloc(Alignment, Size);
}

DLLEXPORT_FUNC(void, _aligned_free, (void* Memory)) {
  je_free(Memory);
}
