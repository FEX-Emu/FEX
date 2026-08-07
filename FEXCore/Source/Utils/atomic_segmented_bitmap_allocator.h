// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/Utils/TypeDefines.h>
#include "Utils/atomic_bitset.h"

#include <array>
#include <optional>

namespace FEXCore::Utils {
struct bitmap_allocator_settings {
  std::array<uint32_t, 3> bucket_sizes;
  std::array<uint32_t, 3> bucket_allocation_percentages;
};

// Default allocation settings passed through template for easy tinkering.
constexpr static bitmap_allocator_settings fex_default_allocator_settings = {
  .bucket_sizes =
    {
      16,   // 16B - 1KB
      512,  // 512B - 32KB
      2048, // 2KB - 128KB
    },
  .bucket_allocation_percentages =
    {
      // 16B bucket special cased to allocate remaining space left over from other buckets.
      100,
      // 40% in the 512B bucket.
      40,
      // 10% in the 2KB bucket.
      10,
    },
};

/**
 * A bitmap allocator that is segmented in to three partitioned buckets, with atomic memory allocation.
 *
 * This class is strongly coupled with FEXCore::Utils::atomic_bitset to have fast and relatively efficient bitmap allocation in a lock-free
 * fashion. The bucket granule sizes are roughly calculated to match FEX's needs for typical allocation ranges in a single atomic word. It
 * supports allocating larger than an atomic word worth of granules with the expectation that those are relatively uncommon. Once a bucket
 * is full, the allocation has a chance to be allocated in to another bucket at a slightly less efficient space usage This is with the
 * expectation that we want to keep a buffer around as long as possible, without allocating a new one as buffer migrating is expensive.
 *
 * TODO: In the future this will support scaling bucket percentages based on which ones filled first. This is currently disabled.
 */
template<bitmap_allocator_settings settings = fex_default_allocator_settings>
class atomic_segmented_bitmap_allocator final {
public:
  ~atomic_segmented_bitmap_allocator() {
    deinit();
  }

  // Initialize segmented allocator asking for a specific arena size.
  // Bitset tracking arena allocations will allocate an independent buffer independent of arena size.
  void init(size_t arena_size) {
    deinit();

    // Align up to page.
    arena_size = FEXCore::AlignUpPowerOf2(arena_size, FEXCore::Utils::FEX_PAGE_SIZE);

    const auto total_bitset_tracking_memory = calculate_bucket_granules_and_bitset_sizes(arena_size);

    // RWX for JIT buffer
    arena_base = reinterpret_cast<uint8_t*>(FEXCore::Allocator::VirtualAlloc(arena_size, true));
    arena_base_size = arena_size;

    // RW for bitset buffer
    const auto bitset_size_aligned_to_page = FEXCore::AlignUpPowerOf2(total_bitset_tracking_memory, FEXCore::Utils::FEX_PAGE_SIZE);
    bitset_base = reinterpret_cast<uint8_t*>(FEXCore::Allocator::VirtualAlloc(bitset_size_aligned_to_page, false));
    bitset_base_size = bitset_size_aligned_to_page;

    // Name the buffer. This is only going to be used for the JIT right now.
    FEXCore::Allocator::VirtualName("FEXMemJIT", arena_base, arena_size);
    FEXCore::Allocator::VirtualName("FEXMem_Misc", bitset_base, bitset_base_size);

    // THP enabled for both buffers.
    FEXCore::Allocator::VirtualTHPControl(arena_base, arena_size, FEXCore::Allocator::THPControl::Enable);
    FEXCore::Allocator::VirtualTHPControl(bitset_base, bitset_base_size, FEXCore::Allocator::THPControl::Enable);

    initialize_bitsets_for_buckets();
  }

  void deinit() {
    if (arena_base) {
      FEXCore::Allocator::VirtualFree(arena_base, arena_base_size);
    }

    if (bitset_base) {
      FEXCore::Allocator::VirtualFree(bitset_base, bitset_base_size);
    }

    arena_base = nullptr;
    bitset_base = nullptr;
  }

