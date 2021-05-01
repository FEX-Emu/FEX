#pragma once

#include <cstdint>
#include <functional>

namespace FEXCore::Allocator {
  using MMAP_Hook = void*(*)(void*, size_t, int, int, int, off_t);
  using MUNMAP_Hook = int(*)(void*, size_t);

  using MALLOC_Hook = void*(*)(size_t);
  using REALLOC_Hook = void*(*)(void*, size_t);
  using FREE_Hook = void(*)(void*);

  __attribute__((visibility("default"))) extern MMAP_Hook mmap;
  __attribute__((visibility("default"))) extern MUNMAP_Hook munmap;
  __attribute__((visibility("default"))) extern MALLOC_Hook malloc;
  __attribute__((visibility("default"))) extern REALLOC_Hook realloc;
  __attribute__((visibility("default"))) extern FREE_Hook free;

  void SetupHooks();
}
