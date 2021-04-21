#include <FEXCore/Utils/Allocator.h>
#include <sys/mman.h>

extern "C" {
  extern void *__libc_malloc(size_t size);
  extern void *__libc_realloc(void *ptr, size_t size);
  extern void __libc_free(void *ptr);
}

namespace FEXCore::Allocator {
  MMAP_Hook mmap {::mmap};
  MUNMAP_Hook munmap {::munmap};
  MALLOC_Hook malloc {::__libc_malloc};
  REALLOC_Hook realloc {::__libc_realloc};
  FREE_Hook free {::__libc_free};
}

extern "C" {
  // Override the global functions
  void *malloc(size_t size) { return FEXCore::Allocator::malloc(size); }
  void *realloc(void *ptr, size_t size) { return FEXCore::Allocator::realloc(ptr, size); }
  void free(void *ptr) { return FEXCore::Allocator::free(ptr); }
}
