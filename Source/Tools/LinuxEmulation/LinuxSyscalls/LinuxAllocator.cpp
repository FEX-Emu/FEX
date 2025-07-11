// SPDX-License-Identifier: MIT
#include "LinuxSyscalls/LinuxAllocator.h"
#include "LinuxSyscalls/Syscalls.h"

#include <FEXCore/Utils/MathUtils.h>
#include <FEXCore/Utils/TypeDefines.h>
#include <FEXHeaderUtils/Syscalls.h>
#include <FEXCore/fextl/map.h>
#include <FEXCore/fextl/memory.h>

#include <bitset>
#include <linux/mman.h>
#include <unistd.h>
#include <sys/user.h>
#include <sys/mman.h>
#include <sys/shm.h>

#ifndef MREMAP_DONTUNMAP
#define MREMAP_DONTUNMAP 4
#endif

namespace FEX::HLE {
class MemAllocator32Bit final : public FEX::HLE::MemAllocator {
private:
  static constexpr uint64_t BASE_KEY = 16;
  const uint64_t TOP_KEY = 0xFFFF'F000ULL >> FEXCore::Utils::FEX_PAGE_SHIFT;
  const uint64_t TOP_KEY32BIT = 0x7FFF'F000ULL >> FEXCore::Utils::FEX_PAGE_SHIFT;

public:
  MemAllocator32Bit() {
    // First 16 pages are taken by the Linux kernel
    for (size_t i = 0; i < 16; ++i) {
      MappedPages.set(i);
    }
    // Take the top page as well
    MappedPages.set(TOP_KEY);
    if (SearchDown) {
      LastScanLocation = TOP_KEY;
      LastKeyLocation = TOP_KEY;
      LastKeyLocation32Bit = TOP_KEY32BIT;
      FindPageRangePtr = &MemAllocator32Bit::FindPageRange_TopDown;
    } else {
      LastScanLocation = BASE_KEY;
      LastKeyLocation = BASE_KEY;
      FindPageRangePtr = &MemAllocator32Bit::FindPageRange;
    }
  }

  void* Mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset) override;
  int Munmap(void* addr, size_t length) override;
  void* Mremap(void* old_address, size_t old_size, size_t new_size, int flags, void* new_address) override;
  uint64_t Shmat(int shmid, const void* shmaddr, int shmflg, uint32_t* ResultAddress) override;
  uint64_t Shmdt(const void* shmaddr) override;
  static constexpr bool SearchDown = true;

  // PageAddr is a page already shifted to page index
  // PagesLength is the number of pages
  void SetUsedPages(uint64_t PageAddr, size_t PagesLength) {
    // Set the range as mapped
    for (size_t i = 0; i < PagesLength; ++i) {
      MappedPages.set(PageAddr + i);
    }
  }

  // PageAddr is a page already shifted to page index
  // PagesLength is the number of pages
  void SetFreePages(uint64_t PageAddr, size_t PagesLength) {
    // Set the range as unused
    for (size_t i = 0; i < PagesLength; ++i) {
      MappedPages.reset(PageAddr + i);
    }
  }

private:
  // Set that contains 4k mapped pages
  // This is the full 32bit memory range
  std::bitset<0x10'0000> MappedPages;
  fextl::map<uint32_t, int> PageToShm {};
  uint64_t LastScanLocation {};
  uint64_t LastKeyLocation {};
  uint64_t LastKeyLocation32Bit {};
  std::mutex AllocMutex {};
  uint64_t FindPageRange(uint64_t Start, size_t Pages) const;
  uint64_t FindPageRange_TopDown(uint64_t Start, size_t Pages) const;
  using FindHandler = uint64_t (MemAllocator32Bit::*)(uint64_t Start, size_t Pages) const;
  FindHandler FindPageRangePtr {};
};

