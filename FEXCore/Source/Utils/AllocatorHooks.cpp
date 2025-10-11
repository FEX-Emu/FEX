// SPDX-License-Identifier: MIT
#ifdef ENABLE_FEX_ALLOCATOR
#include <rpmalloc/rpmalloc.h>
#ifndef _WIN32
#include <linux/prctl.h>
#include <sys/prctl.h>
#include <sys/mman.h>
#else
#define NTDDI_VERSION 0x0A000005
#include <memoryapi.h>
#endif
#endif

#include <cstdint>
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

namespace FEXCore::Allocator {
using mmap_hook_type = void* (*)(void* addr, size_t length, int prot, int flags, int fd, off_t offset);
using munmap_hook_type = int (*)(void* addr, size_t length);

#ifdef ENABLE_FEX_ALLOCATOR
typedef void* (*rp_mmap_hook_type)(size_t size, size_t alignment, size_t* offset, size_t* mapped_size);
typedef void (*rp_munmap_hook_type)(void* address, size_t offset, size_t mapped_size);
extern "C" rp_mmap_hook_type rp_mmap_hook;
extern "C" rp_munmap_hook_type rp_munmap_hook;

#ifndef _WIN32
mmap_hook_type fex_mmap_hook = ::mmap;
munmap_hook_type fex_munmap_hook = ::munmap;
#endif

// Assume a 64KB page size until told otherwise.
static rpmalloc_config_t global_config {
  .page_size = 64 * 1024,
  // THP causes crashes for some reason.
  .enable_huge_pages = 0,
  .disable_decommit = 0,
  .page_name = "FEXAllocator",
  .huge_page_name = "FEXAllocator",
  .unmap_on_finalize = 0,
};

void* malloc(size_t size) {
  return ::rpmalloc(size);
}
void* calloc(size_t n, size_t size) {
  return ::rpcalloc(n, size);
}
void* memalign(size_t align, size_t s) {
  return ::rpmemalign(align, s);
}
void* valloc(size_t size) {
  return ::rpaligned_alloc(global_config.page_size, size);
}
int posix_memalign(void** r, size_t a, size_t s) {
  void* ptr;
  auto res = ::rpposix_memalign(&ptr, a, s);
  *r = ptr;
  return res;
}
void* realloc(void* ptr, size_t size) {
  return ::rprealloc(ptr, size);
}
void free(void* ptr) {
  return ::rpfree(ptr);
}
size_t malloc_usable_size(void* ptr) {
  return ::rpmalloc_usable_size(ptr);
}
void* aligned_alloc(size_t a, size_t s) {
  return ::rpaligned_alloc(a, s);
}
void aligned_free(void* ptr) {
  return ::rpfree(ptr);
}

void InitializeThread() {
  rpmalloc_thread_initialize();
}

[[nodiscard]]
constexpr uint64_t AlignUp(uint64_t value, uint64_t size) {
  return value + (size - value % size) % size;
}

static void* FEX_rp_mmap(size_t size, size_t alignment, size_t* offset, size_t* mapped_size) {
#define pointer_offset(ptr, ofs) (void*)((char*)(ptr) + (ptrdiff_t)(ofs))
  // If the alignment is less than the operating page size then alignment is guaranteed. Just remove it.
  if (alignment < global_config.page_size) {
    alignment = 0;
  }

  size_t map_size = AlignUp(size + alignment, global_config.page_size);
#ifdef _WIN32
  auto ptr = VirtualAlloc(0, map_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
  auto ptr = fex_mmap_hook(0, map_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

  if (ptr == MAP_FAILED) {
    ptr = nullptr;
  } else {
#ifndef PR_SET_VMA
#define PR_SET_VMA 0x53564d41
#endif

#ifndef PR_SET_VMA_ANON_NAME
#define PR_SET_VMA_ANON_NAME 0
#endif
    prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME, ptr, map_size, global_config.page_name);
  }
#endif

  if (ptr == nullptr) {
    fprintf(stderr, "Failed to map VMA region.");
    return nullptr;
  }

  if (alignment) {
    size_t padding = ((uintptr_t)ptr & (uintptr_t)(alignment - 1));
    if (padding) {
      padding = alignment - padding;
    }
    ptr = pointer_offset(ptr, padding);
    *offset = padding;
  }
  *mapped_size = map_size;
  return ptr;
}

static void FEX_rp_memory_commit(void* address, size_t size) {
  if (global_config.disable_decommit) {
    return;
  }

#ifdef _WIN32
  if (!VirtualAlloc(address, size, MEM_COMMIT, PAGE_READWRITE)) {
#else
  if (false) {
#endif
    fprintf(stderr, "Failed to commit VMA region.");
  }
}

static void FEX_rp_memory_decommit(void* address, size_t size) {
  if (global_config.disable_decommit) {
    return;
  }

#ifdef _WIN32
  if (!VirtualFree(address, size, MEM_DECOMMIT)) {
#else
  if (madvise(address, size, MADV_DONTNEED)) {
#endif
    fprintf(stderr, "Failed to decommit VMA region.");
  }
}

static void FEX_rp_memory_unmap(void* address, size_t offset, size_t mapped_size) {
  address = pointer_offset(address, -(int32_t)offset);
#ifdef _WIN32
  ::VirtualFree(address, mapped_size, MEM_RELEASE);
#else
  int Result = fex_munmap_hook(address, mapped_size);
  if (Result == -1) {
    fprintf(stderr, "Failed to unmap VMA region.");
  }
#endif
#undef pointer_offset
}

void SetupAllocatorHooks(mmap_hook_type MMapHook, munmap_hook_type MunmapHook) {
#ifndef _WIN32
  fex_mmap_hook = MMapHook;
  fex_munmap_hook = MunmapHook;
#endif
}

static rpmalloc_interface_t global_interface {
  .memory_map = FEX_rp_mmap,
  .memory_commit = FEX_rp_memory_commit,
  .memory_decommit = FEX_rp_memory_decommit,
  .memory_unmap = FEX_rp_memory_unmap,
  .map_fail_callback = nullptr,
  .error_callback = nullptr,
};

void InitializeAllocator(size_t PageSize) {
  global_config.page_size = PageSize;
  rpmalloc_initialize_config(&global_interface, &global_config);
  rp_mmap_hook = FEX_rp_mmap;
  rp_munmap_hook = FEX_rp_memory_unmap;
}

#elif defined(_WIN32)
#error "Tried building _WIN32 without jemalloc"

#else
void InitializeThread() {}

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

void SetupAllocatorHooks(mmap_hook_type MMapHook, munmap_hook_type MunmapHook) {}

void InitializeAllocator(size_t PageSize) {}

#endif
} // namespace FEXCore::Allocator
