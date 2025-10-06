// SPDX-License-Identifier: MIT
#include "Utils/Allocator/HostAllocator.h"
#include "Utils/PrctlUtils.h"
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/MathUtils.h>
#include <FEXCore/Utils/TypeDefines.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/memory_resource.h>
#include <FEXHeaderUtils/Syscalls.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#ifndef _WIN32
#include <sys/mman.h>
#include <sys/user.h>
#endif

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

  if (flags & MAP_ANONYMOUS) {
    prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME, Result, length, "FEXMem");
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
  void* AlignedBRK = reinterpret_cast<void*>(FEXCore::AlignUp(reinterpret_cast<uintptr_t>(StartingSBRK), FEXCore::Utils::FEX_PAGE_SIZE));
  void* AfterBRK =
    mmap(AlignedBRK, FEXCore::Utils::FEX_PAGE_SIZE, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE | MAP_NORESERVE, -1, 0);
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
    munmap(Ptr, FEXCore::Utils::FEX_PAGE_SIZE);
  }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
static void AssignHookOverrides() {
  SetJemallocMmapHook(FEX_mmap);
  SetJemallocMunmapHook(FEX_munmap);
  FEXCore::Allocator::mmap = FEX_mmap;
  FEXCore::Allocator::munmap = FEX_munmap;
}

void SetupHooks() {
  Alloc64 = Alloc::OSAllocator::Create64BitAllocator();
  AssignHookOverrides();
}

