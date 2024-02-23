// SPDX-License-Identifier: MIT
#include "Utils/Allocator/HostAllocator.h"
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/memory_resource.h>
#include <FEXHeaderUtils/Syscalls.h>
#include <FEXHeaderUtils/TypeDefines.h>

#include <array>
#include <cctype>
#include <cstdio>
#include <fcntl.h>
#ifndef _WIN32
#include <sys/mman.h>
#include <sys/user.h>
#endif

#ifdef ENABLE_JEMALLOC
#include <jemalloc/jemalloc.h>
#endif
#include <errno.h>
#include <memory>
#include <stddef.h>
#include <stdint.h>

extern "C" {
typedef void* (*mmap_hook_type)(void* addr, size_t length, int prot, int flags, int fd, off_t offset);
typedef int (*munmap_hook_type)(void* addr, size_t length);

#ifdef ENABLE_JEMALLOC
extern mmap_hook_type je___mmap_hook;
extern munmap_hook_type je___munmap_hook;
#endif
}

namespace fextl::pmr {
static fextl::pmr::default_resource FEXDefaultResource;
std::pmr::memory_resource* get_default_resource() {
  return &FEXDefaultResource;
}
} // namespace fextl::pmr

#ifndef _WIN32
namespace FEXCore::Allocator {
MMAP_Hook mmap {::mmap};
MUNMAP_Hook munmap {::munmap};

uint64_t HostVASize {};

using GLIBC_MALLOC_Hook = void* (*)(size_t, const void* caller);
using GLIBC_REALLOC_Hook = void* (*)(void*, size_t, const void* caller);
using GLIBC_FREE_Hook = void (*)(void*, const void* caller);

fextl::unique_ptr<Alloc::HostAllocator> Alloc64 {};

void* FEX_mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset) {
  void* Result = Alloc64->Mmap(addr, length, prot, flags, fd, offset);
  if (Result >= (void*)-4096) {
    errno = -(uint64_t)Result;
    return (void*)-1;
  }
  return Result;
}
int FEX_munmap(void* addr, size_t length) {
  int Result = Alloc64->Munmap(addr, length);

  if (Result != 0) {
    errno = -Result;
    return -1;
  }
  return Result;
}

// This function disables glibc's ability to allocate memory through the `sbrk` interface.
// This is run early in the lifecycle of FEX in order to make sure no 64-bit pointers can make it to the guest 32-bit application.
//
// How this works is that this allocates a single page at the current sbrk pointer (aligned upward to page size). This makes it
// so that when the sbrk syscall is used to allocate more memory, it fails with an ENOMEM since it runs in to the allocated guard page.
//
// glibc notices the sbrk failure and falls back to regular mmap based allocations when this occurs. Ensuring that memory can still be allocated.
void* DisableSBRKAllocations() {
  void* INVALID_PTR = reinterpret_cast<void*>(~0ULL);
  // Get the starting sbrk pointer.
  void* StartingSBRK = sbrk(0);
  if (StartingSBRK == INVALID_PTR) {
    // If sbrk is already returning invalid pointers then nothing to do here.
    return INVALID_PTR;
  }

  // Now allocate the next page after the sbrk address to ensure it can't grow.
  // In most cases at the start of `main` this will already be page aligned, which means subsequent `sbrk`
  // calls won't allocate any memory through that.
  void* AlignedBRK = reinterpret_cast<void*>(FEXCore::AlignUp(reinterpret_cast<uintptr_t>(StartingSBRK), FHU::FEX_PAGE_SIZE));
  void* AfterBRK = mmap(AlignedBRK, FHU::FEX_PAGE_SIZE, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE | MAP_NORESERVE, -1, 0);
  if (AfterBRK == INVALID_PTR) {
    // Couldn't allocate the page after the aligned brk? This should never happen.
    // FEXCore::LogMan isn't configured yet so we just need to print the message.
    fextl::fmt::print("Couldn't allocate page after SBRK.\n");
    FEX_TRAP_EXECUTION;
    return INVALID_PTR;
  }

  // Now that the page after sbrk is allocated, FEX needs to consume the remaining sbrk space.
  // This will be anywhere from [0, 4096) bytes.
  // Start allocating from 1024 byte increments just to make any steps a bit faster.
  intptr_t IncrementAmount = 1024;
  for (; IncrementAmount != 0; IncrementAmount >>= 1) {
    while (sbrk(IncrementAmount) != INVALID_PTR)
      ;
  }
  return AlignedBRK;
}

