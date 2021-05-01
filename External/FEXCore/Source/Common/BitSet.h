#pragma once
#include "Common/MathUtils.h"
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/LogManager.h>

#include <cstring>
#include <stdint.h>
#include <stdlib.h>
#include <type_traits>

template<typename T>
struct BitSet final {
  using ElementType = T;
  constexpr static size_t MinimumSize = sizeof(ElementType);
  constexpr static size_t MinimumSizeBits = sizeof(ElementType) * 8;

  ElementType *Memory;
  void Allocate(size_t Elements) {
    size_t AllocateSize = AlignUp(Elements, MinimumSizeBits) / MinimumSize;
    LOGMAN_THROW_A((AllocateSize * MinimumSize) >= Elements, "Fail");
    Memory = static_cast<ElementType*>(FEXCore::Allocator::malloc(AllocateSize));
  }
  void Realloc(size_t Elements) {
    size_t AllocateSize = AlignUp(Elements, MinimumSizeBits) / MinimumSize;
    LOGMAN_THROW_A((AllocateSize * MinimumSize) >= Elements, "Fail");
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

template<typename T>
struct BitSetView final {
  using ElementType = T;
  constexpr static size_t MinimumSize = sizeof(ElementType);
  constexpr static size_t MinimumSizeBits = sizeof(ElementType) * 8;

  ElementType *Memory;

  void GetView(BitSet<T> &Set, uint64_t ElementOffset) {
    LOGMAN_THROW_A((ElementOffset % MinimumSize) == 0,
        "Bitset view offset needs to be aligned to size of backing element");
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
static_assert(std::is_trivially_copyable<BitSet<uint32_t>>::value, "Needs to trivially copyable");

static_assert(sizeof(BitSetView<uint32_t>) == sizeof(uintptr_t), "Needs to just be a pointer");
static_assert(std::is_trivially_copyable<BitSetView<uint32_t>>::value, "Needs to trivially copyable");
