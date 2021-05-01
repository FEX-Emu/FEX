#pragma once
#include "HostAllocator.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

template<typename T>
struct FlexBitSet final {
  using ElementType = T;
  constexpr static size_t MinimumSize = sizeof(ElementType);
  constexpr static size_t MinimumSizeBits = sizeof(ElementType) * 8;

  T Memory[];

  bool Get(T Element) {
    return (Memory[Element / MinimumSizeBits] & (1ULL << (Element % MinimumSizeBits))) != 0;
  }
  bool TestAndClear(T Element) {
    bool Value = Get(Element);
    Memory[Element / MinimumSizeBits] &= ~(1ULL << (Element % MinimumSizeBits));
    return Value;
  }
  void Set(T Element) {
    Memory[Element / MinimumSizeBits] |= (1ULL << (Element % MinimumSizeBits));
  }
  void Clear(T Element) {
    Memory[Element / MinimumSizeBits] &= ~(1ULL << (Element % MinimumSizeBits));
  }
  void MemClear(size_t Elements) {
    memset(Memory, 0, Alloc::AlignUp(Elements / MinimumSizeBits, MinimumSizeBits));
  }
  void MemSet(size_t Elements) {
    memset(Memory, 0xFF, Alloc::AlignUp(Elements / MinimumSizeBits, MinimumSizeBits));
  }

  // This very explicitly doesn't let you take an address
  // Is only a getter
  bool operator[](T Element) {
    return Get(Element);
  }

  static size_t Size(T Elements) {
    return Alloc::AlignUp(Elements / MinimumSizeBits, MinimumSizeBits);
  }
};

static_assert(sizeof(FlexBitSet<uint64_t>) == 0, "This needs to be a flex member");
static_assert(std::is_trivially_copyable<FlexBitSet<uint64_t>>::value, "Needsto be trivially copyable");