uint64_t MemAllocator32Bit::FindPageRange(uint64_t Start, size_t Pages) const {
  // Linear range scan
  while (Start != TOP_KEY) {
    bool Free = true;
    if ((Start + Pages) > TOP_KEY) {
      return 0;
    }
    uint64_t Offset = 0;
    for (; Offset < Pages; ++Offset) {
      if (MappedPages.test(Start + Offset)) {
        Free = false;
        break;
      }
    }

    if (Free) {
      return Start;
    }
    Start += Offset + 1;
  }

  return 0;
}

uint64_t MemAllocator32Bit::FindPageRange_TopDown(uint64_t Start, size_t Pages) const {
  // Linear range scan
  while (Start >= BASE_KEY && Start <= TOP_KEY) {
    bool Free = true;

    uint64_t Offset = 0;
    for (; Offset < Pages; ++Offset) {
      if (MappedPages.test(Start - Offset)) {
        Free = false;
        break;
      }
    }

    if (Free) {
      return Start - Offset;
    }
    Start -= Offset + 1;
  }

  return 0;
}

void* MemAllocator32Bit::Mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset) {
  std::scoped_lock<std::mutex> lk {AllocMutex};
  size_t PagesLength = FEXCore::AlignUp(length, FEXCore::Utils::FEX_PAGE_SIZE) >> FEXCore::Utils::FEX_PAGE_SHIFT;

  uintptr_t Addr = reinterpret_cast<uintptr_t>(addr);
  uintptr_t PageAddr = Addr >> FEXCore::Utils::FEX_PAGE_SHIFT;

  // Define MAP_FIXED_NOREPLACE ourselves to ensure we always parse this flag
  constexpr int FEX_MAP_FIXED_NOREPLACE = 0x100000;
  bool Fixed = ((flags & MAP_FIXED) || (flags & FEX_MAP_FIXED_NOREPLACE));

  // Both Addr and length must be page aligned
  if (Addr & ~FEXCore::Utils::FEX_PAGE_MASK) {
    return reinterpret_cast<void*>(-EINVAL);
  }

  // If we do have an fd then offset must be page aligned
  if (fd != -1 && offset & ~FEXCore::Utils::FEX_PAGE_MASK) {
    return reinterpret_cast<void*>(-EINVAL);
  }

  if (Addr + length > std::numeric_limits<uint32_t>::max()) {
    return reinterpret_cast<void*>(-EOVERFLOW);
  }

  // Check reserved range
  if (Fixed && PageAddr < 16) {
    return reinterpret_cast<void*>(-EINVAL);
  }

  if (!Fixed) {
    // If we aren't mapping fixed the ignore the address input
    Addr = 0;
    PageAddr = 0;
  }

  bool Map32Bit = flags & FEX::HLE::X86_64_MAP_32BIT;

  // Remove the MAP_32BIT flag if it exists now
  flags &= ~FEX::HLE::X86_64_MAP_32BIT;

  auto AllocateNoHint = [&]() -> void* {
    bool Wrapped = false;
    uint64_t BottomPage = Map32Bit && (LastScanLocation >= LastKeyLocation32Bit) ? LastKeyLocation32Bit : LastScanLocation;
restart: {
  // Linear range scan
  uint64_t LowerPage = (this->*FindPageRangePtr)(BottomPage, PagesLength);
  if (LowerPage == 0) {
    // Try again but this time from the start
    BottomPage = Map32Bit ? LastKeyLocation32Bit : LastKeyLocation;
    LowerPage = (this->*FindPageRangePtr)(BottomPage, PagesLength);
  }

  uint64_t UpperPage = LowerPage + PagesLength;
  if (LowerPage == 0) {
    return reinterpret_cast<void*>(-ENOMEM);
  }
  {
    // Try and map the range
    void* MappedPtr =
      ::mmap(reinterpret_cast<void*>(LowerPage << FEXCore::Utils::FEX_PAGE_SHIFT), length, prot, flags | FEX_MAP_FIXED_NOREPLACE, fd, offset);

    if (MappedPtr == MAP_FAILED && errno != EEXIST) {
      return reinterpret_cast<void*>(-errno);
    } else if (MappedPtr == MAP_FAILED || MappedPtr >= reinterpret_cast<void*>(TOP_KEY << FEXCore::Utils::FEX_PAGE_SHIFT)) {
      // Handles the case where MAP_FIXED_NOREPLACE failed with MAP_FAILED
      // or if the host system's kernel isn't new enough then it returns the wrong pointer
      if (MappedPtr != MAP_FAILED && MappedPtr >= reinterpret_cast<void*>(TOP_KEY << FEXCore::Utils::FEX_PAGE_SHIFT)) {
        // Make sure to munmap this so we don't leak memory
        ::munmap(MappedPtr, length);
      }

      if (UpperPage == TOP_KEY) {
        BottomPage = BASE_KEY;
        Wrapped = true;
        goto restart;
      } else if (Wrapped && LowerPage >= LastScanLocation) {
        // We linear scanned the entire memory range. Give up
        return (void*)(uintptr_t)-errno;
      } else {
        // Try again
        if (SearchDown) {
          --BottomPage;
        } else {
          ++BottomPage;
        }
        goto restart;
      }
    } else {
      if (SearchDown) {
        LastScanLocation = LowerPage;
      } else {
        LastScanLocation = UpperPage;
      }
      SetUsedPages(LowerPage, PagesLength);
      return MappedPtr;
    }
  }
}
  };

  // Find a region that fits our address
  if (Addr == 0) {
    return AllocateNoHint();
  } else {
    void* MappedPtr = ::mmap(reinterpret_cast<void*>(PageAddr << FEXCore::Utils::FEX_PAGE_SHIFT),
                             PagesLength << FEXCore::Utils::FEX_PAGE_SHIFT, prot, flags, fd, offset);

    if (MappedPtr >= reinterpret_cast<void*>(TOP_KEY << FEXCore::Utils::FEX_PAGE_SHIFT) && (flags & FEX_MAP_FIXED_NOREPLACE)) {
      // Handles the case where MAP_FIXED_NOREPLACE isn't handled by the host system's
      // kernel and returns the wrong pointer
      // Make sure to munmap this so we don't leak memory
      ::munmap(MappedPtr, length);
      return reinterpret_cast<void*>(-EEXIST);
    } else if (MappedPtr != MAP_FAILED) {
      SetUsedPages(PageAddr, PagesLength);
      return MappedPtr;
    } else {
      return reinterpret_cast<void*>(-errno);
    }
  }
  return 0;
}

