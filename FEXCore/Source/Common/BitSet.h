// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/MathUtils.h>
#include <FEXCore/Utils/LogManager.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <type_traits>

namespace FEXCore {

template<typename T>
struct BitSet final {
  using ElementType = T;
  constexpr static size_t MinimumSize = sizeof(ElementType);
  constexpr static size_t MinimumSizeBits = sizeof(ElementType) * 8;

  ElementType* Memory;
  void Allocate(size_t Elements) {
    size_t AllocateSize = ToBytes(Elements);
    LOGMAN_THROW_A_FMT((AllocateSize * MinimumSize) >= Elements, "Fail");
    Memory = static_cast<ElementType*>(FEXCore::Allocator::malloc(AllocateSize));
  }
  void Realloc(size_t Elements) {
    size_t AllocateSize = ToBytes(Elements);
    LOGMAN_THROW_A_FMT((AllocateSize * MinimumSize) >= Elements, "Fail");
    Memory = static_cast<ElementType*>(FEXCore::Allocator::realloc(Memory, AllocateSize));
  }
  void Free() {
    FEXCore::Allocator::free(Memory);
    Memory = nullptr;
  }
  bool Get(T Element) {
    return (Memory[Element / MinimumSizeBits] & (1ULL << (Element % MinimumSizeBits))) != 0;
  }
  void Set(T Element) {
    Memory[Element / MinimumSizeBits] |= (1ULL << (Element % MinimumSizeBits));
  }
  void Clear(T Element) {
    Memory[Element / MinimumSizeBits] &= (1ULL << (Element % MinimumSizeBits));
  }
  void MemClear(size_t Elements) {
    memset(Memory, 0, ToBytes(Elements));
  }
  void MemSet(size_t Elements) {
    memset(Memory, 0xFF, ToBytes(Elements));
  }
  uint32_t ToBytes(size_t Elements) {
    return AlignUp(Elements, MinimumSizeBits) / MinimumSize;
  }

  // This very explicitly doesn't let you take an address
  // Is only a getter
  bool operator[](T Element) {
    return Get(Element);
  }
};

template<typename T>
struct BitSetView final {
  using ElementType = T;
  constexpr static size_t MinimumSize = sizeof(ElementType);
  constexpr static size_t MinimumSizeBits = sizeof(ElementType) * 8;

  ElementType* Memory;

  void GetView(BitSet<T>& Set, uint64_t ElementOffset) {
    LOGMAN_THROW_A_FMT((ElementOffset % MinimumSize) == 0, "Bitset view offset needs to be aligned to size of backing element");
    Memory = &Set.Memory[ElementOffset / MinimumSizeBits];
  }

  bool Get(T Element) {
    return (Memory[Element / MinimumSizeBits] & (1ULL << (Element % MinimumSizeBits))) != 0;
  }
  void Set(T Element) {
    Memory[Element / MinimumSizeBits] |= (1ULL << (Element % MinimumSizeBits));
  }
  void Clear(T Element) {
    Memory[Element / MinimumSizeBits] &= (1ULL << (Element % MinimumSizeBits));
  }
  void MemClear(size_t Elements) {
    memset(Memory, 0, AlignUp(Elements / MinimumSizeBits, MinimumSizeBits));
  }
  void MemSet(size_t Elements) {
    memset(Memory, 0xFF, AlignUp(Elements / MinimumSizeBits, MinimumSizeBits));
  }

  // This very explicitly doesn't let you take an address
  // Is only a getter
  bool operator[](T Element) {
    return Get(Element);
  }
};

static_assert(sizeof(BitSet<uint32_t>) == sizeof(uintptr_t), "Needs to just be a pointer");
static_assert(std::is_trivially_copyable_v<BitSet<uint32_t>>, "Needs to trivially copyable");

static_assert(sizeof(BitSetView<uint32_t>) == sizeof(uintptr_t), "Needs to just be a pointer");
static_assert(std::is_trivially_copyable_v<BitSetView<uint32_t>>, "Needs to trivially copyable");

} // namespace FEXCore
