#pragma once
#include <cstddef>
#include <stdint.h>

namespace FEXCore::Memory {
  struct MemRegion {
    void *Ptr;
    size_t Offset;
    size_t Size;

    bool operator==(void *rhs) const { return Ptr == rhs; }
    bool contains(uint64_t Addr) const { return Addr >= Offset && Addr < (Offset + Size); }
  };
}

