#pragma once

#include <FEXCore/Utils/CompilerDefs.h>

#include <cstdint>
#include <functional>

namespace FEXCore::Allocator {
  using MMAP_Hook = void*(*)(void*, size_t, int, int, int, off_t);
  using MUNMAP_Hook = int(*)(void*, size_t);

  using MALLOC_Hook = void*(*)(size_t);
  using REALLOC_Hook = void*(*)(void*, size_t);
  using FREE_Hook = void(*)(void*);

  FEX_DEFAULT_VISIBILITY extern MMAP_Hook mmap;
  FEX_DEFAULT_VISIBILITY extern MUNMAP_Hook munmap;
  FEX_DEFAULT_VISIBILITY extern MALLOC_Hook malloc;
  FEX_DEFAULT_VISIBILITY extern REALLOC_Hook realloc;
  FEX_DEFAULT_VISIBILITY extern FREE_Hook free;

  FEX_DEFAULT_VISIBILITY void SetupHooks();
  FEX_DEFAULT_VISIBILITY void ClearHooks();
}