int MemAllocator32Bit::Munmap(void* addr, size_t length) {
  std::scoped_lock<std::mutex> lk {AllocMutex};
  size_t PagesLength = FEXCore::AlignUp(length, FEXCore::Utils::FEX_PAGE_SIZE) >> FEXCore::Utils::FEX_PAGE_SHIFT;

  uintptr_t Addr = reinterpret_cast<uintptr_t>(addr);
  uintptr_t PageAddr = Addr >> FEXCore::Utils::FEX_PAGE_SHIFT;

  uintptr_t PageEnd = PageAddr + PagesLength;

  // Both Addr and length must be page aligned
  if (Addr & ~FEXCore::Utils::FEX_PAGE_MASK) {
    return -EINVAL;
  }

  if (length & ~FEXCore::Utils::FEX_PAGE_MASK) {
    return -EINVAL;
  }

  if (Addr + length > std::numeric_limits<uint32_t>::max()) {
    return -EOVERFLOW;
  }

  // Check reserved range
  if (PageAddr < 16) {
    // Return success for these
    return 0;
  }

  while (PageAddr != PageEnd) {
    // Always pass to munmap, it may be something allocated we aren't tracking
    int Result = ::munmap(reinterpret_cast<void*>(PageAddr << FEXCore::Utils::FEX_PAGE_SHIFT), FEXCore::Utils::FEX_PAGE_SIZE);
    if (Result != 0) {
      return -errno;
    }

    if (MappedPages.test(PageAddr)) {
      MappedPages.reset(PageAddr);
    }

    ++PageAddr;
  }

  return 0;
}

