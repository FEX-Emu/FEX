#include "LogManager.h"
#include "Interface/Memory/MemMapper.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace FEXCore::Memory {

  void *MemMapper::MapRegion(uint64_t Offset, size_t Size, bool Fixed, bool RelativeToBase) {
    return MapRegion(Offset, Size, PROT_READ | PROT_WRITE, Fixed, RelativeToBase);
  }

  void *MemMapper::ChangeMappedRegion(uint64_t Offset, size_t Size, uint32_t Flags, bool Fixed, bool RelativeToBase) {
    uintptr_t PtrOffset = Offset;

    if (RelativeToBase) {
      PtrOffset = reinterpret_cast<uintptr_t>(SHM->Object.Ptr) + Offset;
    }

    void *Ptr = mmap(reinterpret_cast<void*>(PtrOffset), Size, Flags,
      MAP_POPULATE | MAP_PRIVATE | (Fixed ? MAP_FIXED : 0), SHM->SHMFD, Offset);

    if (Ptr == MAP_FAILED) {
      LogMan::Msg::A("Failed to map memory region [0x%lx, 0x%lx)", Offset, Offset + Size);
      return nullptr;
    }

    return Ptr;
  }

  void *MemMapper::MapRegion(uint64_t Offset, size_t Size, uint32_t Flags, bool Fixed, bool RelativeToBase) {
    uintptr_t PtrOffset = Offset;

    if (RelativeToBase) {
      PtrOffset = reinterpret_cast<uintptr_t>(SHM->Object.Ptr) + Offset;
    }

    void *Ptr = mmap(reinterpret_cast<void*>(PtrOffset), Size, Flags,
      MAP_PRIVATE | (Fixed ? MAP_FIXED : 0), SHM->SHMFD, Offset);

    if (Ptr == MAP_FAILED) {
      LogMan::Msg::A("Failed to map memory region [0x%lx, 0x%lx)", Offset, Offset + Size);
      return nullptr;
    }

    MappedRegions.emplace_back(MemRegion{Ptr, Offset, Size});

    return Ptr;
  }

  void MemMapper::UnmapRegion(void *Ptr, size_t Size) {
    auto it = std::find(MappedRegions.begin(), MappedRegions.end(), Ptr);
    if (it != MappedRegions.end()) {
      munmap(Ptr, Size);
      MappedRegions.erase(it);
    }
  }

  void *MemMapper::GetPointer(uint64_t Offset) {
    for (auto const &Region : MappedRegions) {
      if (Region.contains(Offset)) {
        return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(Region.Ptr) + (Offset - Region.Offset));
      }
    }

    return nullptr;
  }

#ifndef NDEBUG
  void *MemMapper::GetPointerSizeCheck(uint64_t Offset, uint64_t Size) {
    for (auto const &Region : MappedRegions) {
      if (Region.contains(Offset)) {
        LogMan::Throw::A((Region.Offset + Region.Size) >= (Offset + Size), "Pointer in region but region isn't large enough. Needs 0x%lx more", Offset - Region.Offset + Size - Region.Size);
        if ((Region.Offset + Region.Size) >= (Offset + Size)) {
          return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(Region.Ptr) + (Offset - Region.Offset));
        }
      }
    }
    return nullptr;
  }
#endif

}

