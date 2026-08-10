// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/MathUtils.h>

#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace FEXCore::Utils {
/**
 * A bitset that supports allocating contiguous ranges atomically.
 * - Lock-free, with the caveat that on contention for > 64-bit it would be faster to acquire a lock.
 *   - If low-contention then atomic-behaviour wins.
 * - Three modes of allocation:
 *   - 1-bit, single-atomic.
 *   - <= 64-bit, single-atomic, contained within single word (introduces sparsity).
 *   - > 64-bit, multiple-atomic, contiguous, roll-back on contiguous allocation failure.
 * - Can return failure to allocate even if there is space in certain circumstances.
 *   - If the allocated size crosses multiple words.
 *   - Race to allocation caused contention.
 * - Remembers last allocation/free for inner-word allocations to improve performance.
 *   - Large greater than atomic-word scans always scan from the start.
 * - Resetting the bitset with clear() is lower cost when page_size=true.
 *   - MADV_DONTNEED replaces pages with zero-page
 *   - When page_size=false, basic memset is also fairly quick.
 */
template<bool track_last_allocation = false, bool page_sized = true>
class atomic_bitset final {
public:
  void init(void* ptr, size_t bits) {
    LOGMAN_THROW_A_FMT(bits != 0, "Can't init zero");

    base = reinterpret_cast<uint64_t*>(ptr);
    bits_to_track = bits;
    words_to_track = bits / WORD_SIZE_BITS;
    LOGMAN_THROW_A_FMT(bits % WORD_SIZE_BITS == 0, "Bits to track must match uint64_t");
    if constexpr (page_sized) {
      LOGMAN_THROW_A_FMT(bits % (4096 * 8) == 0, "Bits to track must match bit count in page");
    }

    last_allocation_track.set_last_allocation(0);
  }

  // Allocate a contiguous buffer of bits.
  // Returns initial bit offset on success, ~0ULL on failure.
  size_t allocate(size_t count) {
    LOGMAN_THROW_A_FMT(count != 0, "Can't allocate zero");
    LOGMAN_THROW_A_FMT(count <= bits_to_track, "Can't allocate larger than size");

    if (count == 1) [[likely]] {
      // Common and trivial case.
      return allocate_one(last_allocation_track.get_last_allocation(), words_to_track);
    } else if (count <= WORD_SIZE_BITS) [[likely]] {
      // Allocate up to a single word. Don't allow cross-word allocations
      // Could cause some sparsity
      return allocate_inside_word(count, last_allocation_track.get_last_allocation(), words_to_track);
    }

    // TODO: Always scans from beginning to end.
    // Support iterative scanning.
    return allocate_large_amount(count, 0, words_to_track);
  }

  // Frees a contiguous set of bits.
  void free(size_t index, size_t count) {
    LOGMAN_THROW_A_FMT(count != 0, "Can't free zero");
    LOGMAN_THROW_A_FMT(index < bits_to_track, "Can't free beyond end");

    if (count == 1) [[likely]] {
      free_one(index);
      return;
    } else if ((index % WORD_SIZE_BITS + count) <= WORD_SIZE_BITS) [[likely]] {
      free_inside_word(index, count);
      return;
    }

    free_large_amount(index, count);
  }

  // Clears the entire bitset.
  // Not thread safe!
  void clear() {
    const size_t bytes = words_to_track * sizeof(uint64_t);
    if constexpr (page_sized) {
      // VirtualDontNeed replaces pages with zero page.
      FEXCore::Allocator::VirtualDontNeed(base, bytes);
    } else {
      memset(base, 0, bytes);
    }

    last_allocation_track.set_last_allocation(0);
  }

  // Checks if a single bit is set.
  bool is_set(size_t index) const {
    const size_t word_index = index / WORD_SIZE_BITS;
    const size_t word_offset = index % WORD_SIZE_BITS;
    auto word_atomic = std::atomic_ref<uint64_t>(base[word_index]);

    const uint64_t bit_mask = 1ULL << word_offset;
    return (word_atomic.load() & bit_mask) != 0;
  }

  size_t size_in_bits() const {
    return bits_to_track;
  }

  constexpr static size_t invalid() {
    return ~0ULL;
  }

