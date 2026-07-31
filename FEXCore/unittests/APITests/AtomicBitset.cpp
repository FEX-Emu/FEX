// SPDX-License-Identifier: MIT
#include <catch2/catch_all.hpp>
#include <thread>

#include "Utils/atomic_bitset.h"

bool CheckMemoryIsZero(void* ptr, size_t size) {
  REQUIRE(size % sizeof(uint64_t) == 0);

  auto ptr_u64 = reinterpret_cast<uint64_t*>(ptr);
  for (size_t i = 0; i < (size / sizeof(uint64_t)); ++i) {
    if (ptr_u64[i] != 0) {
      return false;
    }
  }

  return true;
}

bool CheckMemoryIsSet(void* ptr, size_t size) {
  REQUIRE(size % sizeof(uint64_t) == 0);

  auto ptr_u64 = reinterpret_cast<uint64_t*>(ptr);
  for (size_t i = 0; i < (size / sizeof(uint64_t)); ++i) {
    if (ptr_u64[i] != ~0ULL) {
      return false;
    }
  }

  return true;
}

struct buffer {
  uint8_t* ptr;

  uint8_t* ptr_base;
  size_t size;
};

buffer AllocateProtectedBuffer(size_t size) {
  buffer buf {
    .size = size + 4096 * 2,
  };
  buf.ptr_base = reinterpret_cast<uint8_t*>(mmap(nullptr, buf.size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
  REQUIRE(buf.ptr_base != nullptr);

  buf.ptr = buf.ptr_base + 4096;

  // RW only the pages requested.
  mprotect(buf.ptr, size, PROT_READ | PROT_WRITE);

  return buf;
}

void FreeProtectedBuffer(buffer buf) {
  munmap(buf.ptr_base, buf.size);
}

TEST_CASE("Single") {
  constexpr size_t size = 4096;
  constexpr size_t size_bits = size * 8;
  auto buf = AllocateProtectedBuffer(size);

  FEXCore::Utils::atomic_bitset set {};
  set.init(buf.ptr, size_bits);

  REQUIRE(set.size_in_bits() == size_bits);

  // Basic allocation check.
  auto slot = set.allocate(1);
  REQUIRE(slot != set.invalid());
  CHECK(slot == 0);

  set.free(slot, 1);

  CHECK(CheckMemoryIsZero(buf.ptr, size));

  FreeProtectedBuffer(buf);
}

TEST_CASE("All") {
  constexpr size_t size = 4096;
  constexpr size_t size_bits = size * 8;
  auto buf = AllocateProtectedBuffer(size);

  FEXCore::Utils::atomic_bitset set {};
  set.init(buf.ptr, size_bits);

  REQUIRE(set.size_in_bits() == size_bits);

  // Allocate all bits, ensuring all can be allocated.
  for (size_t i = 0; i < size_bits; ++i) {
    auto slot = set.allocate(1);
    REQUIRE(slot != set.invalid());
  }

  CHECK(CheckMemoryIsSet(buf.ptr, size));

  // Ensure that overallocation fails.
  CHECK(set.allocate(1) == set.invalid());

  // Free all the bits
  for (size_t i = 0; i < size_bits; ++i) {
    set.free(i, 1);
  }

  CHECK(CheckMemoryIsZero(buf.ptr, size));

  FreeProtectedBuffer(buf);
}

TEST_CASE("Large") {
  constexpr size_t size = 4096;
  constexpr size_t size_bits = size * 8;
  auto buf = AllocateProtectedBuffer(size);

  FEXCore::Utils::atomic_bitset set {};
  set.init(buf.ptr, size_bits);

  REQUIRE(set.size_in_bits() == size_bits);

  // Allocate a single 64-bit word.
  auto slot = set.allocate(64);
  REQUIRE(slot != set.invalid());
  CHECK(slot == 0);

  set.free(slot, 64);

  CHECK(CheckMemoryIsZero(buf.ptr, size));

  FreeProtectedBuffer(buf);
}

TEST_CASE("Large Sparse") {
  constexpr size_t size = 4096;
  constexpr size_t size_bits = size * 8;
  auto buf = AllocateProtectedBuffer(size);

  FEXCore::Utils::atomic_bitset set {};
  set.init(buf.ptr, size_bits);

  REQUIRE(set.size_in_bits() == size_bits);

  // Allocate a single bit.
  auto slot = set.allocate(1);
  REQUIRE(slot != set.invalid());
  CHECK(slot == 0);

  // Allocate a single 64-bit contiguous region.
  // Due to implementation behaviour, this should be a full word ahead of the previous.
  auto slot64 = set.allocate(64);
  REQUIRE(slot64 != set.invalid());
  CHECK(slot64 == 64);

  set.free(slot, 1);
  set.free(slot64, 64);

  CHECK(CheckMemoryIsZero(buf.ptr, size));

  FreeProtectedBuffer(buf);
}

TEST_CASE("Large Sparse - in-fill") {
  constexpr size_t size = 4096;
  constexpr size_t size_bits = size * 8;
  auto buf = AllocateProtectedBuffer(size);

  FEXCore::Utils::atomic_bitset set {};
  set.init(buf.ptr, size_bits);

  REQUIRE(set.size_in_bits() == size_bits);

  // Allocate a single bit.
  auto slot = set.allocate(1);
  REQUIRE(slot != set.invalid());
  CHECK(slot == 0);

  // Allocate a single 64-bit contiguous region.
  // Due to implementation behaviour, this should be a full word ahead of the previous.
  auto slot64 = set.allocate(64);
  REQUIRE(slot64 != set.invalid());
  CHECK(slot64 == 64);

  std::vector<size_t> sparse {};

  for (size_t i = 1; i < 64; ++i) {
    // Allocation of single elements should fill in sparsity.
    auto new_slot = set.allocate(1);
    REQUIRE(new_slot != set.invalid());
    CHECK(new_slot == i);
    sparse.emplace_back(new_slot);
  }

  for (auto it : sparse) {
    set.free(it, 1);
  }

  set.free(slot, 1);
  set.free(slot64, 64);

  CHECK(CheckMemoryIsZero(buf.ptr, size));

  FreeProtectedBuffer(buf);
}

TEST_CASE("Large Sparse - chunk") {
  constexpr size_t size = 4096;
  constexpr size_t size_bits = size * 8;
  auto buf = AllocateProtectedBuffer(size);

  FEXCore::Utils::atomic_bitset set {};
  set.init(buf.ptr, size_bits);

  REQUIRE(set.size_in_bits() == size_bits);

  // Allocate a single bit.
  auto slot = set.allocate(1);
  REQUIRE(slot != set.invalid());
  CHECK(slot == 0);

  // Allocate a single 64-bit contiguous region.
  // Due to implementation behaviour, this should be a full word ahead of the previous.
  auto slot64 = set.allocate(64);
  REQUIRE(slot64 != set.invalid());
  CHECK(slot64 == 64);

  // A smaller allocation that fits within an empty word should still sub allocate.
  auto slot32 = set.allocate(32);
  REQUIRE(slot32 != set.invalid());
  CHECK(slot32 < slot64);

  set.free(slot, 1);
  set.free(slot64, 64);
  set.free(slot32, 32);

  CHECK(CheckMemoryIsZero(buf.ptr, size));

  FreeProtectedBuffer(buf);
}

TEST_CASE("Large Sparse - chunk in-fill") {
  constexpr size_t size = 4096;
  constexpr size_t size_bits = size * 8;
  auto buf = AllocateProtectedBuffer(size);

  FEXCore::Utils::atomic_bitset set {};
  set.init(buf.ptr, size_bits);

  REQUIRE(set.size_in_bits() == size_bits);

  // Allocate a single bit.
  auto slot = set.allocate(1);
  REQUIRE(slot != set.invalid());
  CHECK(slot == 0);

  // Allocate a single 64-bit contiguous region.
  // Due to implementation behaviour, this should be a full word ahead of the previous.
  auto slot64 = set.allocate(64);
  REQUIRE(slot64 != set.invalid());
  CHECK(slot64 == 64);

  // 63-bits should in-fill between the previous two allocations
  auto slot63 = set.allocate(63);
  REQUIRE(slot63 != set.invalid());
  CHECK(slot63 < slot64);
  CHECK(slot63 == (slot + 1));

  set.free(slot, 1);
  set.free(slot64, 64);
  set.free(slot63, 63);

  CHECK(CheckMemoryIsZero(buf.ptr, size));

  FreeProtectedBuffer(buf);
}

TEST_CASE("Large to small") {
  constexpr size_t size = 4096;
  constexpr size_t size_bits = size * 8;
  auto buf = AllocateProtectedBuffer(size);

  FEXCore::Utils::atomic_bitset set {};
  set.init(buf.ptr, size_bits);

  REQUIRE(set.size_in_bits() == size_bits);

  // Allocate a single word.
  auto slot = set.allocate(64);
  REQUIRE(slot != set.invalid());
  CHECK(slot == 0);

  // Clearing the sub bits individually should work.
  for (size_t i = 0; i < 64; ++i) {
    set.free(slot + i, 1);
  }

  CHECK(CheckMemoryIsZero(buf.ptr, size));

  FreeProtectedBuffer(buf);
}

TEST_CASE("Small to large") {
  constexpr size_t size = 4096;
  constexpr size_t size_bits = size * 8;
  auto buf = AllocateProtectedBuffer(size);

  FEXCore::Utils::atomic_bitset set {};
  set.init(buf.ptr, size_bits);

  REQUIRE(set.size_in_bits() == size_bits);

  // Allocate a single word using single allocations.
  auto slot0 = set.allocate(1);
  REQUIRE(slot0 != set.invalid());
  CHECK(slot0 == 0);

  for (size_t i = 1; i < 64; ++i) {
    auto slot = set.allocate(1);
    REQUIRE(slot != set.invalid());
    CHECK(slot == i);
  }

  // Freeing smaller continguous slots using a larger size should work.
  set.free(slot0, 64);

  CHECK(CheckMemoryIsZero(buf.ptr, size));

  FreeProtectedBuffer(buf);
}

TEST_CASE("Large Clear") {
  constexpr size_t size = 4096 * 4;
  constexpr size_t size_bits = size * 8;
  auto buf = AllocateProtectedBuffer(size);

  FEXCore::Utils::atomic_bitset set {};
  set.init(buf.ptr, size_bits);

  REQUIRE(set.size_in_bits() == size_bits);

  // Allocate the whole set
  while (set.allocate(64) != set.invalid())
    ;

  CHECK(CheckMemoryIsSet(buf.ptr, size));

  // Clearing the set should reset everything.
  set.clear();

  CHECK(CheckMemoryIsZero(buf.ptr, size));

  FreeProtectedBuffer(buf);
}

TEST_CASE("Larger than word") {
  constexpr size_t size = 4096;
  constexpr size_t size_bits = size * 8;
  auto buf = AllocateProtectedBuffer(size);

  FEXCore::Utils::atomic_bitset set {};
  set.init(buf.ptr, size_bits);

  REQUIRE(set.size_in_bits() == size_bits);

  struct test_data {
    uint64_t offset_count {};
    uint64_t total_size {};
  };

  auto run_test = [&set](test_data data) {
    if (data.offset_count) {
      REQUIRE(set.allocate(data.offset_count) == 0);
    }

    REQUIRE(set.allocate(data.total_size) == data.offset_count);

    for (size_t i = 0; i < data.offset_count; ++i) {
      REQUIRE(set.is_set(i));
    }

    for (size_t i = 0; i < data.total_size; ++i) {
      REQUIRE(set.is_set(data.offset_count + i));
    }

    // Reset the buffer.
    set.clear();
  };

  // Test matrix to hit all the code paths for larger than word allocations
  //
  // | Head offset | Head | Head+Center | Head+Center+Tail |
  // | ----------- | ---- | ----------- | ---------------- |
  // | Offset(0)   |    🗹 |           🗹 |                🗹 |
  // | Offset(1)   |    🗹 |           🗹 |                🗹 |
  // | Offset(63)  |    🗹 |           🗹 |                🗹 |
  // | Offset(511) |    🗹 |           🗹 |                🗹 |

  constexpr static test_data tests[] = {
    {0, 64 + 1},       // Offset(0) + Head + Tail
    {1, 63 + 2},       // Offset(1) + Head + Tail
    {62, 2 + 63},      // Offset(62) + Head + Tail
    {510, 2 + 63},     // Offset(510) + Head + Tail
    {0, 64 + 64},      // Offset(0) + Head + Center
    {1, 63 + 64},      // Offset(1) + Head + Center
    {63, 1 + 64},      // Offset(63) + Head + Center
    {511, 1 + 64},     // Offset(511) + Head + Center
    {0, 64 + 64 + 1},  // Offset(0) + Head + Center + Tail
    {1, 63 + 64 + 1},  // Offset(1) + Head + Center + Tail
    {63, 1 + 64 + 1},  // Offset(63) + Head + Center + Tail
    {511, 1 + 64 + 1}, // Offset(511) + Head + Center + Tail
  };

  for (auto test : tests) {
    run_test(test);
  }

  FreeProtectedBuffer(buf);
}

TEST_CASE("Larger than word - Free") {
  constexpr size_t size = 4096;
  constexpr size_t size_bits = size * 8;
  auto buf = AllocateProtectedBuffer(size);

  FEXCore::Utils::atomic_bitset set {};
  set.init(buf.ptr, size_bits);

  REQUIRE(set.size_in_bits() == size_bits);

  struct test_data {
    uint64_t offset_count {};
    uint64_t total_size {};
  };

  auto run_test = [&set, &buf](test_data data) {
    if (data.offset_count) {
      REQUIRE(set.allocate(data.offset_count) == 0);
    }

    REQUIRE(set.allocate(data.total_size) == data.offset_count);

    for (size_t i = 0; i < data.offset_count; ++i) {
      REQUIRE(set.is_set(i));
    }

    for (size_t i = 0; i < data.total_size; ++i) {
      REQUIRE(set.is_set(data.offset_count + i));
    }

    if (data.offset_count) {
      set.free(0, data.offset_count);
    }

    set.free(data.offset_count, data.total_size);

    REQUIRE(CheckMemoryIsZero(buf.ptr, size));

    // Reset the buffer.
    set.clear();
  };

  // Test matrix to hit all the code paths for larger than word allocations
  //
  // | Head offset | Head | Head+Center | Head+Center+Tail |
  // | ----------- | ---- | ----------- | ---------------- |
  // | Offset(0)   |    🗹 |           🗹 |                🗹 |
  // | Offset(1)   |    🗹 |           🗹 |                🗹 |
  // | Offset(63)  |    🗹 |           🗹 |                🗹 |
  constexpr static test_data tests[] = {
    {0, 64 + 1},      // Offset(0) + Head + Tail
    {1, 63 + 2},      // Offset(1) + Head + Tail
    {62, 2 + 63},     // Offset(62) + Head + Tail
    {0, 64 + 64},     // Offset(0) + Head + Center
    {1, 63 + 64},     // Offset(1) + Head + Center
    {63, 1 + 64},     // Offset(63) + Head + Center
    {0, 64 + 64 + 1}, // Offset(0) + Head + Center + Tail
    {1, 63 + 64 + 1}, // Offset(1) + Head + Center + Tail
    {63, 1 + 64 + 1}, // Offset(63) + Head + Center + Tail
  };

  for (auto test : tests) {
    run_test(test);
  }

  FreeProtectedBuffer(buf);
}

TEST_CASE("Larger than Word - 128-bits") {
  // Test to ensure on small bitset size, a larger than word allocation still fits.
  constexpr size_t size = 4096;
  auto buf = AllocateProtectedBuffer(size);

  FEXCore::Utils::atomic_bitset<false, false> set {};

  // Move the set up to the edge of the page to detect overruns.
  set.init(reinterpret_cast<uint8_t*>(buf.ptr) + (4096 - 16), 128);

  REQUIRE(set.size_in_bits() == 128);

  for (size_t i = 0; i < 128; ++i) {
    auto slot = set.allocate(i + 1);
    REQUIRE(slot != set.invalid());
    set.free(slot, i + 1);
  }

  FreeProtectedBuffer(buf);
}

TEST_CASE("Race acquire - Single") {
  // Test to ensure that allocating 1 slot should never fail unless it is actually full.
  // Basic race condition check.
  constexpr size_t size = 4096;
  auto buf = AllocateProtectedBuffer(size);

  FEXCore::Utils::atomic_bitset<false, false> set {};

  // Move the set up to the edge of the page to detect overruns.
  set.init(reinterpret_cast<uint8_t*>(buf.ptr) + (4096 - 32), 256);

  REQUIRE(set.size_in_bits() == 256);

  // Allocate all 256-bits
  for (size_t i = 0; i < 256; ++i) {
    REQUIRE(set.allocate(1) != set.invalid());
  }

  // Free the first 128-bits
  for (size_t i = 0; i < 128; ++i) {
    set.free(i, 1);
  }

  std::atomic<bool> Acquire {};
  std::atomic<uint64_t> Waiters {};
  std::vector<std::thread> threads {};
  std::atomic<uint64_t> slots[129] {};
  threads.reserve(128);

  auto acquire = [&](int idx) {
    ++Waiters;

    // Spin until allowed to race.
    while (!Acquire.load()) {
      // Be nice to valgrind.
      std::this_thread::yield();
    }

    auto slot = set.allocate(1);

    if (slot == set.invalid()) {
      // Set to invalid slot. Should never occur.
      slot = 128;
    }

    // Increment the slot counter for the number of times this slot has allocated.
    slots[slot]++;
  };

  for (size_t i = 0; i < 128; ++i) {
    threads.emplace_back(acquire, i);
  }

  // Wait until all threads are claimed to be ready.
  while (Waiters.load() != 128) {
    // Be nice to valgrind.
    std::this_thread::yield();
  }

  Acquire = true;

  // Wait for threads to exit.
  for (auto& t : threads) {
    t.join();
  }

  // Every slot should only ever be acquired once.
  for (size_t i = 0; i < 128; ++i) {
    CHECK(slots[i].load() == 1);
  }

  // There should be no invalid slots returned.
  CHECK(slots[128].load() == 0);

  FreeProtectedBuffer(buf);
}

TEST_CASE("Larger than word - rewind") {
  constexpr size_t size = 4096;
  auto buf = AllocateProtectedBuffer(size);

  FEXCore::Utils::atomic_bitset<false, false> set {};

  // Move the set up to the edge of the page to detect overruns.
  set.init(reinterpret_cast<uint8_t*>(buf.ptr) + (4096 - 32), 256);

  REQUIRE(set.size_in_bits() == 256);

  // Allocate all 256-bits
  for (size_t i = 0; i < 256; ++i) {
    REQUIRE(set.allocate(1) != set.invalid());
  }

  // Free the first 128-bits
  for (size_t i = 0; i < 128; ++i) {
    set.free(i, 1);
  }

  std::atomic<bool> Running {};
  std::atomic<bool> Stop {};
  std::thread t {[&]() {
    LogMan::Msg::DFmt("Spinning");
    // Acquire and free 1-bit back to back
    while (!Stop) {
      auto slot = set.allocate(1);

      // Introduce some variability by yielding here.
      std::this_thread::yield();

      Running = true;
      if (slot != set.invalid()) {
        set.free(slot, 1);
      } else {
        LogMan::Msg::DFmt("We're full!");
        break;
      }
    }
  }};

  LogMan::Msg::DFmt("Waiting for thread to start!");
  while (!Running.load()) {
    // Be nice to valgrind.
    std::this_thread::yield();
  }

  LogMan::Msg::DFmt("Attempting to allocate 128-bit while contended");

  // Try and acquire 128-bits while contended.
  size_t attempts {};
  for (;;) {
    auto slot = set.allocate(128);
    if (slot == set.invalid()) {
      ++attempts;
      // Be nice to valgrind.
      std::this_thread::yield();
      continue;
    }

    // Can't fit in anything other than slot0
    REQUIRE(slot == 0);
    LogMan::Msg::DFmt("We got 128-bits in slot: {} after {} attempts", slot, attempts);
    set.free(slot, 128);
    break;
  }
  Stop = true;
  t.join();

  FreeProtectedBuffer(buf);
}
