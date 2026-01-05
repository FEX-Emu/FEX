// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Utils/LogManager.h>

#include <cstdint>

namespace FEXCore::Utils {

/**
 * @brief Casts a class's member function pointer to a raw pointer that we can JIT
 *
 * Has additional validation to ensure we aren't casting a class member that is invalid
 */
template<typename PointerToMemberType>
class MemberFunctionToPointerCast final {
public:
  MemberFunctionToPointerCast(PointerToMemberType Function) {
    memcpy(&PMF, &Function, sizeof(PMF));
  }

  uintptr_t GetConvertedPointer() const {
#ifdef ARCHITECTURE_x86_64
    // Itanium C++ ABI (https://itanium-cxx-abi.github.io/cxx-abi/abi.html#member-function-pointers)
    // Low bit of ptr specifies if this Member function pointer is virtual or not
    // Throw an assert if we were trying to cast a virtual member
    LOGMAN_THROW_A_FMT((PMF.ptr & 1) == 0, "C++ Pointer-To-Member representation didn't have low bit set to 0. Are you trying to cast a "
                                           "virtual member?");
#elif defined(ARCHITECTURE_arm64)
    // C++ ABI for the Arm 64-bit Architecture (IHI 0059E)
    // 4.2.1 Representation of pointer to member function
    // Differs from Itanium specification
    LOGMAN_THROW_A_FMT(PMF.adj == 0, "C++ Pointer-To-Member representation didn't have adj == 0. Are you trying to cast a virtual member?");
#else
#error Don't know how to cast Member to function here. Likely just Itanium
#endif
    return PMF.ptr;
  }

  // Gets the vtable entry position of a virtual member function.
  size_t GetVTableOffset() const {
#ifdef ARCHITECTURE_x86_64
    // Itanium C++ ABI (https://itanium-cxx-abi.github.io/cxx-abi/abi.html#member-function-pointers)
    // Low bit of ptr specifies if this Member function pointer is virtual or not
    // Throw an assert if we are not loading a virtual member.
    LOGMAN_THROW_A_FMT((PMF.ptr & 1) == 1, "C++ Pointer-To-Member representation didn't have low bit set to 1. This cast only works for "
                                           "virtual members.");
    return PMF.ptr & ~1ULL;
#elif defined(ARCHITECTURE_arm64)
    // C++ ABI for the Arm 64-bit Architecture (IHI 0059E)
    // 4.2.1 Representation of pointer to member function
    // Differs from Itanium specification
    LOGMAN_THROW_A_FMT((PMF.adj & 1) == 1, "C++ Pointer-To-Member representation didn't have adj == 1. This cast only works for virtual "
                                           "members.");
    return PMF.ptr;
#else
#error Don't know how to cast Member to function here. Likely just Itanium
#endif
  }

  // Gets the pointer to the vtable entry for the object passed it.
  template<typename Class>
  uintptr_t GetVTableEntry(Class* VirtualClass) const {
    // VTable is always stored at the beginning of a class object.
    uintptr_t* VTable = *reinterpret_cast<uintptr_t**>(VirtualClass);

    size_t Offset = GetVTableOffset() / sizeof(void*);
    return VTable[Offset];
  }

private:
  struct PointerToMember {
    uintptr_t ptr;
    uintptr_t adj;
  };

  PointerToMember PMF;

  // Ensure the representation of PointerToMember matches
  static_assert(sizeof(PMF) == sizeof(PointerToMemberType));
};
} // namespace FEXCore::Utils