  // Debug interface
  // non-atomically returns the number of set bits in the bitset.
  size_t popcount() const {
    size_t count {};

    // Just ensure all store are visible.
    std::atomic_thread_fence(std::memory_order_release);

    for (size_t word_index = 0; word_index < words_to_track; ++word_index) {
      auto word_atomic = std::atomic_ref<uint64_t>(base[word_index]);
      count += std::popcount(word_atomic.load(std::memory_order_relaxed));
    }
    return count;
  }

private:
  uint64_t* base {};
  size_t bits_to_track {};
  size_t words_to_track {};
  struct data_to_track_nop {
    constexpr static size_t get_last_allocation() {
      return 0;
    }
    constexpr static void set_last_allocation(size_t) {}
  };

  struct data_to_track {
    std::atomic<size_t> last_allocation_word {};

    constexpr size_t get_last_allocation() const {
      // It's okay if this isn't up to date, full scan of the region still occurs.
      return last_allocation_word.load(std::memory_order_relaxed);
    }
    constexpr void set_last_allocation(size_t word) {
      last_allocation_word = word;
    }
  };
  using data_type = typename std::conditional<track_last_allocation, data_to_track, data_to_track_nop>::type;
  data_type last_allocation_track {};

  constexpr static size_t WORD_SIZE_BITS = sizeof(uint64_t) * 8;

  size_t allocate_one(size_t beginning_word_index, size_t ending_word_index) {
    // Trivial spin.
    for (size_t i = beginning_word_index; i < ending_word_index; ++i) {
      auto word_atomic = std::atomic_ref<uint64_t>(base[i]);
      auto expected_word = word_atomic.load(std::memory_order_relaxed);

      if (expected_word == ~0ULL) {
        // Won't pass.
        continue;
      }

      // Spin on the word trying to acquire a bit.
      // Uncontended case should immediately succeed.
      // Contended case can spin the whole word and lose every acquire.

      do {
        const auto zero_bit = std::countr_one(expected_word);
        const auto bit_mask = 1ULL << zero_bit;

        // If mask was already set, then we raced to acquire (returned value will be 1).
        // If mask not set, then we will have acquired (returned value will be 0).
        expected_word = word_atomic.fetch_or(bit_mask);
        if ((expected_word & bit_mask) == 0) {
          // Acquired the bit, return the offset.
          last_allocation_track.set_last_allocation(i);
          return i * WORD_SIZE_BITS + zero_bit;
        }

        // Bit was already acquired.
        expected_word |= bit_mask;
      } while (expected_word != ~0ULL);
    }

    if constexpr (track_last_allocation) {
      if (beginning_word_index) {
        // One more chance to get an allocation.
        // Scan before the previous allocation to see if any free slots have appeared.
        return allocate_one(0, beginning_word_index);
      }
    }

    // Failure to acquire here.
    return invalid();
  }

  size_t allocate_inside_word(size_t count, size_t beginning_word_index, size_t ending_word_index) {
    for (size_t i = beginning_word_index; i < ending_word_index; ++i) {
      auto word_atomic = std::atomic_ref<uint64_t>(base[i]);
      auto expected_word = word_atomic.load(std::memory_order_relaxed);

      // Spin on the word trying to acquire a bit.
      // Uncontended case should immediately succeed.
      // Contended case can spin the whole word and lose every acquire.
      while (expected_word != ~0ULL) {
        auto zero_bit = std::countr_one(expected_word);
        bool fits = false;
        uint64_t bit_mask = count == WORD_SIZE_BITS ? ~0ULL : ((1ULL << count) - 1);
        for (; (zero_bit + count) <= WORD_SIZE_BITS; ++zero_bit) {
          // Check if the bits fit.
          uint64_t tmp_bit_mask = bit_mask << zero_bit;
          if ((expected_word & tmp_bit_mask) == 0) {
            fits = true;
            break;
          }
        }

        // Could never fit, early exit.
        if (!fits) {
          break;
        }

        // Shift bit_mask to the desired location.
        bit_mask <<= zero_bit;

        uint64_t desired_word {};
        bool acquired = true;

        do {
          if (expected_word & bit_mask) {
            // Couldn't acquire this field, move to the next.
            acquired = false;
            break;
          }

          // We desire setting a single bit.
          desired_word = expected_word | bit_mask;
        } while (!word_atomic.compare_exchange_strong(expected_word, desired_word));

        if (acquired) {
          // Acquired the bit, return the offset.
          last_allocation_track.set_last_allocation(i);
          return i * WORD_SIZE_BITS + zero_bit;
        }
      }
    }

    if constexpr (track_last_allocation) {
      if (beginning_word_index) {
        // One more chance to get an allocation.
        // Scan before the previous allocation to see if any free slots have appeared.
        return allocate_inside_word(count, 0, beginning_word_index);
      }
    }

    // Failure to acquire here.
    return invalid();
  }

