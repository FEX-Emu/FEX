// SPDX-License-Identifier: MIT
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include "Utils/Allocator/FlexBitSet.h"
#include "Utils/Allocator/HostAllocator.h"
#include <FEXCore/Utils/Allocator.h>
#include <sys/mman.h>

template<typename T>
bool HasSyscallError(T Result) {
  constexpr uint64_t MAX_ERRNO = 0xFFFF'FFFF'FFFF'0001ULL;
  return reinterpret_cast<uint64_t>(Result) >= MAX_ERRNO;
}

TEST_CASE("FlexBitSet - Fixed replacement") {
  const auto RegionSize = 128 * 1024 * 1024;
  fextl::vector<FEXCore::Allocator::MemoryRegion> MemoryRegions {};
  for (size_t i = 0; i < 2; ++i) {
    auto Ptr = mmap(nullptr, RegionSize, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    MemoryRegions.emplace_back(FEXCore::Allocator::MemoryRegion {
      .Ptr = Ptr,
      .Size = RegionSize,
    });
  }

  auto Allocator = Alloc::OSAllocator::Create64BitAllocatorWithRegions(MemoryRegions);
  auto Base = Allocator->Mmap(nullptr, 4096, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  REQUIRE(!HasSyscallError(Base));

  // Allocate perfectly overlapping pages. Allocate as many pages as the region.
  // FEX had a bug where the allocator could run out of memory with MAP_FIXED.
  for (size_t i = 0; i < (RegionSize / 4096); ++i) {
    auto NewBase = Allocator->Mmap(Base, 4096, PROT_NONE, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    REQUIRE(Base == NewBase);
  }

  // Leak the memory for now. Crashes if we try.
  Allocator.release();
}