  // Allocate memory.
  // Returns nullptr on failure to allocate.
  void* allocate(size_t size) {
    const auto allocation_order = find_allocation_order(size);
    for (size_t i = 0; i < num_bitset_buckets; ++i) {
      const auto bitset_index = get_bitset_index_from_allocation_order(allocation_order, i);
      auto& bucket = bitset_buckets[bitset_index];

      if (!bucket.bucket_allocation_size) {
        continue;
      }

      const auto bucket_granule_size = bitset_buckets[bitset_index].bucket_granule_size;
      const auto aligned_size = FEXCore::AlignUpPowerOf2(size, bucket_granule_size);
      const auto bits_to_allocate = FEXCore::DividePow2(aligned_size, bucket_granule_size);

      auto allocated_bitset_index = bucket.atomic_bitset.allocate(bits_to_allocate);

      if (allocated_bitset_index == bucket.atomic_bitset.invalid()) {
        // Mark that the bucket might be full and continue going.
        mark_bucket_potentially_full(bucket, bitset_index);
        continue;
      }

      // Bitset range allocated, get the pointer.
      return get_ptr_from_bitset(bucket, allocated_bitset_index);
    }

    return nullptr;
  }

  // Free memory.
  void free(void* ptr, size_t size) {
    for (auto& bucket : bitset_buckets) {
      if (ptr >= bucket.bucket_allocation_base && ptr < (bucket.bucket_allocation_base + bucket.bucket_allocation_size)) {
        // Pointer base must be aligned to granule size, but size doesn't need to be.
        // User could have asked for a smaller size and we rounded up to the larger granule.
        LOGMAN_THROW_A_FMT(reinterpret_cast<uint64_t>(ptr) % bucket.bucket_granule_size == 0, "Pointer was not aligned to bucket granule "
                                                                                              "size!");

        const auto bitset_index = FEXCore::DividePow2(
          (reinterpret_cast<uint64_t>(ptr) - reinterpret_cast<uint64_t>(bucket.bucket_allocation_base)), bucket.bucket_granule_size);
        const auto bitset_count = FEXCore::DividePow2(FEXCore::AlignUpPowerOf2(size, bucket.bucket_granule_size), bucket.bucket_granule_size);
        bucket.atomic_bitset.free(bitset_index, bitset_count);
        return;
      }
    }

    LogMan::Msg::AFmt("Attempted to free a pointer that isn't tracked in the bitmap?");
  }

  // Completely clear the allocator.
  // Not thread safe!
  void clear() {
    // VirtualDontNeed replaces pages with zero page.
    FEXCore::Allocator::VirtualDontNeed(arena_base, arena_base_size);
    FEXCore::Allocator::VirtualDontNeed(bitset_base, bitset_base_size);

    // Reinitialize the bitsets.
    initialize_bitsets_for_buckets();
    bucket_filled_order = ~0ULL;
  }

  // Reevaluate bucket allocation capacity percentages.
  // Not thread safe!
  void reevaluate_bucket_allocations_percentages() {
    if (bucket_filled_order.load() == ~0ULL) {
      // Buckets never claimed that they could be full.
      return;
    }

    // TODO: Adjust `bitset_allocation_percentages` based on fill order.
    // TODO: Reallocate bitsets with `calculate_bucket_granules_and_bitset_sizes`
    // Statistically for a Denuvo game, likely bucket[0] will be the first to fill, which will consume additional resources from the larger buckets.
    // Statistically for any other game, likely bucket[1] will be the first to fill, which will consume additional resources from bucket[0].
    // TODO: Gather growth statistics from a variety of titles to determine trends.

    // Reset bucket fill order.
    bucket_filled_order = ~0ULL;
  }

  // Debugger routines.
  // Returns a bucket index if the pointer exists in it.
  ssize_t find_bucket_index(void* ptr) const {
    for (size_t i = 0; i < bitset_buckets.size(); ++i) {
      const auto& bucket = bitset_buckets[i];
      if (ptr >= bucket.bucket_allocation_base && ptr < (bucket.bucket_allocation_base + bucket.bucket_allocation_size)) {
        return i;
      }
    }
    return -1;
  }

