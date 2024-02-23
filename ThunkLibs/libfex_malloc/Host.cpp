/*
$info$
tags: thunklibs|fex_malloc
desc: Handles allocations between guest and host thunks
$end_info$
*/

#include <cstring>
#include <cstdlib>
#include <stdio.h>
#include <memory.h>

#include "common/Host.h"
#include <dlfcn.h>

#include "Types.h"

#include "thunkgen_host_libfex_malloc.inl"

void fexfn_impl_libfex_malloc_fex_get_allocation_ptrs(AllocationPtrs* Ptrs);

extern "C" {
// FEX allocation routines
extern MallocPtr FEX_Malloc_Ptr;
extern FreePtr FEX_Free_Ptr;
extern CallocPtr FEX_Calloc_Ptr;
extern MemalignPtr FEX_Memalign_Ptr;
extern ReallocPtr FEX_Realloc_Ptr;
extern VallocPtr FEX_Valloc_Ptr;
extern PosixMemalignPtr FEX_PosixMemalign_Ptr;
extern AlignedAllocPtr FEX_AlignedAlloc_Ptr;
extern MallocUsablePtr FEX_MallocUsable_Ptr;
}

extern "C" {
AllocationPtrs AllocationPointers {
  .Malloc = FEX_Malloc_Ptr,
  .Free = FEX_Free_Ptr,
  .Calloc = FEX_Calloc_Ptr,
  .Memalign = FEX_Memalign_Ptr,
  .Realloc = FEX_Realloc_Ptr,
  .Valloc = FEX_Valloc_Ptr,
  .PosixMemalign = FEX_PosixMemalign_Ptr,
  .AlignedAlloc = FEX_AlignedAlloc_Ptr,
  .MallocUsable = FEX_MallocUsable_Ptr,
};

// Our allocators
#define ALIAS(fn) __attribute__((alias(#fn), used))
#define PREALIAS(fn) ALIAS(fn)

void* fex_malloc(size_t Size) {
  return AllocationPointers.Malloc(Size);
}
void* __libc_malloc(size_t Size) __attribute__((alias("fex_malloc"), used));
void* malloc(size_t Size) __attribute__((alias("fex_malloc"), used));

void fex_free(void* p) {
  AllocationPointers.Free(p);
}
void __libc_free(void* ptr) PREALIAS(fex_free);
void __GI___libc_free(void* ptr) PREALIAS(fex_free);
void free(void* ptr) PREALIAS(fex_free);

void* fex_calloc(size_t n, size_t size) {
  return AllocationPointers.Calloc(n, size);
}
void* __libc_calloc(size_t n, size_t size) PREALIAS(fex_calloc);
void* calloc(size_t n, size_t size) PREALIAS(fex_calloc);

void* fex_memalign(size_t align, size_t s) {
  return AllocationPointers.Memalign(align, s);
}
void* __libc_memalign(size_t align, size_t s) PREALIAS(fex_memalign);
void* memalign(size_t align, size_t s) PREALIAS(fex_memalign);

void* fex_realloc(void* ptr, size_t size) {
  return AllocationPointers.Realloc(ptr, size);
}
void* __libc_realloc(void* ptr, size_t size) PREALIAS(fex_realloc);
void* realloc(void* ptr, size_t size) PREALIAS(fex_realloc);

void* fex_valloc(size_t size) {
  return AllocationPointers.Valloc(size);
}
void* __libc_valloc(size_t size) PREALIAS(fex_valloc);
void* valloc(size_t size) PREALIAS(fex_valloc);

int fex_posix_memalign(void** r, size_t a, size_t s) {
  return AllocationPointers.PosixMemalign(r, a, s);
}
int __posix_memalign(void** r, size_t a, size_t s) PREALIAS(fex_posix_memalign);
int posix_memalign(void** r, size_t a, size_t s) PREALIAS(fex_posix_memalign);

void* fex_aligned_alloc(size_t a, size_t s) {
  return AllocationPointers.AlignedAlloc(a, s);
}
void* aligned_alloc(size_t a, size_t s) PREALIAS(fex_aligned_alloc);

size_t fex_malloc_usable_size(void* ptr) {
  return AllocationPointers.MallocUsable(ptr);
}

size_t __malloc_usable_size(void* ptr) {
  return fex_malloc_usable_size(ptr);
}
size_t malloc_usable_size(void* ptr) {
  return fex_malloc_usable_size(ptr);
}

static void fexfn_unpack_libfex_malloc_malloc(void* argsv) {
  struct arg_t {
    size_t a_0;
    void* rv;
  };
  auto args = (arg_t*)argsv;
  args->rv = AllocationPointers.Malloc(args->a_0);
}

static void fexfn_unpack_libfex_malloc_free(void* argsv) {
  struct arg_t {
    void* a_0;
  };
  auto args = (arg_t*)argsv;
  AllocationPointers.Free(args->a_0);
}

static void fexfn_unpack_libfex_malloc_calloc(void* argsv) {
  struct arg_t {
    size_t a_0;
    size_t a_1;
    void* rv;
  };
  auto args = (arg_t*)argsv;
  args->rv = AllocationPointers.Calloc(args->a_0, args->a_1);
}
static void fexfn_unpack_libfex_malloc_memalign(void* argsv) {
  struct arg_t {
    size_t a_0;
    size_t a_1;
    void* rv;
  };
  auto args = (arg_t*)argsv;
  args->rv = AllocationPointers.Memalign(args->a_0, args->a_1);
}
static void fexfn_unpack_libfex_malloc_realloc(void* argsv) {
  struct arg_t {
    void* a_0;
    size_t a_1;
    void* rv;
  };
  auto args = (arg_t*)argsv;
  args->rv = AllocationPointers.Realloc(args->a_0, args->a_1);
}
static void fexfn_unpack_libfex_malloc_valloc(void* argsv) {
  struct arg_t {
    size_t a_0;
    void* rv;
  };
  auto args = (arg_t*)argsv;

  args->rv = AllocationPointers.Valloc(args->a_0);
}
static void fexfn_unpack_libfex_malloc_posix_memalign(void* argsv) {
  struct arg_t {
    void** a_0;
    size_t a_1;
    size_t a_2;
    int rv;
  };
  auto args = (arg_t*)argsv;

  args->rv = AllocationPointers.PosixMemalign(args->a_0, args->a_1, args->a_2);
}
static void fexfn_unpack_libfex_malloc_aligned_alloc(void* argsv) {
  struct arg_t {
    size_t a_0;
    size_t a_1;
    void* rv;
  };
  auto args = (arg_t*)argsv;

  args->rv = AllocationPointers.AlignedAlloc(args->a_0, args->a_1);
}
static void fexfn_unpack_libfex_malloc_malloc_usable_size(void* argsv) {
  struct arg_t {
    void* a_0;
    size_t rv;
  };
  auto args = (arg_t*)argsv;

  args->rv = AllocationPointers.MallocUsable(args->a_0);
}

void (*__free_hook)(void* ptr) = fex_free;
void* (*__malloc_hook)(size_t size) = fex_malloc;
void* (*__realloc_hook)(void* ptr, size_t size) = fex_realloc;
void* (*__memalign_hook)(size_t alignment, size_t size) = fex_memalign;
}

void fexfn_impl_libfex_malloc_fex_get_allocation_ptrs(AllocationPtrs* Ptrs) {
  *Ptrs = AllocationPointers;
}

static void init_lib() {
  // Set pointers
  AllocationPointers.Malloc = FEX_Malloc_Ptr;
  AllocationPointers.Free = FEX_Free_Ptr;
  AllocationPointers.Calloc = FEX_Calloc_Ptr;
  AllocationPointers.Memalign = FEX_Memalign_Ptr;
  AllocationPointers.Realloc = FEX_Realloc_Ptr;
  AllocationPointers.Valloc = FEX_Valloc_Ptr;
  AllocationPointers.PosixMemalign = FEX_PosixMemalign_Ptr;
  AllocationPointers.AlignedAlloc = FEX_AlignedAlloc_Ptr;
  AllocationPointers.MallocUsable = FEX_MallocUsable_Ptr;
}

EXPORTS(libfex_malloc)
LOAD_LIB_INIT(init_lib)
