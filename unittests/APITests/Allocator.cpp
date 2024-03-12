#include <catch2/catch_all.hpp>
#include <FEXCore/Utils/Allocator.h>

namespace {

using FEXCore::Allocator::MemoryRegion;

static std::optional<MemoryRegion> LastStackMapping;
void* MmapOverride(void* Ptr, size_t Size, int Flags, int, int, __off_t) {
  if (Flags != PROT_NONE) {
    LastStackMapping = MemoryRegion { Ptr, Size };
  }
  return Ptr;
}

struct Fixture {
  Fixture() {
    fd = mkstemp(filename);
    if (fd == -1) {
      std::abort();
    }
  }

  ~Fixture() {
    close(fd);
    remove(filename);
  }

  fextl::vector<FEXCore::Allocator::MemoryRegion> StealMemoryRegion(std::string_view Input, uintptr_t Begin, uintptr_t End, std::optional<uintptr_t> StackBottom = std::nullopt) {
    // Reload input, or just create all possible inputs as file and then select the fd instead
    lseek(fd, 0, SEEK_SET);
    write(fd, Input.data(), Input.size());
    lseek(fd, 0, SEEK_SET);
    LastStackMapping.reset();
    return FEXCore::Allocator::StealMemoryRegion(Begin, End, fd, MmapOverride, reinterpret_cast<void*>(StackBottom.value_or(0xffffffffffffffff)));
  }

  char filename[64] = P_tmpdir "/alloctestXXXXXX";
  int fd;
};

MemoryRegion FromTo(uintptr_t Start, uintptr_t End) {
  return MemoryRegion { reinterpret_cast<void*>(Start), End - Start };
}

} // anonymous namespace

namespace FEXCore::Allocator {
bool operator==(const MemoryRegion& a, const MemoryRegion& b) {
  return a.Ptr == b.Ptr && a.Size == b.Size;
}

inline std::ostream& operator<<(std::ostream& os, MemoryRegion region) {
  os << std::hex << region.Ptr << "-" << reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(region.Ptr) + region.Size);
  return os;
}

inline std::ostream& operator<<(std::ostream& os, fextl::vector<MemoryRegion> regions) {
  os << "{";
  bool first = true;
  for (auto& region : regions) {
    if (!first) {
      os << ", ";
    }
    first = false;
    os << region;
  }
  os << "}";
  return os;
}
}

TEST_CASE_METHOD(Fixture, "Trivial") {
  // Single entry covering exactly 2 pages of memory
  const char SingletonMappings[] =
    "000000100000-000000102000 r--p 00000000 00:00 0                          placeholder\n";

  auto Begin = GENERATE(0, 0xff000, 0x100000, 0x101000, 0x102000);
  auto End = GENERATE(0xff000, 0x100000, 0x101000, 0x102000, 0x103000);
  if (Begin >= End) {
    return;
  }

  auto Mappings = StealMemoryRegion(SingletonMappings, Begin, End);
  INFO("StealMemoryRegion 0x" << std::hex << Begin << "-0x" << End);

  if (Begin < 0x100000 && End == 0x103000) {
    CHECK_THAT(Mappings, Catch::Matchers::Equals(fextl::vector<MemoryRegion> { FromTo(Begin, 0x100000), FromTo(0x102000, 0x103000) }));
  } else if (Begin < 0x100000 && End < 0x100000) {
    CHECK_THAT(Mappings, Catch::Matchers::Equals(fextl::vector<MemoryRegion> { FromTo(Begin, End) }));
  } else if (Begin < 0x100000 && End <= 0x102000) {
    CHECK_THAT(Mappings, Catch::Matchers::Equals(fextl::vector<MemoryRegion> { FromTo(Begin, 0x100000) }));
  } else if (End != 0x103000) {
    CHECK_THAT(Mappings, Catch::Matchers::Equals(fextl::vector<MemoryRegion> { }));
  } else {
    // Begin >= 0x100000 and End == 0x103000
    CHECK_THAT(Mappings, Catch::Matchers::Equals(fextl::vector<MemoryRegion> { FromTo(0x102000, End) }));
  }
}

