// SPDX-License-Identifier: MIT
#include <catch2/catch_all.hpp>
#include <unordered_set>

#include "Utils/atomic_segmented_bitmap_allocator.h"

TEST_CASE("Single") {
  FEXCore::Utils::atomic_segmented_bitmap_allocator alloc {};
  alloc.init(4096);
  alloc.init(162 * 1024);
  alloc.init(1280 * 1024);
}

TEST_CASE("Single - tiny") {
  FEXCore::Utils::atomic_segmented_bitmap_allocator alloc {};
  alloc.init(4096);
  std::map<void*, uint64_t> hits {};
  for (size_t i = 0; i < 256; ++i) {
    auto ptr = alloc.allocate(16);
    CHECK(ptr != nullptr);

    // Count any potential duplicate hits.
    hits[ptr] += 1;
  }

  // Ensure all hits aren't duplicated.
  for (auto hit : hits) {
    CHECK(hit.second == 1);
  }

  // Final allocation should fail.
  CHECK(alloc.allocate(16) == nullptr);
}

TEST_CASE("Single - tiny failover") {
  FEXCore::Utils::atomic_segmented_bitmap_allocator alloc {};

  // Should give 6400 + 128 + 0 allocation counts with the default buckets.
  const size_t allocation_size = 164 * 1024;
  alloc.init(allocation_size);

  std::map<void*, uint64_t> hits {};
  std::unordered_set<ssize_t> hit_buckets;

  void* ptr {};
  while ((ptr = alloc.allocate(16)) != nullptr) {
    // Count any potential duplicate hits.
    hits[ptr] += 1;
    hit_buckets.emplace(alloc.find_bucket_index(ptr));
  }

  // Ensure all hits aren't duplicated.
  for (auto hit : hits) {
    CHECK(hit.second == 1);
  }

  // Ensure at least two buckets were hit.
  size_t buckets {};
  size_t total_memory {};
  for (auto index : hit_buckets) {
    REQUIRE(index != -1);
    buckets++;

    const auto granule_information = alloc.get_granule_information(index);
    REQUIRE(granule_information.has_value());
    CHECK(granule_information->free == 0);
    total_memory += granule_information->allocated * granule_information->granule_size;
  }

  CHECK(buckets > 1);
  CHECK(total_memory == allocation_size);

  // Walk all the allocations and free them.
  for (auto hit : hits) {
    alloc.free(hit.first, 16);
  }
}

TEST_CASE("Single - tiny - free") {
  FEXCore::Utils::atomic_segmented_bitmap_allocator alloc {};

  std::vector<void*> values {};
  values.resize(256);
  alloc.init(4096);

  for (size_t i = 0; i < 256; ++i) {
    auto ptr = alloc.allocate(16);
    values[i] = ptr;
  }

  for (size_t i = 0; i < 256; ++i) {
    alloc.free(values[i], 16);
  }
}
