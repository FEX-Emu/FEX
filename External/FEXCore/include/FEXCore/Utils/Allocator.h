#pragma once

#include <cstdint>
#include <functional>

namespace FEXCore::Allocator {
  using MMAP_Hook = std::function<void*(void*, size_t, int, int, int, off_t)>;
  using MUNMAP_Hook = std::function<int(void*, size_t)>;

  __attribute__((visibility("default"))) extern MMAP_Hook mmap;
  __attribute__((visibility("default"))) extern MUNMAP_Hook munmap;
}
