#pragma once

#include <FEXCore/Utils/MathUtils.h>

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

  // This very explicitly doesn't let you take an address
  // Is only a getter
  bool operator[](size_t Element) const {
    return Get(Element);
  }

  static size_t Size(uint64_t Elements) {
    return FEXCore::AlignUp(Elements / MinimumSizeBits, MinimumSizeBits);
  }
};

static_assert(sizeof(FlexBitSet<uint64_t>) == 0, "This needs to be a flex member");
static_assert(std::is_trivially_copyable_v<FlexBitSet<uint64_t>>, "Needs to be trivially copyable");

} // namespace FEXCore
