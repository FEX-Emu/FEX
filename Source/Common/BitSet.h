#pragma once
#include "Common/MathUtils.h"

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
		Memory = static_cast<ElementType*>(malloc(AlignUp(Elements / MinimumSize, MinimumSize) * MinimumSize));
	}
	void Realloc(size_t Elements) {
		Memory = static_cast<ElementType*>(realloc(Memory, AlignUp(Elements / MinimumSize, MinimumSize) * MinimumSize));
	}
	void Free() {
		free(Memory);
		Memory = nullptr;
	}
	bool Get(T Element) {
		return (Memory[Element / MinimumSizeBits] & (1 << (Element % MinimumSizeBits))) != 0;
	}
	void Set(T Element) {
		Memory[Element / MinimumSizeBits] |= (1ULL << (Element % MinimumSizeBits));
	}
	void Clear(T Element) {
		Memory[Element / MinimumSizeBits] &= (1ULL << (Element % MinimumSizeBits));
	}
	void MemClear(size_t Size) {
		memset(Memory, 0, Size);
	}
	void MemSet(size_t Size) {
		memset(Memory, 0xFF, Size);
	}

	// This very explicitly doesn't let you take an address
	// Is only a getter
	bool operator[](T Element) {
		return Get(Element);
	}
};

static_assert(sizeof(BitSet<uint32_t>) == sizeof(uintptr_t), "Needs to just be a pointer");
static_assert(std::is_pod<BitSet<uint32_t>>::value, "Needs to POD");
