#pragma once
#include "Interface/Memory/SharedMem.h"
#include <FEXCore/Memory/MemMapper.h>
#include <stdint.h>
#include <vector>

namespace FEXCore::Context {
struct Context;
}

namespace FEXCore::Memory {

  class MemMapper final {
  friend struct FEXCore::Context::Context;
  public:
    void SetBaseRegion(FEXCore::SHM::SHMObject *NewSHM) {
      SHM = reinterpret_cast<FEXCore::SHM::InternalSHMObject*>(NewSHM);
    }
    size_t GetSHMSize() const {
      return SHM->Size;
    }

    void *MapRegion(uint64_t Offset, size_t Size, bool Fixed = true, bool RelativeToBase = true);
    void *MapRegion(uint64_t Offset, size_t Size, uint32_t Flags, bool Fixed = true, bool RelativeToBase = true);
    void *ChangeMappedRegion(uint64_t Offset, size_t Size, uint32_t Flags, bool Fixed = true, bool RelativeToBase = true);

    void UnmapRegion(void *Ptr, size_t Size);

    void *GetMemoryBase() { return SHM->Object.Ptr; }

    void *GetPointer(uint64_t Offset);
#ifndef NDEBUG
    void *GetPointerSizeCheck(uint64_t Offset, uint64_t Size);
#else
    void *GetPointerSizeCheck(uint64_t Offset, uint64_t Size) { return GetPointer(Offset); }
#endif
    template<typename T>
    T GetPointer(uint64_t Offset) {
      return reinterpret_cast<T>(GetPointer(Offset));
    }

    template<typename T>
    T GetBaseOffset(uint64_t Offset) {
      return reinterpret_cast<T>((reinterpret_cast<uintptr_t>(GetMemoryBase()) + Offset));
    }

  private:
    FEXCore::SHM::InternalSHMObject *SHM;
    std::vector<FEXCore::Memory::MemRegion> MappedRegions{};
  };
}

