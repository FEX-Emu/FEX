/*
$info$
tags: thunklibs|fex_malloc
desc: Handles allocations between guest and host thunks
$end_info$
*/

#include <cstring>
#include <map>
#include <string>

#include "common/Guest.h"
#include <stdarg.h>

#include "Types.h"

#include "thunkgen_guest_libfex_malloc.inl"

#include <vector>

extern "C" {
void fex_malloc_NoOptimize() {
  // Does nothing, just ensures our libraries pull it in
}

#define ALIAS(fn) __attribute__((alias(#fn), used))
#define PREALIAS(fn) ALIAS(fn)


void* __libc_calloc(size_t n, size_t size) PREALIAS(fexfn_pack_calloc);

void __libc_free(void* ptr) PREALIAS(fexfn_pack_free);

void* __libc_malloc(size_t size) PREALIAS(fexfn_pack_malloc);

void* __libc_memalign(size_t align, size_t s) PREALIAS(fexfn_pack_memalign);

void* __libc_realloc(void* ptr, size_t size) PREALIAS(fexfn_pack_realloc);

void* __libc_valloc(size_t size) PREALIAS(fexfn_pack_valloc);

int __posix_memalign(void** r, size_t a, size_t s) PREALIAS(fexfn_pack_posix_memalign);

// If we replace libc malloc and an application calls the malloc_usable_size then we can get a crash
// Symbol doesn't alias exactly so just wrap it

size_t __malloc_usable_size(void* ptr) {
  return fexfn_pack_malloc_usable_size(ptr);
}
}

LOAD_LIB(libfex_malloc)
