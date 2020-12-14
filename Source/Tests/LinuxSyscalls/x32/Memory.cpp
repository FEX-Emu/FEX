#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"

#include "Common/MathUtils.h"

#include <bitset>
#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>

namespace FEX::HLE::x32 {
class MemAllocator {
private:
  static constexpr uint64_t PAGE_SHIFT = 12;
  static constexpr uint64_t PAGE_SIZE = 1 << PAGE_SHIFT;
  static constexpr uint64_t PAGE_MASK = (1 << PAGE_SHIFT) - 1;
  static constexpr uint64_t BASE_KEY = 16;
  const uint64_t TOP_KEY = 0xFFFF'F000ULL >> PAGE_SHIFT;

public:
  MemAllocator() {
    // First 16 pages are taken by the Linux kernel
    for (size_t i = 0; i < 16; ++i) {
      MappedPages.set(i);
    }
    // Take the top page as well
    MappedPages.set(TOP_KEY);
  }
  void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
  int munmap(void *addr, size_t length);
  void *mremap(void *old_address, size_t old_size, size_t new_size, int flags, void *new_address);

private:
  // Set that contains 4k mapped pages
  // This is the full 32bit memory range
  std::bitset<0x10'0000> MappedPages;
  uint64_t LastScanLocation = BASE_KEY;
  std::mutex AllocMutex{};
  uint64_t FindPageRange(uint64_t Start, size_t Pages);
  uint64_t FindPageRange_TopDown(uint64_t Start, size_t Pages);
};

uint64_t MemAllocator::FindPageRange(uint64_t Start, size_t Pages) {
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

uint64_t MemAllocator::FindPageRange_TopDown(uint64_t Start, size_t Pages) {
  // Linear range scan
  Start -= Pages;
  while (Start != BASE_KEY) {
    bool Free = true;
    if (Start < BASE_KEY) {
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
    Start -= (Pages - Offset) + 1;
  }

  return 0;
}

void *MemAllocator::mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
  std::scoped_lock<std::mutex> lk{AllocMutex};
  size_t PagesLength = AlignUp(length, PAGE_SIZE) >> PAGE_SHIFT;

  uintptr_t Addr = reinterpret_cast<uintptr_t>(addr);
  uintptr_t PageAddr = AlignUp(Addr, PAGE_SIZE) >> PAGE_SHIFT;

  uintptr_t PageEnd = PageAddr + PagesLength;

  bool Fixed = ((flags & MAP_FIXED) ||
      (flags & MAP_FIXED_NOREPLACE));

  // Both Addr and length must be page aligned
  if (Addr & PAGE_MASK) {
    return (void*)-EINVAL;
  }

  // If we do have an fd then offset must be page aligned
  if (fd != -1 &&
      offset & PAGE_MASK) {
    return (void*)-EINVAL;
  }

  if (Addr + length > std::numeric_limits<uint32_t>::max()) {
    return (void*)-EOVERFLOW;
  }

  // Check reserved range
  if (Fixed && PageAddr < 16) {
    return (void*)-EINVAL;
  }

  if (!Fixed) {
    // If we aren't mapping fixed the ignore the address input
    Addr = 0;
    PageAddr = 0;
    PageEnd = PagesLength;
  }

  // Find a region that fits our address
  if (Addr == 0) {
    bool Wrapped = false;
    uint64_t BottomPage = LastScanLocation;
restart:
    {
      // Linear range scan
      uint64_t LowerPage = FindPageRange(BottomPage, PagesLength);
      if (LowerPage == 0) {
        // Try again but this time from the start
        BottomPage = BASE_KEY;
        LowerPage = FindPageRange(BottomPage, PagesLength);
      }

      uint64_t UpperPage = LowerPage + PagesLength;
      if (LowerPage == 0) {
        return (void*)(uintptr_t)-ENOMEM;
      }
      {
        // Try and map the range
        void *MappedPtr = ::mmap(
          reinterpret_cast<void*>(LowerPage<< PAGE_SHIFT),
          length,
          prot,
          flags | MAP_FIXED_NOREPLACE,
          fd,
          offset);

        if (MappedPtr == MAP_FAILED) {
          if (UpperPage == TOP_KEY) {
            BottomPage = BASE_KEY;
            Wrapped = true;
            goto restart;
          }
          else if (Wrapped &&
            LowerPage >= LastScanLocation) {
            // We linear scanned the entire memory range. Give up
            return (void*)(uintptr_t)-errno;
          }
          else {
            // Try again
            BottomPage += PagesLength;
            goto restart;
          }
        }
        else {
          LastScanLocation = UpperPage;
          // Set the range as mapped
          for (size_t i = 0; i < PagesLength; ++i) {
            MappedPages.set(LowerPage + i);
          }
          return MappedPtr;
        }
      }
    }
  }
  else {
    void *MappedPtr = ::mmap(
      reinterpret_cast<void*>(PageAddr << PAGE_SHIFT),
      PagesLength << PAGE_SHIFT,
      prot,
      flags,
      fd,
      offset);

    if (MappedPtr != MAP_FAILED) {
      for (size_t i = 0; i < PagesLength; ++i) {
        MappedPages.set(PageAddr + i);
      }
      return MappedPtr;
    }
    else {
      return (void*)(uintptr_t)-errno;
    }
  }
  return 0;
}

int MemAllocator::munmap(void *addr, size_t length) {
  std::scoped_lock<std::mutex> lk{AllocMutex};
  size_t PagesLength = AlignUp(length, PAGE_SIZE) >> PAGE_SHIFT;

  uintptr_t Addr = reinterpret_cast<uintptr_t>(addr);
  uintptr_t PageAddr = Addr >> PAGE_SHIFT;

  uintptr_t PageEnd = PageAddr + PagesLength;

  // Both Addr and length must be page aligned
  if (Addr & PAGE_MASK) {
    return -EINVAL;
  }

  if (length & PAGE_MASK) {
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
    int Result = ::munmap(reinterpret_cast<void*>(PageAddr << PAGE_SHIFT), PAGE_SIZE);
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

void *MemAllocator::mremap(void *old_address, size_t old_size, size_t new_size, int flags, void *new_address) {
  std::scoped_lock<std::mutex> lk{AllocMutex};
  // XXX: Not currently supported
  return reinterpret_cast<void*>(-ENOMEM);
}

  static std::unique_ptr<MemAllocator> alloc{};
  void RegisterMemory() {
    alloc = std::make_unique<MemAllocator>();

    REGISTER_SYSCALL_IMPL_X32(mmap, [](FEXCore::Core::InternalThreadState *Thread, uint32_t addr, uint32_t length, int prot, int flags, int fd, int32_t offset) -> uint64_t {
      return (uint64_t)alloc->mmap(reinterpret_cast<void*>(addr), length, prot,flags, fd, offset);
    });

    REGISTER_SYSCALL_IMPL_X32(mmap2, [](FEXCore::Core::InternalThreadState *Thread, uint32_t addr, uint32_t length, int prot, int flags, int fd, int32_t pgoffset) -> uint64_t {
      return (uint64_t)alloc->mmap(reinterpret_cast<void*>(addr), length, prot,flags, fd, pgoffset * 0x1000);
    });

    REGISTER_SYSCALL_IMPL_X32(munmap, [](FEXCore::Core::InternalThreadState *Thread, void *addr, size_t length) -> uint64_t {
      return alloc->munmap(addr, length);
    });

    REGISTER_SYSCALL_IMPL_X32(mprotect, [](FEXCore::Core::InternalThreadState *Thread, void *addr, uint32_t len, int prot) -> uint64_t {
      uint64_t Result = ::mprotect(addr, len, prot);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(mremap, [](FEXCore::Core::InternalThreadState *Thread, void *old_address, size_t old_size, size_t new_size, int flags, void *new_address) -> uint64_t {
      return reinterpret_cast<uint64_t>(alloc->mremap(old_address, old_size, new_size, flags, new_address));
    });

    REGISTER_SYSCALL_IMPL_X32(mlockall, [](FEXCore::Core::InternalThreadState *Thread, int flags) -> uint64_t {
      uint64_t Result = ::mlock2(reinterpret_cast<void*>(0x1'0000), 0x1'0000'0000ULL - 0x1'0000, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(munlockall, [](FEXCore::Core::InternalThreadState *Thread) -> uint64_t {
      uint64_t Result = ::munlock(reinterpret_cast<void*>(0x1'0000), 0x1'0000'0000ULL - 0x1'0000);
      SYSCALL_ERRNO();
    });
  }

}