void ClearHooks() {
  SetJemallocMmapHook(::mmap);
  SetJemallocMunmapHook(::munmap);
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
        void* Ptr = ::mmap(reinterpret_cast<void*>(Size - FEXCore::Utils::FEX_PAGE_SIZE * i), FEXCore::Utils::FEX_PAGE_SIZE, PROT_NONE,
                           MAP_FIXED_NOREPLACE | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (Ptr != (void*)~0ULL) {
          ::munmap(Ptr, FEXCore::Utils::FEX_PAGE_SIZE);
          if (Ptr == (void*)(Size - FEXCore::Utils::FEX_PAGE_SIZE * i)) {
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

fextl::vector<MemoryRegion> CollectMemoryGaps(uintptr_t Begin, uintptr_t End, int MapsFD) {
  fextl::vector<MemoryRegion> Regions;

  uintptr_t RegionEnd = 0;

  char Buffer[2048];
  const char* Cursor = Buffer;
  ssize_t Remaining = 0;

  bool EndOfFileReached = false;

  while (true) {
    const auto line_begin = Cursor;
    auto line_end = std::find(line_begin, Cursor + Remaining, '\n');

    // Check if the buffered data covers the entire line.
    // If not, try buffering more data.
    if (line_end == Cursor + Remaining) {
      if (EndOfFileReached) {
        // No more data to buffer. Add remaining memory and return.
        const auto MapBegin = std::max(RegionEnd, Begin);
        STEAL_LOG("[%d] EndOfFile; MapBegin: %016lX MapEnd: %016lX\n", __LINE__, MapBegin, End);
        if (End > MapBegin) {
          Regions.push_back({(void*)MapBegin, End - MapBegin});
        }

        return Regions;
      }

      // Move pending content back to the beginning, then buffer more data.
      std::copy(Cursor, Cursor + Remaining, std::begin(Buffer));
      auto PendingBytes = Remaining;
      do {
        Remaining = read(MapsFD, Buffer + PendingBytes, sizeof(Buffer) - PendingBytes);
      } while (Remaining == -1 && errno == EAGAIN);

      if (Remaining < sizeof(Buffer) - PendingBytes) {
        EndOfFileReached = true;
      }

      Remaining += PendingBytes;

      Cursor = Buffer;
      continue;
    }

    // Parse mapped region in the format "fffff7cc3000-fffff7cc4000 r--p ..."
    {
      uintptr_t RegionBegin {};
      auto result = std::from_chars(Cursor, line_end, RegionBegin, 16);
      LogMan::Throw::AFmt(result.ec == std::errc {} && *result.ptr == '-', "Unexpected line format");
      Cursor = result.ptr + 1;

      // Add gap between the previous region and the current one
      const auto MapBegin = std::max(RegionEnd, Begin);
      const auto MapEnd = std::min(RegionBegin, End);
      if (MapEnd > MapBegin) {
        Regions.push_back({(void*)MapBegin, MapEnd - MapBegin});
      }

      result = std::from_chars(Cursor, line_end, RegionEnd, 16);
      LogMan::Throw::AFmt(result.ec == std::errc {} && *result.ptr == ' ', "Unexpected line format");
      Cursor = result.ptr + 1;

      STEAL_LOG("[%d] parsed line: RegionBegin=%016lX RegionEnd=%016lX\n", __LINE__, RegionBegin, RegionEnd);

      if (RegionEnd >= End) {
        // Early return if we are completely beyond the allocation space.
        return Regions;
      }
    }

    Remaining -= line_end + 1 - line_begin;
    Cursor = line_end + 1;
  }
  FEX_UNREACHABLE;
}

fextl::vector<MemoryRegion> StealMemoryRegion(uintptr_t Begin, uintptr_t End) {
  const uintptr_t StackLocation_u64 = reinterpret_cast<uintptr_t>(alloca(0));

  const int MapsFD = open("/proc/self/maps", O_RDONLY);
  LogMan::Throw::AFmt(MapsFD != -1, "Failed to open /proc/self/maps");

  auto Regions = CollectMemoryGaps(Begin, End, MapsFD);
  close(MapsFD);

  // If the memory bounds include the stack, blocking all memory regions will
  // limit the stack size to the current value. To allow some stack growth,
  // we don't block the memory gap directly below the stack memory but
  // instead map it as readable+writable.
  {
    auto StackRegionIt = std::find_if(Regions.begin(), Regions.end(), [StackLocation_u64](auto& Region) {
      return reinterpret_cast<uintptr_t>(Region.Ptr) + Region.Size > StackLocation_u64;
    });

    // If no gap crossing the stack pointer was found but the SP is within
    // the given bounds, the stack mapping is right after the last gap.
    bool IsStackMapping = StackRegionIt != Regions.end() || StackLocation_u64 <= End;

    if (IsStackMapping && StackRegionIt != Regions.begin() &&
        reinterpret_cast<uintptr_t>(std::prev(StackRegionIt)->Ptr) + std::prev(StackRegionIt)->Size <= End) {
      // Allocate the region under the stack as READ | WRITE so the stack can still grow
      --StackRegionIt;

      auto Alloc =
        mmap(StackRegionIt->Ptr, StackRegionIt->Size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_NORESERVE | MAP_PRIVATE | MAP_FIXED, -1, 0);

      LogMan::Throw::AFmt(Alloc != MAP_FAILED, "mmap({},{:x}) failed", fmt::ptr(StackRegionIt->Ptr), StackRegionIt->Size);
      LogMan::Throw::AFmt(Alloc == StackRegionIt->Ptr, "mmap returned {} instead of {}", Alloc, fmt::ptr(StackRegionIt->Ptr));

      Regions.erase(StackRegionIt);
    }
  }

  // Block remaining memory gaps
  for (auto RegionIt = Regions.begin(); RegionIt != Regions.end(); ++RegionIt) {
    auto Alloc = mmap(RegionIt->Ptr, RegionIt->Size, PROT_NONE, MAP_ANONYMOUS | MAP_NORESERVE | MAP_PRIVATE | MAP_FIXED_NOREPLACE, -1, 0);

    LogMan::Throw::AFmt(Alloc != MAP_FAILED, "mmap({},{:x}) failed", fmt::ptr(RegionIt->Ptr), RegionIt->Size);
    LogMan::Throw::AFmt(Alloc == RegionIt->Ptr, "mmap returned {} instead of {}", Alloc, fmt::ptr(RegionIt->Ptr));
  }

  return Regions;
}

fextl::vector<MemoryRegion> Setup48BitAllocatorIfExists() {
  size_t Bits = FEXCore::Allocator::DetermineVASize();
  if (Bits < 48) {
    return {};
  }

  uintptr_t Begin48BitVA = 0x0'8000'0000'0000ULL;
  uintptr_t End48BitVA = 0x1'0000'0000'0000ULL;
  auto Regions = StealMemoryRegion(Begin48BitVA, End48BitVA);

  Alloc64 = Alloc::OSAllocator::Create64BitAllocatorWithRegions(Regions);
  AssignHookOverrides();

  return Regions;
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