void* MemAllocator32Bit::Mremap(void* old_address, size_t old_size, size_t new_size, int flags, void* new_address) {
  size_t OldPagesLength = FEXCore::AlignUp(old_size, FEXCore::Utils::FEX_PAGE_SIZE) >> FEXCore::Utils::FEX_PAGE_SHIFT;
  size_t NewPagesLength = FEXCore::AlignUp(new_size, FEXCore::Utils::FEX_PAGE_SIZE) >> FEXCore::Utils::FEX_PAGE_SHIFT;

  {
    std::scoped_lock<std::mutex> lk {AllocMutex};
    if (flags & MREMAP_FIXED) {
      void* MappedPtr = ::mremap(old_address, old_size, new_size, flags, new_address);

      if (MappedPtr != MAP_FAILED) {
        if (!(flags & MREMAP_DONTUNMAP)) {
          // Unmap the old location
          uintptr_t OldAddr = reinterpret_cast<uintptr_t>(old_address);
          SetFreePages(OldAddr >> FEXCore::Utils::FEX_PAGE_SHIFT, OldPagesLength);
        }

        // Map the new pages
        uintptr_t NewAddr = reinterpret_cast<uintptr_t>(MappedPtr);
        SetUsedPages(NewAddr >> FEXCore::Utils::FEX_PAGE_SHIFT, NewPagesLength);
      } else {
        return reinterpret_cast<void*>(-errno);
      }
    } else {
      uintptr_t OldAddr = reinterpret_cast<uintptr_t>(old_address);
      uintptr_t OldPageAddr = OldAddr >> FEXCore::Utils::FEX_PAGE_SHIFT;

      if (NewPagesLength < OldPagesLength) {
        void* MappedPtr = ::mremap(old_address, old_size, new_size, flags & ~MREMAP_MAYMOVE);

        if (MappedPtr != MAP_FAILED) {
          // Clear the pages that we just shrunk
          size_t NewPagesLength = FEXCore::AlignUp(new_size, FEXCore::Utils::FEX_PAGE_SIZE) >> FEXCore::Utils::FEX_PAGE_SHIFT;
          uintptr_t NewPageAddr = reinterpret_cast<uintptr_t>(MappedPtr) >> FEXCore::Utils::FEX_PAGE_SHIFT;
          SetFreePages(NewPageAddr + NewPagesLength, OldPagesLength - NewPagesLength);
          return MappedPtr;
        } else {
          return reinterpret_cast<void*>(-errno);
        }
      } else {
        // Scan the region forward from our first region's endd to see if it can be extended
        bool CanExtend {true};

        for (size_t i = OldPagesLength; i < NewPagesLength; ++i) {
          if (MappedPages[OldPageAddr + i]) {
            CanExtend = false;
            break;
          }
        }

        if (CanExtend) {
          void* MappedPtr = ::mremap(old_address, old_size, new_size, flags & ~MREMAP_MAYMOVE);

          if (MappedPtr != MAP_FAILED) {
            // Map the new pages
            size_t NewPagesLength = FEXCore::AlignUp(new_size, FEXCore::Utils::FEX_PAGE_SIZE) >> FEXCore::Utils::FEX_PAGE_SHIFT;
            uintptr_t NewAddr = reinterpret_cast<uintptr_t>(MappedPtr);
            SetUsedPages(NewAddr >> FEXCore::Utils::FEX_PAGE_SHIFT, NewPagesLength);
            return MappedPtr;
          } else if (!(flags & MREMAP_MAYMOVE)) {
            // We have one more chance if MAYMOVE is specified
            return reinterpret_cast<void*>(-errno);
          }
        }
      }
    }
  }

  // Flags can not contain MREMAP_FIXED at this point
  // Flags might contain MREMAP_MAYMOVE and/or MREMAP_DONTUNMAP
  // New Size is >= old size

  // First, try and allocate a region the size of the new size
  void* MappedPtr = this->Mmap(nullptr, new_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  std::scoped_lock<std::mutex> lk {AllocMutex};
  if (FEX::HLE::HasSyscallError(MappedPtr)) {
    // Couldn't find a region that fit our space
    return MappedPtr;
  }

  // Good news, we found a region
  // This will overwrite the previous mmap if it succeeds
  MappedPtr = ::mremap(old_address, old_size, new_size, flags | MREMAP_FIXED | MREMAP_MAYMOVE, MappedPtr);

  if (MappedPtr != MAP_FAILED) {
    if (!(flags & MREMAP_DONTUNMAP) && MappedPtr != old_address) {
      // If we have both MREMAP_DONTUNMAP not set and the new pointer is at a new location
      // Make sure to clear the old mapping
      uintptr_t OldAddr = reinterpret_cast<uintptr_t>(old_address);
      SetFreePages(OldAddr >> FEXCore::Utils::FEX_PAGE_SHIFT, OldPagesLength);
    }

    // Map the new pages
    size_t NewPagesLength = FEXCore::AlignUp(new_size, FEXCore::Utils::FEX_PAGE_SIZE) >> FEXCore::Utils::FEX_PAGE_SHIFT;
    uintptr_t NewAddr = reinterpret_cast<uintptr_t>(MappedPtr);
    SetUsedPages(NewAddr >> FEXCore::Utils::FEX_PAGE_SHIFT, NewPagesLength);
    return MappedPtr;
  }

  // Failed
  return reinterpret_cast<void*>(-errno);
}

uint64_t MemAllocator32Bit::Shmat(int shmid, const void* shmaddr, int shmflg, uint32_t* ResultAddress) {
  std::scoped_lock<std::mutex> lk {AllocMutex};

  if (shmaddr != nullptr) {
    // shmaddr must be valid
    uint64_t Result = reinterpret_cast<uint64_t>(::shmat(shmid, shmaddr, shmflg));
    if (Result != -1) {
      uint32_t SmallRet = Result >> 32;
      if (!(SmallRet == 0 || SmallRet == ~0U)) {
        LOGMAN_MSG_A_FMT("Syscall returning something with data in the upper 32bits! BUG!");
        return -ENOMEM;
      }

      uintptr_t NewAddr = reinterpret_cast<uintptr_t>(Result);
      uintptr_t NewPageAddr = NewAddr >> FEXCore::Utils::FEX_PAGE_SHIFT;

      // Add to the map
      PageToShm[NewPageAddr] = shmid;

      *ResultAddress = Result;

      // We must get the shm size and track it
      struct shmid_ds buf {};

      if (shmctl(shmid, IPC_STAT, &buf) == 0) {
        // Map the new pages
        size_t NewPagesLength = buf.shm_segsz >> FEXCore::Utils::FEX_PAGE_SHIFT;
        SetUsedPages(NewPageAddr, NewPagesLength);
      }

      // Zero on working result
      Result = 0;
    } else {
      Result = -errno;
    }
    return Result;
  } else {
    // We must get the shm size and track it
    struct shmid_ds buf {};
    uint64_t PagesLength {};

    if (shmctl(shmid, IPC_STAT, &buf) == 0) {
      PagesLength = FEXCore::AlignUp(buf.shm_segsz, FEXCore::Utils::FEX_PAGE_SIZE) >> FEXCore::Utils::FEX_PAGE_SHIFT;
    } else {
      return -EINVAL;
    }

    bool Wrapped = false;
    uint64_t BottomPage = LastScanLocation;
restart: {
  // Linear range scan
  uint64_t LowerPage = (this->*FindPageRangePtr)(BottomPage, PagesLength);
  if (LowerPage == 0) {
    // Try again but this time from the start
    BottomPage = LastKeyLocation;
    LowerPage = (this->*FindPageRangePtr)(BottomPage, PagesLength);
  }

  uint64_t UpperPage = LowerPage + PagesLength;
  if (LowerPage == 0) {
    return -ENOMEM;
  }
  {
    // Try and map the range
    void* MappedPtr = ::shmat(shmid, reinterpret_cast<const void*>(LowerPage << FEXCore::Utils::FEX_PAGE_SHIFT), shmflg);

    if (MappedPtr == MAP_FAILED) {
      if (UpperPage == TOP_KEY) {
        BottomPage = LastKeyLocation;
        Wrapped = true;
        goto restart;
      } else if (Wrapped && LowerPage >= LastScanLocation) {
        // We linear scanned the entire memory range. Give up
        return -errno;
      } else {
        // Try again
        BottomPage += PagesLength;
        goto restart;
      }
    } else {
      if (SearchDown) {
        LastScanLocation = LowerPage;
      } else {
        LastScanLocation = UpperPage;
      }
      // Set the range as mapped
      SetUsedPages(LowerPage, PagesLength);

      *ResultAddress = reinterpret_cast<uint64_t>(MappedPtr);

      // Add to the map
      PageToShm[LowerPage] = shmid;

      // Zero on working result
      return 0;
    }
  }
}
  }
}
uint64_t MemAllocator32Bit::Shmdt(const void* shmaddr) {
  std::scoped_lock<std::mutex> lk {AllocMutex};

  uint32_t AddrPage = reinterpret_cast<uint64_t>(shmaddr) >> FEXCore::Utils::FEX_PAGE_SHIFT;
  auto it = PageToShm.find(AddrPage);

  if (it == PageToShm.end()) {
    // Page wasn't mapped
    return -EINVAL;
  }

  int shmid = it->second;
  struct shmid_ds buf {};
  if (shmctl(shmid, IPC_STAT, &buf) == 0) {
    size_t PagesLength = FEXCore::AlignUp(buf.shm_segsz, FEXCore::Utils::FEX_PAGE_SIZE) >> FEXCore::Utils::FEX_PAGE_SHIFT;
    SetFreePages(AddrPage, PagesLength);
  } else {
    LOGMAN_MSG_A_FMT("Failed to get shm size during shmdt");
  }

  uint64_t Result = ::shmdt(shmaddr);

  if (Result == 0) {
    PageToShm.erase(it);
  }

  SYSCALL_ERRNO();
}

class MemAllocatorPassThrough final : public FEX::HLE::MemAllocator {
public:
  void* Mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset) override {
    uint64_t Result = (uint64_t)::mmap(addr, length, prot, flags, fd, offset);
    if (Result == ~0ULL) {
      return reinterpret_cast<void*>(-errno);
    }
    return reinterpret_cast<void*>(Result);
  }

  int Munmap(void* addr, size_t length) override {
    uint64_t Result = (uint64_t)::munmap(addr, length);
    SYSCALL_ERRNO();
  }

  void* Mremap(void* old_address, size_t old_size, size_t new_size, int flags, void* new_address) override {
    uint64_t Result = (uint64_t)::mremap(old_address, old_size, new_size, flags, new_address);
    if (Result == ~0ULL) {
      return reinterpret_cast<void*>(-errno);
    }
    return reinterpret_cast<void*>(Result);
  }

  uint64_t Shmat(int shmid, const void* shmaddr, int shmflg, uint32_t* ResultAddress) override {
    uint64_t Result = (uint64_t)::shmat(shmid, reinterpret_cast<const void*>(shmaddr), shmflg);
    if (Result != ~0ULL) {
      *ResultAddress = Result;
      Result = 0;
    }
    SYSCALL_ERRNO();
  }

  uint64_t Shmdt(const void* shmaddr) override {
    uint64_t Result = ::shmdt(shmaddr);
    SYSCALL_ERRNO();
  }
};

fextl::unique_ptr<FEX::HLE::MemAllocator> Create32BitAllocator() {
  return fextl::make_unique<MemAllocator32Bit>();
}

fextl::unique_ptr<FEX::HLE::MemAllocator> CreatePassthroughAllocator() {
  return fextl::make_unique<MemAllocatorPassThrough>();
}

} // namespace FEX::HLE