  size_t allocate_large_amount(size_t count, size_t beginning_word_index, size_t ending_word_index) {
    // This version of the code needs to explicitly deal with large allocations that can't fit in a word.
    // So we are always scanning minimum 2 words.
    const size_t num_words_to_scan = FEXCore::AlignUpPowerOf2(count, WORD_SIZE_BITS) / WORD_SIZE_BITS;
    const size_t last_word_to_scan = ending_word_index - num_words_to_scan - 1;

    // Scan forward to find the first word.
    for (size_t base_index = beginning_word_index; base_index < last_word_to_scan;) {
      uint64_t leading_zeros {};

      size_t center_word_index = 1;
      bool has_center {};

      size_t tail_bits {};

      size_t remaining_bits = count;

      auto check_head_fitment = [&]() -> bool {
        auto base_word_atomic = std::atomic_ref<uint64_t>(base[base_index]);
        auto base_expected_word = base_word_atomic.load(std::memory_order_relaxed);

        // Count the leading zeros, if it is above zero then we can start here.
        leading_zeros = std::countl_zero(base_expected_word);

        if (leading_zeros == 0) {
          // Nope.
          ++base_index;
          return false;
        }

        return true;
      };

      auto check_center_fitment = [&]() -> bool {
        // Subtract the number of zeros.
        remaining_bits -= leading_zeros;

        has_center = remaining_bits >= WORD_SIZE_BITS;

        while (remaining_bits >= WORD_SIZE_BITS) {
          // All words in-between head and tail must be zero.
          auto center_word_atomic = std::atomic_ref<uint64_t>(base[base_index + center_word_index]);
          auto center_expected_word = center_word_atomic.load(std::memory_order_relaxed);

          if (center_expected_word != 0) {
            // Couldn't fit, won't ever fit in this range, so jump ahead to the last scanned item.
            // Might still be able to start on the tail of this word.
            base_index += center_word_index;
            return false;
          }

          ++center_word_index;
          remaining_bits -= WORD_SIZE_BITS;
        }

        return true;
      };

      auto check_tail_fitment = [&]() -> bool {
        tail_bits = remaining_bits;
        if (tail_bits) {
          // Now for the tail (if it is necessary).
          size_t tail_index = center_word_index;

          // Count the trailing zeros, if it fits out remaining bits then we can try and allocate.
          auto tail_word_atomic = std::atomic_ref<uint64_t>(base[base_index + tail_index]);
          auto tail_expected_word = tail_word_atomic.load(std::memory_order_relaxed);

          const auto trailing_zeros = std::countr_zero(tail_expected_word);

          if (trailing_zeros < remaining_bits) {
            // Couldn't fit, but also won't ever fit in this range. Jump ahead to this tail item.
            // Might still be able to start on the tail of this word.
            base_index += tail_index;
            return false;
          }
        }

        return true;
      };

      if (!check_head_fitment()) {
        continue;
      }

      if (!check_center_fitment()) {
        continue;
      }

      if (!check_tail_fitment()) {
        continue;
      }

      // We can try fitting!
      if (attempt_allocate_range_from_base(base_index, count, leading_zeros, tail_bits, has_center)) {
        const size_t head_leading_offset = (WORD_SIZE_BITS - leading_zeros);
        return base_index * WORD_SIZE_BITS + head_leading_offset;
      }

      // Failure to fit here means we could never fit in this full range. Jump past all the bits
      // Rescanning the tail to avoid fragmented sparsity on the tails.
      base_index += num_words_to_scan - 1;
    }

    return invalid();
  }