TEST_CASE_METHOD(Fixture, "RealWorld") {
  const char RealWorldMappings[] =
    "aaaaaaaa0000-aaaaaadba000 r--p 00000000 00:00 0                          placeholder\n"
    "aaaaaadc9000-aaaaab77a000 r-xp 00000000 00:00 0                          placeholder\n"
    "aaaaab789000-aaaaab7b7000 r--p 00000000 00:00 0                          placeholder\n"
    "aaaaab7c6000-aaaaab894000 rw-p 00000000 00:00 0                          placeholder\n"
    "aaaaab894000-aaaaabcc9000 rw-p 00000000 00:00 0                          placeholder\n"
    "aaaaabcc9000-aaaaabcca000 ---p 00000000 00:00 0                          placeholder\n"
    "fffff6a00000-fffff7a00000 rw-p 00000000 00:00 0\n"
    "fffff7af0000-fffff7c78000 r-xp 00000000 00:00 0                          placeholder\n"
    "fffff7c78000-fffff7c87000 ---p 00000000 00:00 0                          placeholder\n"
    "fffff7c87000-fffff7c8b000 r--p 00000000 00:00 0                          placeholder\n"
    "fffff7c8b000-fffff7c8d000 rw-p 00000000 00:00 0                          placeholder\n"
    "fffff7c8d000-fffff7c99000 rw-p 00000000 00:00 0\n"
    "fffff7ca0000-fffff7cb4000 r-xp 00000000 00:00 0                          placeholder\n"
    "fffff7cb4000-fffff7cc3000 ---p 00000000 00:00 0                          placeholder\n"
    "fffff7cc3000-fffff7cc4000 r--p 00000000 00:00 0                          placeholder\n"
    "fffff7cc4000-fffff7cc5000 rw-p 00000000 00:00 0                          placeholder\n"
    "fffff7cd0000-fffff7d56000 r-xp 00000000 00:00 0                          placeholder\n"
    "fffff7d56000-fffff7d65000 ---p 00000000 00:00 0                          placeholder\n"
    "fffff7d65000-fffff7d66000 r--p 00000000 00:00 0                          placeholder\n"
    "fffff7d66000-fffff7d67000 rw-p 00000000 00:00 0                          placeholder\n"
    "fffff7d70000-fffff7f7a000 r-xp 00000000 00:00 0                          placeholder\n"
    "fffff7f7a000-fffff7f89000 ---p 00000000 00:00 0                          placeholder\n"
    "fffff7f89000-fffff7f94000 r--p 00000000 00:00 0                          placeholder\n"
    "fffff7f94000-fffff7f97000 rw-p 00000000 00:00 0                          placeholder\n"
    "fffff7f97000-fffff7f9a000 rw-p 00000000 00:00 0\n"
    "fffff7fc2000-fffff7fed000 r-xp 00000000 00:00 0                          placeholder\n"
    "fffff7fef000-fffff7ff9000 rw-p 00000000 00:00 0\n"
    "fffff7ff9000-fffff7ffb000 r--p 00000000 00:00 0                          placeholder\n"
    "fffff7ffb000-fffff7ffc000 r-xp 00000000 00:00 0                          placeholder\n"
    "fffff7ffc000-fffff7ffe000 r--p 00000000 00:00 0                          placeholder\n"
    "fffff7ffe000-fffff8000000 rw-p 00000000 00:00 0                          placeholder\n"
    "fffffffd2000-1000000000000 rw-p 00000000 00:00 0                         [stack]\n";

  using namespace Catch::Generators;
  uintptr_t Begin = GENERATE(take(30, random<uintptr_t>(0, 0xffffffffffffffff / 0x1000))) * 0x1000;
  uintptr_t End = GENERATE(take(30, random<uintptr_t>(0, 0xffffffffffffffff / 0x1000))) * 0x1000;
  if (Begin >= End) {
    return;
  }

  auto Mappings = StealMemoryRegion(RealWorldMappings, Begin, End);
  INFO("StealMemoryRegion 0x" << std::hex << Begin << "-0x" << End);

  fextl::vector<MemoryRegion> ref {
    FromTo(0x0, 0xaaaaaaaa0000),
    FromTo(0xaaaaaadba000, 0xaaaaaadc9000),
    FromTo(0xaaaaab77a000, 0xaaaaab789000),
    FromTo(0xaaaaab7b7000, 0xaaaaab7c6000),
    FromTo(0xaaaaabcca000, 0xfffff6a00000),
    FromTo(0xfffff7a00000, 0xfffff7af0000),
    FromTo(0xfffff7c99000, 0xfffff7ca0000),
    FromTo(0xfffff7cc5000, 0xfffff7cd0000),
    FromTo(0xfffff7d67000, 0xfffff7d70000),
    FromTo(0xfffff7f9a000, 0xfffff7fc2000),
    FromTo(0xfffff7fed000, 0xfffff7fef000),
    FromTo(0xfffff8000000, 0xfffffffd2000),
    FromTo(0x1000000000000, 0xffffffffffffffff),
  };

  for (auto it = ref.begin(); it != ref.end();) {
    if (reinterpret_cast<uintptr_t>(it->Ptr) + it->Size <= Begin) {
      it = ref.erase(it);
    } else if (reinterpret_cast<uintptr_t>(it->Ptr) >= End) {
      it = ref.erase(it);
    } else {
      ++it;
    }
  }

  if (!ref.empty()) {
    ref.front().Size -= std::max(Begin, reinterpret_cast<uintptr_t>(ref.front().Ptr)) - reinterpret_cast<uintptr_t>(ref.front().Ptr);
    ref.front().Ptr = std::max(reinterpret_cast<void*>(Begin), ref.front().Ptr);
    ref.back().Size = End - reinterpret_cast<uintptr_t>(ref.back().Ptr);
  }

  CHECK_THAT(Mappings, Catch::Matchers::Equals(ref));
}