  // Returns the number of buckets.
  // Although they may be empty without the ability to allocate.
  size_t num_buckets() const {
    return bitset_buckets.size();
  }

  struct bucket_alloc_information {
    size_t granule_size {};
    size_t allocated {};
    size_t free {};
  };

  // Returns information about a bucket's granule allocations.
  std::optional<bucket_alloc_information> get_granule_information(size_t bucket_index) const {
    if (bucket_index >= bitset_buckets.size()) {
      return std::nullopt;
    }

    const auto& bucket = bitset_buckets[bucket_index];
    const auto count = bucket.atomic_bitset.popcount();

    return bucket_alloc_information {
      .granule_size = bucket.bucket_granule_size,
      .allocated = count,
      .free = bucket.atomic_bitset.size_in_bits() - count,
    };
  }

private:
  // Arena base.
  uint8_t* arena_base {};
  size_t arena_base_size {};

  // Bitset tracking.
  uint8_t* bitset_base {};
  size_t bitset_base_size {};

  // Bucket sizes are important for how much can be allocated in a single 64-bit atomic operation.
  // Ensure these two buffers are sorted by size.
  constexpr static std::array<uint32_t, 3> bucket_granule_sizes = settings.bucket_sizes;

  // These must be power of two.
  static_assert(std::has_single_bit(bucket_granule_sizes[0]));
  static_assert(std::has_single_bit(bucket_granule_sizes[1]));
  static_assert(std::has_single_bit(bucket_granule_sizes[2]));

  // Ordered by size.
  static_assert(bucket_granule_sizes[0] < bucket_granule_sizes[1]);
  static_assert(bucket_granule_sizes[1] < bucket_granule_sizes[2]);

  constexpr static size_t num_bitset_buckets = bucket_granule_sizes.size();
  constexpr static size_t bitset_index_shift = 2;
  constexpr static size_t bitset_index_mask = (1U << bitset_index_shift) - 1;

  struct bucket_information {
    // Memory that the bitset is controlling
    uint8_t* bucket_allocation_base {};
    size_t bucket_allocation_size {};

    // Memory backing the bitset itself.
    uint8_t* bitset_memory_base {};
    size_t bitset_memory_size {};

    // The allocation granule of the bitset.
    size_t bucket_granule_size {};

    // std::atomic<bool>::fetch_or isn't required to be implemented, so use uint8_t instead.
    std::atomic<uint8_t> marked_potentially_full {};
    FEXCore::Utils::atomic_bitset<true, false> atomic_bitset {};
  };

  // Order in which the buckets filled for heuristic scaling of bucket sizes.
  std::atomic<uint64_t> bucket_filled_order {~0ULL};
  std::array<bucket_information, num_bitset_buckets> bitset_buckets {};

  // TODO: Support scaling bitset bucket percentages by order in which they filled.
  // Currently this is const and can't change when the buckets run out of space.
  constexpr static std::array<uint64_t, num_bitset_buckets> bitset_allocation_percentages {
    settings.bucket_allocation_percentages[0],
    settings.bucket_allocation_percentages[1],
    settings.bucket_allocation_percentages[2],
  };

  // bucket[0] is special cased to catch everything remaining.
  static_assert(settings.bucket_allocation_percentages[0] == 100);