  bool attempt_allocate_range_from_base(size_t base_index, size_t count, size_t head_bits, size_t tail_bits, bool has_center) {
    const bool has_tail = tail_bits != 0;

    const uint64_t head_bit_mask = head_bits == WORD_SIZE_BITS ? ~0ULL : (((1ULL << head_bits) - 1) << (WORD_SIZE_BITS - head_bits));
    const uint64_t tail_bit_mask = (1ULL << tail_bits) - 1;
    const uint64_t center_words_count = (count - head_bits - tail_bits) / WORD_SIZE_BITS;
    const uint64_t center_words_base_index = base_index + 1;
    const uint64_t tail_word_base_index = center_words_base_index + center_words_count;

    // Three distinct sections, all of which need to support rewinding.
    // - Head: Setting the leading zeros to one
    //   - Always exists. Can be a full word, or partial.
    // - Center: Setting all in-between words to ~0ULL
    //   - Might not exist if tail is smaller than a word
    //   - Always full words if it does exist.
    // - Tail: Set all trailing zeros up to the size to 1
    //   - Might not exist if Center perfectly aligned to word edge.
    //   - Always partial words, otherwise it would be considered "Center".

    bool set_head {true};
    bool set_center {true};
    bool set_tail {true};
    size_t num_center_set {};

    // Head first.
    auto set_head_word = [&]() -> bool {
      auto head_word_atomic = std::atomic_ref<uint64_t>(base[base_index]);
      auto head_expected_word = head_word_atomic.load(std::memory_order_relaxed);

      uint64_t desired_word {};
      do {
        if (head_expected_word & head_bit_mask) {
          // Another thread raced and allocated.
          return false;
        }

        // Set the whole mask in one atomic operation.
        desired_word = head_expected_word | head_bit_mask;
      } while (!head_word_atomic.compare_exchange_strong(head_expected_word, desired_word));

      return true;
    };

    auto clear_head_word = [&]() {
      auto head_word_atomic = std::atomic_ref<uint64_t>(base[base_index]);
      head_word_atomic.fetch_and(~head_bit_mask);
    };

    auto set_center_words = [&]() -> bool {
      const size_t end_center_word_index = center_words_base_index + center_words_count;
      for (size_t center_index = center_words_base_index; center_index < end_center_word_index; ++center_index) {
        auto center_word_atomic = std::atomic_ref<uint64_t>(base[center_index]);
        auto center_expected_word = center_word_atomic.load(std::memory_order_relaxed);

        // Center words set a full ~0ULL mask.
        const uint64_t desired_word {~0ULL};
        do {
          if (center_expected_word) {
            // Another thread raced and allocated.
            return false;
          }
        } while (!center_word_atomic.compare_exchange_strong(center_expected_word, desired_word));

        ++num_center_set;
      }

      return true;
    };

    auto clear_center_words = [&]() {
      const size_t end_center_word_index = center_words_base_index + num_center_set;
      for (size_t center_index = center_words_base_index; center_index < end_center_word_index; ++center_index) {
        auto center_word_atomic = std::atomic_ref<uint64_t>(base[center_index]);
        center_word_atomic.store(0);
      }
    };

    auto set_tail_word = [&]() -> bool {
      auto tail_word_atomic = std::atomic_ref<uint64_t>(base[tail_word_base_index]);
      auto tail_expected_word = tail_word_atomic.load(std::memory_order_relaxed);

      uint64_t desired_word {};
      do {
        if (tail_expected_word & tail_bit_mask) {
          // Another thread raced and allocated.
          return false;
        }

        // Set the whole mask in one atomic operation.
        desired_word = tail_expected_word | tail_bit_mask;
      } while (!tail_word_atomic.compare_exchange_strong(tail_expected_word, desired_word));

      return true;
    };

    set_head = set_head_word();

    // Do the center if it exists.
    if (set_head && has_center) {
      set_center = set_center_words();
    }

    // Do the tail if it exists.
    if (set_head && set_center && has_tail) {
      set_tail = set_tail_word();
    }

    if (set_head && set_center && set_tail) {
      return true;
    }

    // Some stage failed to set, rewind everything.
    if (set_head) {
      // Clear the head if it was set
      clear_head_word();
    }

    if (has_center && num_center_set) {
      // `set_center` might not be set, but it still managed to set some of the words.
      clear_center_words();
    }

    // Tail doesn't need to rewind as it will never have been set if we got here.
    return false;
  }

#if defined(__ARM_FEATURE_ATOMICS) && __ARM_FEATURE_ATOMICS == 1
  // Might violate memory-ordering requirements?
  // TODO: Verify and enable or delete depending.
  // Provides an 11% (Cortex-X4) to 25% (AmpereOneA) performance improvement.
  constexpr static bool use_stclr {};
  static inline void stclr(uint64_t value, uint64_t* addr) {
    asm volatile("stclrl %[Val], [%[addr]];" ::[Val] "r"(value), [addr] "r"(addr) : "memory");
  }
#endif