TEST_CASE_METHOD(Fixture, "StackCase") {
  uintptr_t StackBottom = GENERATE(0x100004, 0x101000, 0x101ffc); // NOTE: Must point into mapped memory, otherwise the test makes no sense
  uintptr_t Begin = 0;
  uintptr_t End = GENERATE(0x100000, 0x101000, 0x102000, 0x103000, 0x200000);

  const char MappingsList[] =
    "000000100000-000000102000 r--p 00000000 00:00 0                          placeholder\n"
    "000000102000-000000104000 r--p 00000000 00:00 0                          placeholder\n"
    "000000110000-000000112000 r--p 00000000 00:00 0                          placeholder\n"
    "000000112000-000000114000 r--p 00000000 00:00 0                          placeholder\n"
    "000000120000-000000122000 r--p 00000000 00:00 0                          placeholder\n"
    "000000122000-000000124000 r--p 00000000 00:00 0                          placeholder\n";

  auto Mappings = StealMemoryRegion(MappingsList, 0, End, StackBottom);
  INFO("StealMemoryRegion 0x" << std::hex << Begin << "-0x" << End << " (stack @ 0x" << StackBottom << ")");

  if (End == 0x101000 && StackBottom <= 0x101000) {
    // Known failure in old implementation
    // TODO: Is the new behavior desirable?
    return;
  }

  fextl::vector<MemoryRegion> ref {
    FromTo(0x0, 0x100000),
    FromTo(0x104000, 0x110000),
    FromTo(0x114000, 0x120000),
    FromTo(0x124000, 0xffffffffffffffff),
  };

  for (auto it = ref.begin(); it != ref.end();) {
    if (reinterpret_cast<uintptr_t>((it)->Ptr) > End) {
      it = ref.erase(it, ref.end());
      if (it != ref.begin()) {
        if (End >= 0x102000) {
          // Crossed mapping boundary, so the previous gap must also be dropped
          // NOTE: This is probably overfitting implementation details
          it = ref.erase(std::prev(it));
        }
      }
      break;
    }

    if ((reinterpret_cast<uintptr_t>(std::prev(it)->Ptr) + std::prev(it)->Size <= StackBottom) && reinterpret_cast<uintptr_t>((it)->Ptr) > StackBottom) {
      REQUIRE(LastStackMapping.has_value());
      CHECK(std::prev(it)->Ptr == LastStackMapping.value().Ptr);
      CHECK(std::prev(it)->Size == LastStackMapping.value().Size);
      it = std::next(ref.erase(std::prev(it)));
    } else {
      ++it;
    }
  }

  for (auto it = ref.begin(); it != ref.end();) {
    if (reinterpret_cast<uintptr_t>(it->Ptr) + it->Size <= Begin) {
      it = ref.erase(it);
    } else if (reinterpret_cast<uintptr_t>(it->Ptr) >= End) {
      it = ref.erase(it);
    } else {
      ++it;
    }
  }

  if (!ref.empty()) {
    ref.front().Size -= std::max(Begin, reinterpret_cast<uintptr_t>(ref.front().Ptr)) - reinterpret_cast<uintptr_t>(ref.front().Ptr);
    ref.front().Ptr = std::max(reinterpret_cast<void*>(Begin), ref.front().Ptr);

    if (reinterpret_cast<uintptr_t>(ref.back().Ptr) + ref.back().Size > End) {
      ref.back().Size = End - reinterpret_cast<uintptr_t>(ref.back().Ptr);
    }
  }

  CHECK_THAT(Mappings, Catch::Matchers::Equals(ref));
}

TEST_CASE_METHOD(Fixture, "StackCase2") {
  uintptr_t StackBottom = GENERATE(0x100004); // NOTE: Must point into mapped memory, otherwise the test makes no sense
  uintptr_t Begin = 0;
  uintptr_t End = GENERATE(0x101000);

  const char MappingsList[] =
    "000000000000-000000002000 r--p 00000000 00:00 0                          placeholder\n"
    "000000100000-000000102000 r--p 00000000 00:00 0                          placeholder\n"
;

  auto Mappings = StealMemoryRegion(MappingsList, 0, End, StackBottom);
  INFO("StealMemoryRegion 0x" << std::hex << Begin << "-0x" << End << " (stack @ 0x" << StackBottom << ")");

  REQUIRE(LastStackMapping.has_value());
  CHECK(reinterpret_cast<uintptr_t>(LastStackMapping.value().Ptr) == 0x2000);
  CHECK(LastStackMapping.value().Size == 0x100000 - 0x2000);
  CHECK_THAT(Mappings, Catch::Matchers::Equals(fextl::vector<MemoryRegion> {}));
}
