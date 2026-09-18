#include <catch2/catch_test_macros.hpp>

#include <cstdint>

// Retries until the instruction reports success.
// clang-format off
#define RNG(insn, Value)            \
  __asm volatile(".Lretry%=:\n"     \
                 insn " %0\n"       \
                 "jnc .Lretry%=\n"  \
                 : "=r"(Value)      \
                 :                  \
                 : "cc")
// clang-format on

// This test is designed to test the software fallback implementation for RDRAND and RDSEED.
// To that end, it is executed with FEX_HOSTFEATURES=disablerng.

TEST_CASE("rdrand") {
  uint32_t eax, ebx, ecx, edx;
  __asm volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1), "c"(0));
  CHECK((ecx >> 30) & 1); // RDRAND
  __asm volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
  CHECK((ebx >> 18) & 1); // RDSEED

  uintptr_t A, B;
  RNG("rdrand", A);
  RNG("rdrand", B);

  // This assert is not technically correct,
  // but at a 1/(2^32) chance of failure, I like our odds.
  CHECK(A != B);

  RNG("rdseed", A);
  RNG("rdseed", B);
  CHECK(A != B);
}