  void free_one(size_t index) {
    const size_t word_index = index / WORD_SIZE_BITS;
    const size_t word_offset = index % WORD_SIZE_BITS;
    last_allocation_track.set_last_allocation(word_index);

    const uint64_t bic_bit_mask = 1ULL << word_offset;
#if defined(__ARM_FEATURE_ATOMICS) && __ARM_FEATURE_ATOMICS == 1
    if constexpr (use_stclr) {
      stclr(bic_bit_mask, &base[word_index]);
      return;
    }
#endif

    auto word_atomic = std::atomic_ref<uint64_t>(base[word_index]);
    word_atomic.fetch_and(~bic_bit_mask);
  }

  void free_inside_word(size_t index, size_t count) {
    const size_t word_index = index / WORD_SIZE_BITS;
    const size_t word_offset = index % WORD_SIZE_BITS;
    auto word_atomic = std::atomic_ref<uint64_t>(base[word_index]);
    last_allocation_track.set_last_allocation(word_index);

    if (count == WORD_SIZE_BITS) {
      word_atomic.store(0);
      return;
    }

    const uint64_t bic_bit_mask = ((1ULL << count) - 1) << word_offset;
#if defined(__ARM_FEATURE_ATOMICS) && __ARM_FEATURE_ATOMICS == 1
    if constexpr (use_stclr) {
      stclr(bic_bit_mask, &base[word_index]);
      return;
    }
#endif
    word_atomic.fetch_and(~bic_bit_mask);
  }

  void free_large_amount(size_t index, size_t count) {
    // Incoming count can be less than WORD_SIZE_BITS if it is unaligned and crossing multiple words.
    // Needs to always handle at minimum a head plus center and/or tail arrangement.
    const size_t base_index = index / WORD_SIZE_BITS;
    const size_t last_index = FEXCore::AlignUpPowerOf2(index + count, WORD_SIZE_BITS) / WORD_SIZE_BITS;
    const size_t num_words_to_scan = last_index - base_index;
    LOGMAN_THROW_A_FMT(num_words_to_scan > 1, "Needs to be larger than 1 ({}, {})", index, count);

    size_t remaining_bits = count;

    const uint64_t head_offset_start = index % WORD_SIZE_BITS;
    const uint64_t head_bits = WORD_SIZE_BITS - head_offset_start;

    uint64_t head_mask = head_bits == WORD_SIZE_BITS ? ~0ULL : (((1ULL << head_bits) - 1) << head_offset_start);

    remaining_bits -= head_bits;
    auto head_word_atomic = std::atomic_ref<uint64_t>(base[base_index]);
    head_word_atomic.fetch_and(~head_mask);

    const size_t remaining_center_words = remaining_bits / WORD_SIZE_BITS;
    for (size_t i = 0; i < remaining_center_words; ++i) {
      // Handle centers if they exist.
      auto center_word_atomic = std::atomic_ref<uint64_t>(base[base_index + i + 1]);
      center_word_atomic.store(0);
      remaining_bits -= WORD_SIZE_BITS;
    }

    if (remaining_bits) {
      // Handle tail if they exist, must always be less than WORD_SIZE_BITS.
      LOGMAN_THROW_A_FMT(remaining_bits < WORD_SIZE_BITS, "Too large");
      const uint64_t tail_mask = (1ULL << remaining_bits) - 1;

      auto tail_word_atomic = std::atomic_ref<uint64_t>(base[base_index + remaining_center_words + 1]);
      tail_word_atomic.fetch_and(~tail_mask);
    }
  }
};
} // namespace FEXCore::Utils