void ReenableSBRKAllocations(void* Ptr) {
  const void* INVALID_PTR = reinterpret_cast<void*>(~0ULL);
  if (Ptr != INVALID_PTR) {
    munmap(Ptr, FHU::FEX_PAGE_SIZE);
  }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
void SetupHooks() {
  Alloc64 = Alloc::OSAllocator::Create64BitAllocator();
#ifdef ENABLE_JEMALLOC
  je___mmap_hook = FEX_mmap;
  je___munmap_hook = FEX_munmap;
#endif
  FEXCore::Allocator::mmap = FEX_mmap;
  FEXCore::Allocator::munmap = FEX_munmap;
}

void ClearHooks() {
#ifdef ENABLE_JEMALLOC
  je___mmap_hook = ::mmap;
  je___munmap_hook = ::munmap;
#endif
  FEXCore::Allocator::mmap = ::mmap;
  FEXCore::Allocator::munmap = ::munmap;

  // XXX: This is currently a leak.
  // We can't work around this yet until static initializers that allocate memory are completely removed from our codebase
  // Luckily we only remove this on process shutdown, so the kernel will do the cleanup for us
  Alloc64.release();
}
#pragma GCC diagnostic pop

FEX_DEFAULT_VISIBILITY size_t DetermineVASize() {
  if (HostVASize) {
    return HostVASize;
  }

  static constexpr std::array<uintptr_t, 7> TLBSizes = {
    57, 52, 48, 47, 42, 39, 36,
  };

  for (auto Bits : TLBSizes) {
    uintptr_t Size = 1ULL << Bits;
    // Just try allocating
    // We can't actually determine VA size on ARM safely
    auto Find = [](uintptr_t Size) -> bool {
      for (int i = 0; i < 64; ++i) {
        // Try grabbing a some of the top pages of the range
        // x86 allocates some high pages in the top end
        void* Ptr = ::mmap(reinterpret_cast<void*>(Size - FHU::FEX_PAGE_SIZE * i), FHU::FEX_PAGE_SIZE, PROT_NONE,
                           MAP_FIXED_NOREPLACE | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (Ptr != (void*)~0ULL) {
          ::munmap(Ptr, FHU::FEX_PAGE_SIZE);
          if (Ptr == (void*)(Size - FHU::FEX_PAGE_SIZE * i)) {
            return true;
          }
        }
      }
      return false;
    };

    if (Find(Size)) {
      HostVASize = Bits;
      return Bits;
    }
  }

  LOGMAN_MSG_A_FMT("Couldn't determine host VA size");
  FEX_UNREACHABLE;
}

#define STEAL_LOG(...) // fprintf(stderr, __VA_ARGS__)

fextl::vector<MemoryRegion> StealMemoryRegion(uintptr_t Begin, uintptr_t End) {
  void* const StackLocation = alloca(0);
  const uintptr_t StackLocation_u64 = reinterpret_cast<uintptr_t>(StackLocation);
  fextl::vector<MemoryRegion> Regions;

  int MapsFD = open("/proc/self/maps", O_RDONLY);
  LogMan::Throw::AFmt(MapsFD != -1, "Failed to open /proc/self/maps");

  enum { ParseBegin, ParseEnd, ScanEnd } State = ParseBegin;

  uintptr_t RegionBegin = 0;
  uintptr_t RegionEnd = 0;

  uintptr_t PreviousMapEnd = 0;

  char Buffer[2048];
  const char* Cursor;
  ssize_t Remaining = 0;

  for (;;) {

    if (Remaining == 0) {
      do {
        Remaining = read(MapsFD, Buffer, sizeof(Buffer));
      } while (Remaining == -1 && errno == EAGAIN);

      Cursor = Buffer;
    }

    if (Remaining == 0 && State == ParseBegin) {
      STEAL_LOG("[%d] EndOfFile; RegionBegin: %016lX RegionEnd: %016lX\n", __LINE__, RegionBegin, RegionEnd);

      const auto MapBegin = std::max(RegionEnd, Begin);
      const auto MapEnd = End;

      STEAL_LOG("     MapBegin: %016lX MapEnd: %016lX\n", MapBegin, MapEnd);

      if (MapEnd > MapBegin) {
        STEAL_LOG("     Reserving\n");

        auto MapSize = MapEnd - MapBegin;
        auto Alloc = mmap((void*)MapBegin, MapSize, PROT_NONE, MAP_ANONYMOUS | MAP_NORESERVE | MAP_PRIVATE | MAP_FIXED_NOREPLACE, -1, 0);

        LogMan::Throw::AFmt(Alloc != MAP_FAILED, "mmap({:x},{:x}) failed", MapBegin, MapSize);
        LogMan::Throw::AFmt(Alloc == (void*)MapBegin, "mmap({},{:x}) returned {} instead of {:x}", Alloc, MapBegin);

        Regions.push_back({(void*)MapBegin, MapSize});
      }

      close(MapsFD);
      return Regions;
    }

    LogMan::Throw::AFmt(Remaining > 0, "Failed to parse /proc/self/maps");

    auto c = *Cursor++;
    Remaining--;

    if (State == ScanEnd) {
      if (c == '\n') {
        State = ParseBegin;
      }
      continue;
    }

    if (State == ParseBegin) {
      if (c == '-') {
        STEAL_LOG("[%d] ParseBegin; RegionBegin: %016lX RegionEnd: %016lX\n", __LINE__, RegionBegin, RegionEnd);

        const auto MapBegin = std::max(RegionEnd, Begin);
        const auto MapEnd = std::min(RegionBegin, End);

        // Store the location we are going to map.
        PreviousMapEnd = MapEnd;

        STEAL_LOG("     MapBegin: %016lX MapEnd: %016lX\n", MapBegin, MapEnd);

        if (MapEnd > MapBegin) {
          STEAL_LOG("     Reserving\n");

          auto MapSize = MapEnd - MapBegin;
          auto Alloc = mmap((void*)MapBegin, MapSize, PROT_NONE, MAP_ANONYMOUS | MAP_NORESERVE | MAP_PRIVATE | MAP_FIXED_NOREPLACE, -1, 0);

          LogMan::Throw::AFmt(Alloc != MAP_FAILED, "mmap({:x},{:x}) failed", MapBegin, MapSize);
          LogMan::Throw::AFmt(Alloc == (void*)MapBegin, "mmap({},{:x}) returned {} instead of {:x}", Alloc, MapBegin);

          Regions.push_back({(void*)MapBegin, MapSize});
        }

        RegionBegin = 0;
        RegionEnd = 0;
        State = ParseEnd;
        continue;
      } else {
        LogMan::Throw::AFmt(std::isalpha(c) || std::isdigit(c), "Unexpected char '{}' in ParseBegin", c);
        RegionBegin = (RegionBegin << 4) | (c <= '9' ? (c - '0') : (c - 'a' + 10));
      }
    }

    if (State == ParseEnd) {
      if (c == ' ') {
        STEAL_LOG("[%d] ParseEnd; RegionBegin: %016lX RegionEnd: %016lX\n", __LINE__, RegionBegin, RegionEnd);

        State = ScanEnd;

        // If the previous map's ending and the region we just parsed overlap the stack then we need to save the stack mapping.
        // Otherwise we will have severely limited stack size which crashes quickly.
        if (PreviousMapEnd <= StackLocation_u64 && RegionEnd > StackLocation_u64) {
          auto BelowStackRegion = Regions.back();
          LOGMAN_THROW_AA_FMT(reinterpret_cast<uint64_t>(BelowStackRegion.Ptr) + BelowStackRegion.Size == PreviousMapEnd, "This needs to "
                                                                                                                          "match");

          // Allocate the region under the stack as READ | WRITE so the stack can still grow
          auto Alloc = mmap(BelowStackRegion.Ptr, BelowStackRegion.Size, PROT_READ | PROT_WRITE,
                            MAP_ANONYMOUS | MAP_NORESERVE | MAP_PRIVATE | MAP_FIXED, -1, 0);

          LogMan::Throw::AFmt(Alloc != MAP_FAILED, "mmap({:x},{:x}) failed", BelowStackRegion.Ptr, BelowStackRegion.Size);
          LogMan::Throw::AFmt(Alloc == BelowStackRegion.Ptr, "mmap({},{:x}) returned {} instead of {:x}", Alloc, BelowStackRegion.Ptr);

          Regions.pop_back();
        }
        continue;
      } else {
        LogMan::Throw::AFmt(std::isalpha(c) || std::isdigit(c), "Unexpected char '{}' in ParseEnd", c);
        RegionEnd = (RegionEnd << 4) | (c <= '9' ? (c - '0') : (c - 'a' + 10));
      }
    }
  }

  ERROR_AND_DIE_FMT("unreachable");
}

fextl::vector<MemoryRegion> Steal48BitVA() {
  size_t Bits = FEXCore::Allocator::DetermineVASize();
  if (Bits < 48) {
    return {};
  }

  uintptr_t Begin48BitVA = 0x0'8000'0000'0000ULL;
  uintptr_t End48BitVA = 0x1'0000'0000'0000ULL;
  return StealMemoryRegion(Begin48BitVA, End48BitVA);
}

void ReclaimMemoryRegion(const fextl::vector<MemoryRegion>& Regions) {
  for (const auto& Region : Regions) {
    ::munmap(Region.Ptr, Region.Size);
  }
}

void LockBeforeFork(FEXCore::Core::InternalThreadState* Thread) {
  if (Alloc64) {
    Alloc64->LockBeforeFork(Thread);
  }
}

void UnlockAfterFork(FEXCore::Core::InternalThreadState* Thread, bool Child) {
  if (Alloc64) {
    Alloc64->UnlockAfterFork(Thread, Child);
  }
}
} // namespace FEXCore::Allocator
#endif
