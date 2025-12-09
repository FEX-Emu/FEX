// SPDX-License-Identifier: MIT
#ifdef ENABLE_JEMALLOC
#include <jemalloc/jemalloc.h>
#endif

#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>

namespace FEXCore::Allocator {

#ifndef _WIN32
using mmap_hook_type = void* (*)(void* addr, size_t length, int prot, int flags, int fd, off_t offset);
using munmap_hook_type = int (*)(void* addr, size_t length);
#endif

#ifdef ENABLE_JEMALLOC
void* malloc(size_t size) {
  return ::je_malloc(size);
}
void* calloc(size_t n, size_t size) {
  return ::je_calloc(n, size);
}
void* memalign(size_t align, size_t s) {
  return ::je_memalign(align, s);
}
void* valloc(size_t size) {
  return ::je_valloc(size);
}
int posix_memalign(void** r, size_t a, size_t s) {
  return ::je_posix_memalign(r, a, s);
}
void* realloc(void* ptr, size_t size) {
  return ::je_realloc(ptr, size);
}
void free(void* ptr) {
  return ::je_free(ptr);
}
size_t malloc_usable_size(void* ptr) {
  return ::je_malloc_usable_size(ptr);
}
void* aligned_alloc(size_t a, size_t s) {
  return ::je_aligned_alloc(a, s);
}
void aligned_free(void* ptr) {
  return ::je_free(ptr);
}

#ifndef _WIN32
extern "C" mmap_hook_type je___mmap_hook;
extern "C" munmap_hook_type je___munmap_hook;

void SetJemallocMmapHook(mmap_hook_type Hook) {
  je___mmap_hook = Hook;
}
void SetJemallocMunmapHook(munmap_hook_type Hook) {
  je___munmap_hook = Hook;
}
#endif

#elif defined(_WIN32)
#error "Tried building _WIN32 without jemalloc"

#else
void* malloc(size_t size) {
  return ::malloc(size);
}
void* calloc(size_t n, size_t size) {
  return ::calloc(n, size);
}
void* memalign(size_t align, size_t s) {
  return ::memalign(align, s);
}
void* valloc(size_t size) {
  return ::valloc(size);
}
int posix_memalign(void** r, size_t a, size_t s) {
  return ::posix_memalign(r, a, s);
}
void* realloc(void* ptr, size_t size) {
  return ::realloc(ptr, size);
}
void free(void* ptr) {
  return ::free(ptr);
}
size_t malloc_usable_size(void* ptr) {
  return ::malloc_usable_size(ptr);
}
void* aligned_alloc(size_t a, size_t s) {
  return ::aligned_alloc(a, s);
}
void aligned_free(void* ptr) {
  return ::free(ptr);
}

void SetJemallocMmapHook(mmap_hook_type) {}
void SetJemallocMunmapHook(munmap_hook_type) {}

#endif
} // namespace FEXCore::Allocator