  // Calculate bitset sizes based on percentages of the arena size.
  // eg - split at 50+40+10:
  //   - 16MB:  8MB + 6.4MB + 1.6MB
  //   - 128MB: 64MB + 51.2MB + 12.8MB
  size_t calculate_bucket_granules_and_bitset_sizes(size_t arena_size) {
    size_t total_bitset_range {};
    size_t total_bitset_tracking_memory {};

    for (size_t inverse_i = bucket_granule_sizes.size(); inverse_i > 0; --inverse_i) {
      const auto i = inverse_i - 1;
      const size_t bucket_granule_size = bucket_granule_sizes[i];
      const auto allocation_percentage = bitset_allocation_percentages[i];
      auto& bucket = bitset_buckets[i];

      // Set the bucket granule size.
      bucket.bucket_granule_size = bucket_granule_size;

      // Calculate the amount of space this bitset tracks.
      // If each bucket is tracking data at all it must track at least one 64-bit atomic word of bits.
      // 16B bucket:  1KB minimum
      // 512B bucket: 32KB minimum
      // 2KB bucket:  128KB minimum
      // Total: 161KB to reach minimums.
      //
      // If minimum of each bucket isn't reached, that bucket allocates **ZERO**
      size_t bucket_track_size {};
      if (allocation_percentage == 100) {
        // Use the remaining size to allocate in special case.
        bucket_track_size = FEXCore::AlignDown(arena_size - total_bitset_range, bucket_granule_size * WORD_SIZE_BITS);
      } else {
        bucket_track_size = FEXCore::AlignDown(arena_size * allocation_percentage / 100, bucket_granule_size * WORD_SIZE_BITS);
      }

      const auto tracked_bits = FEXCore::DividePow2(bucket_track_size, bucket_granule_size);
      const auto bitset_memory_size = tracked_bits / 8;
      bucket.bucket_allocation_size = bucket_track_size;
      bucket.bitset_memory_size = bitset_memory_size;

      // Mark the bucket as empty if it has no size.
      bucket.marked_potentially_full.store(bucket_track_size == 0, std::memory_order_relaxed);
      total_bitset_range += bucket_track_size;
      total_bitset_tracking_memory += bitset_memory_size;

      LOGMAN_THROW_A_FMT(tracked_bits % WORD_SIZE_BITS == 0, "Wasn't aligned to 64-bits!");
    }

    LOGMAN_THROW_A_FMT((arena_size - total_bitset_range) == 0, "Still had {} bytes remaining in bitmap allocator init",
                       (arena_size - total_bitset_range));

    return total_bitset_tracking_memory;
  }

  void initialize_bitsets_for_buckets() {
    uint8_t* current_base_arena = arena_base;
    uint8_t* current_base_bitset = bitset_base;
    for (auto& bucket : bitset_buckets) {
      if (!bucket.bucket_allocation_size) {
        continue;
      }

      const auto tracked_bits = FEXCore::DividePow2(bucket.bucket_allocation_size, bucket.bucket_granule_size);

      bucket.bucket_allocation_base = current_base_arena;
      bucket.atomic_bitset.init(current_base_bitset, tracked_bits);

      current_base_arena += bucket.bucket_allocation_size;
      current_base_bitset += bucket.bitset_memory_size;
    }

    current_base_bitset =
      reinterpret_cast<uint8_t*>(FEXCore::AlignUpPowerOf2(reinterpret_cast<uint64_t>(current_base_bitset), FEXCore::Utils::FEX_PAGE_SIZE));
    LogMan::Throw::AFmt(current_base_arena == (arena_base + arena_base_size), "Failed arena math: 0x{:x} != expected 0x{:x}",
                        (uint64_t)current_base_arena, (uint64_t)arena_base + arena_base_size);
    LogMan::Throw::AFmt(current_base_bitset == (bitset_base + bitset_base_size), "Failed bitset math: 0x{:x} != expected 0x{:x}",
                        (uint64_t)current_base_bitset, (uint64_t)bitset_base + bitset_base_size);
  }

  // Marking a bucket as potentially being full.
  // Doesn't necessarily mean it is actually full, just allocations have started failing.
  // Which this could mean high fragmentation or actually full.
  // Regardless mark the buffer based on order of allocations failing.
  void mark_bucket_potentially_full(bucket_information& bucket, size_t bitset_index) {
    if (bucket.marked_potentially_full.load(std::memory_order_relaxed)) {
      return;
    }

    // fetch_or is faster than CAS here.
    bool previous_full = bucket.marked_potentially_full.fetch_or(true);
    if (previous_full) {
      // Another thread already marking it as full. Minor race.
      return;
    }

    uint64_t expected = bucket_filled_order.load(std::memory_order_relaxed);
    uint64_t desired;

    do {
      desired = (expected << bitset_index_shift) | bitset_index;
    } while (!bucket_filled_order.compare_exchange_strong(expected, desired));
  }

