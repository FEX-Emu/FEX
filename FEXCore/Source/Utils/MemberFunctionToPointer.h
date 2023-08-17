#pragma once

#include <FEXCore/Utils/LogManager.h>

#include <cstdint>

namespace FEXCore::Utils {

/**
 * @brief Casts a class's member function pointer to a raw pointer that we can JIT
 *
 * Has additional validation to ensure we aren't casting a class member that is invalid
 */
template <typename PointerToMemberType>
class MemberFunctionToPointerCast final {
  public:
    MemberFunctionToPointerCast(PointerToMemberType Function) {
      memcpy(&PMF, &Function, sizeof(PMF));

#ifdef _M_X86_64
      // Itanium C++ ABI (https://itanium-cxx-abi.github.io/cxx-abi/abi.html#member-function-pointers)
      // Low bit of ptr specifies if this Member function pointer is virtual or not
      // Throw an assert if we were trying to cast a virtual member
      LOGMAN_THROW_AA_FMT((PMF.ptr & 1) == 0, "C++ Pointer-To-Member representation didn't have low bit set to 0. Are you trying to cast a virtual member?");
#elif defined(_M_ARM_64 )
      // C++ ABI for the Arm 64-bit Architecture (IHI 0059E)
      // 4.2.1 Representation of pointer to member function
      // Differs from Itanium specification
      LOGMAN_THROW_AA_FMT(PMF.adj == 0, "C++ Pointer-To-Member representation didn't have adj == 0. Are you trying to cast a virtual member?");
#else
#error Don't know how to cast Member to function here. Likely just Itanium
#endif
    }

    uintptr_t GetConvertedPointer() const {
      return PMF.ptr;
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
}
