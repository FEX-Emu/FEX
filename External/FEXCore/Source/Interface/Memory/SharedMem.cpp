#include "LogManager.h"
#include "Interface/Memory/SharedMem.h"
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace FEXCore::SHM {
  void *MapRegionFlags(InternalSHMObject *SHM, size_t Offset, size_t Size, uint32_t flags, bool Fixed) {
    uintptr_t PtrOffset = reinterpret_cast<uintptr_t>(SHM->Object.Ptr) + Offset;

    void *Ptr = mmap(reinterpret_cast<void*>(PtrOffset), Size, flags,
      MAP_NORESERVE | MAP_PRIVATE | (Fixed ? MAP_FIXED : 0), SHM->SHMFD, Offset);
    if (Ptr == MAP_FAILED) {
      LogMan::Msg::A("Failed to map memory region [0x%lx, 0x%lx)", Offset, Offset + Size);
      return nullptr;
    }

    return Ptr;
  }

  SHMObject *AllocateSHMRegion(size_t Size) {
    InternalSHMObject *SHM = new InternalSHMObject{};
    const std::string SHMName = "FEXCore" + std::to_string(getpid());

    SHM->SHMFD = shm_open(SHMName.c_str(), O_RDWR | O_CREAT | O_EXCL, 0600);
    if (SHM->SHMFD == -1) {
      LogMan::Msg::E("Couldn't open SHM");
      goto err;
    }

    // Unlink the SHM file immediately so it doesn't get left around
    shm_unlink(SHMName.c_str());

    // Extend the SHM to the size we requested
    if (ftruncate(SHM->SHMFD, Size) != 0) {
      LogMan::Msg::E("Couldn't set SHM size");
      goto err;
    }

    SHM->Object.Ptr = MapRegionFlags(SHM, 0, Size, PROT_READ | PROT_WRITE, false);
    SHM->Size = Size;
    if (SHM->Object.Ptr == nullptr) {
      goto err;
    }

    return &SHM->Object;
err:
    delete SHM;
    return nullptr;
  }

  void DestroyRegion(SHMObject *SHM) {
    InternalSHMObject *Obj = reinterpret_cast<InternalSHMObject*>(SHM);
    close(Obj->SHMFD);
    delete Obj;
  }

}