  // Return a pointer to the data backing the bitset based on allocation index.
  void* get_ptr_from_bitset(const bucket_information& bucket, size_t allocation_index) const {
    return bucket.bucket_allocation_base + bucket.bucket_granule_size * allocation_index;
  }

  // Returns an ordered list of bitsets for which bitsets we should allocate from.
  // Always returns all the bitsets but fitment of the heuristic may change the order.
  // LSB is highest priority, MSB is lowest priority.
  uint64_t find_allocation_order(size_t size) const {
    // Align up by the size of the smallest bucket size
    size = FEXCore::AlignUpPowerOf2(size, bitset_buckets[0].bucket_granule_size);

    struct tracking {
      int32_t index {};
    };

    // This is effectively setup to be a linear scan std::deque, but without the slow overhead of std::deque.
    std::array<tracking, num_bitset_buckets> remaining_bitsets = {{
      {.index = 0},
      {.index = 1},
      {.index = 2},
    }};

    // Check exact size match.
    int32_t exact_match_index = -1;
    int32_t smallest_single_atomic_index = -1;
    for (auto it = remaining_bitsets.begin(); it != remaining_bitsets.end(); ++it) {
      auto& bitset_tracking = *it;
      const auto bitset_index = bitset_tracking.index;
      const auto bucket_granule_size = bitset_buckets[bitset_index].bucket_granule_size;

      if (bucket_granule_size == size) {
        exact_match_index = bitset_index;
        bitset_tracking.index = -1;
        break;
      }
    }

    // Check for smallest match within a 64-bit atomic.
    for (auto it = remaining_bitsets.begin(); it != remaining_bitsets.end(); ++it) {
      auto& bitset_tracking = *it;
      if (bitset_tracking.index == -1) {
        continue;
      }

      const auto bitset_index = bitset_tracking.index;
      const auto bucket_granule_size = bitset_buckets[bitset_index].bucket_granule_size;

      if (size < bucket_granule_size) {
        // If the allocation size is smaller than a single bit, then skip this.
        // Ensures we don't burn 2KB buckets with 128B allocations unnecessarily.
        continue;
      }

      const auto bucket_granule_size_per_atomic_word = bucket_granule_size * WORD_SIZE_BITS;

      if (size <= bucket_granule_size_per_atomic_word) {
        // Fits within a single 64-bit atomic.
        smallest_single_atomic_index = bitset_index;
        bitset_tracking.index = -1;
        break;
      }
    }

    uint64_t allocation_order {};
    uint64_t current_shift {};

    // Exact match has highest priority.
    if (exact_match_index != -1) {
      allocation_order = exact_match_index;
      current_shift += bitset_index_shift;
    }

    // Smallest fit within a single atomic word has second priority.
    if (smallest_single_atomic_index != -1) {
      allocation_order |= (smallest_single_atomic_index << current_shift);
      current_shift += bitset_index_shift;
    }

    // Remaining is sorted by smallest->largest bucket size;
    // TODO: Might lead to the bitset allocation heuristic to favor scaling the lower bucket sizes to a larger percentage.
    for (auto it : remaining_bitsets) {
      if (it.index == -1) {
        continue;
      }

      allocation_order |= (it.index << current_shift);
      current_shift += bitset_index_shift;
    }

    return allocation_order;
  }

  constexpr static size_t get_bitset_index_from_allocation_order(uint64_t allocation_order, size_t index) {
    return (allocation_order >> (index * bitset_index_shift)) & bitset_index_mask;
  }

  constexpr static size_t WORD_SIZE_BITS = sizeof(uint64_t) * 8;
};
} // namespace FEXCore::Utils
