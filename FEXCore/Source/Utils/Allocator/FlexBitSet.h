// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Utils/MathUtils.h>
#include <FEXCore/Utils/LogManager.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace FEXCore {

template<typename T>
struct FlexBitSet final {
  using ElementType = T;
  constexpr static size_t MinimumSize = sizeof(ElementType);
  constexpr static size_t MinimumSizeBits = sizeof(ElementType) * 8;

  T Memory[];

  bool Get(size_t Element) const {
    return (Memory[Element / MinimumSizeBits] & (1ULL << (Element % MinimumSizeBits))) != 0;
  }
  bool TestAndClear(size_t Element) {
    bool Value = Get(Element);
    Memory[Element / MinimumSizeBits] &= ~(1ULL << (Element % MinimumSizeBits));
    return Value;
  }
  void Set(size_t Element) {
    Memory[Element / MinimumSizeBits] |= (1ULL << (Element % MinimumSizeBits));
  }
  void Clear(size_t Element) {
    Memory[Element / MinimumSizeBits] &= ~(1ULL << (Element % MinimumSizeBits));
  }
  void MemClear(size_t Elements) {
    memset(Memory, 0, FEXCore::AlignUp(Elements / MinimumSizeBits, MinimumSizeBits));
  }
  void MemSet(size_t Elements) {
    memset(Memory, 0xFF, FEXCore::AlignUp(Elements / MinimumSizeBits, MinimumSizeBits));
  }

  // Range scanning results
  struct BitsetScanResults {
    // Which element was found. ~0ULL if not found.
    size_t FoundElement;
    // During the scan, found a hole in the allocations that didn't fit.
    bool FoundHole;
  };

  // TODO: Make {Forward,Backward}ScanForRange faster
  // Currently these functions test a single bit at a time, which is fairly costly.
  // The compiler emits a full element load per iteration, wasting a bunch of time on loads.
  // If we change these functions to have a pre-amble and post-amble to align the primary loop to the element size then this can go
  // significantly faster.
  //
  // Once the element scanning is aligned to the element size, we can then use native count leading zero(CLZ) and count trailing zero(CTZ)
  // instructions on a full element to scan uint64_t elements per loop iteration.

  // Implementation details:
  // Template argument WantUnset
  // Used to determine if the desired range is for set or unset ranges.
  // Typically `WantUnset` should be true. Used for finding a unset range inside of a range will set elements.
  //
  // @param BeginningElement - The first element in the set to start scanning from.
  // @param ElementCount - How many elements to find a range for fitting.
  // @param MinimumElement - Minimum element in the set to search to
  //
  // @return The scan results
  template<bool WantUnset>
  BitsetScanResults BackwardScanForRange(size_t BeginningElement, size_t ElementCount, size_t MinimumElement) {
    bool FoundHole {};

    // Final element to iterate to.
    const size_t FinalElement = MinimumElement + ElementCount - 1;

    for (size_t CurrentPage = BeginningElement; CurrentPage >= FinalElement;) {
      size_t Remaining = ElementCount;
      LOGMAN_THROW_A_FMT(CurrentPage <= BeginningElement && CurrentPage >= FinalElement, "BackwardScanForRange: Scanning less than "
                                                                                         "available range");

      while (Remaining) {
        if (this->Get(CurrentPage - Remaining + 1) == WantUnset) {
          // Has an intersecting range
          break;
        }
        --Remaining;
      }

      if (Remaining) {
        // If we found at least one Element hole then track that
        if (Remaining != ElementCount) {
          FoundHole = true;
        }

        // Didn't find a slab range
        CurrentPage -= Remaining;
      } else {
        // We have a slab range
        return BitsetScanResults {CurrentPage - ElementCount + 1, FoundHole};
      }
    }

    return BitsetScanResults {~0ULL, FoundHole};
  }

  // @param BeginningElement - The first element in the set to start scanning from.
  // @param ElementCount - How many elements to find a range for fitting.
  // @param ElementsInSet - How many elements are in the full set.
  //
  // @return The scan results
  template<bool WantUnset>
  BitsetScanResults ForwardScanForRange(size_t BeginningElement, size_t ElementCount, size_t ElementsInSet) {
    bool FoundHole {};

    // Final element to iterate to.
    const size_t FinalElement = ElementsInSet - ElementCount + 1;

    for (size_t CurrentElement = BeginningElement; CurrentElement <= FinalElement;) {
      // If we have enough free space, check if we have enough free pages that are contiguous
      size_t Remaining = ElementCount;

      LOGMAN_THROW_A_FMT(CurrentElement >= BeginningElement && CurrentElement <= FinalElement, "ForwardScanForRange: Scanning less than "
                                                                                               "available range");

      while (Remaining) {
        if (this->Get(CurrentElement + Remaining - 1) == WantUnset) {
          // Has an intersecting range
          break;
        }
        --Remaining;
      }

      if (Remaining) {
        // If we found at least one Element hole then track that
        if (Remaining != ElementCount) {
          FoundHole = true;
        }

        // Didn't find a slab range
        CurrentElement += Remaining;
      } else {
        // We have a slab range
        return BitsetScanResults {CurrentElement, FoundHole};
      }
    }

    return BitsetScanResults {~0ULL, FoundHole};
  }

  // This very explicitly doesn't let you take an address
  // Is only a getter
  bool operator[](size_t Element) const {
    return Get(Element);
  }

  // Returns the number of bits required to hold the number of elements.
  // Just rounds up to the MinimumSizeInBits.
  constexpr static size_t SizeInBits(uint64_t Elements) {
    return FEXCore::AlignUp(Elements, MinimumSizeBits);
  }
  // Returns the number of bytes required to hold the number of elements.
  constexpr static size_t SizeInBytes(uint64_t Elements) {
    return SizeInBits(Elements) / 8;
  }
};

static_assert(sizeof(FlexBitSet<uint64_t>) == 0, "This needs to be a flex member");
static_assert(std::is_trivially_copyable_v<FlexBitSet<uint64_t>>, "Needs to be trivially copyable");

} // namespace FEXCore
