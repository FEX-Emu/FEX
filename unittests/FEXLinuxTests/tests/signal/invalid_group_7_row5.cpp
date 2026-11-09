#include "invalid_util.h"

#include <catch2/catch_test_macros.hpp>

#include <signal.h>
#include <sys/mman.h>
#include <stdint.h>

#ifndef REG_RIP
#define REG_RIP REG_EIP
#endif

enum trapno {
  X86_TRAPNO_UD = 6,
  X86_TRAPNO_GP = 13,
};

#define CONCAT(x, y) x##y
#define TestSymbols(num)                       \
  extern "C" uint64_t CONCAT(TestBegin_, num); \
  extern "C" uint64_t CONCAT(TestEnd_, num);

#define Test(num, asm, trapno, errno, si_code, signal)                                                      \
  capturing_handler_skip = (unsigned long)&CONCAT(TestEnd_, num) - (unsigned long)&CONCAT(TestBegin_, num); \
  const unsigned long EXPECTED_RIP = (unsigned long)&CONCAT(TestBegin_, num);                               \
  const int EXPECTED_TRAPNO = trapno;                                                                       \
  const int EXPECTED_ERR = errno;                                                                           \
  const int EXPECTED_SI_CODE = si_code;                                                                     \
  const int EXPECTED_SIGNAL = signal;                                                                       \
  __asm volatile("TestBegin_" #num ":" asm ";"                                                              \
                                           "TestEnd_" #num ":" ::                                           \
                                             : "memory");

#define TEST(num, name, asm, trapno, errno, _si_code, _signal)      \
  TestSymbols(num);                                                 \
  TEST_CASE("Signals: " #name) {                                    \
    struct sigaction act {};                                        \
    act.sa_sigaction = CapturingHandler;                            \
    act.sa_flags = SA_SIGINFO;                                      \
    sigaction(SIGSEGV, &act, nullptr);                              \
    sigaction(SIGTRAP, &act, nullptr);                              \
    sigaction(SIGILL, &act, nullptr);                               \
                                                                    \
    Test(num, asm, trapno, errno, _si_code, _signal);               \
                                                                    \
    REQUIRE(from_handler.has_value());                              \
    CHECK(from_handler->mctx.gregs[REG_RIP] == EXPECTED_RIP);       \
    CHECK(from_handler->mctx.gregs[REG_TRAPNO] == EXPECTED_TRAPNO); \
    CHECK(from_handler->mctx.gregs[REG_ERR] == EXPECTED_ERR);       \
    CHECK(from_handler->si_code == EXPECTED_SI_CODE);               \
    CHECK(from_handler->signal == EXPECTED_SIGNAL);                 \
    from_handler.reset();                                           \
  }

// Invalid group 7 row 5 instructions
// 0xe8 is serialize, so skipped.
TEST(0, "0xe9", ".byte 0x0f, 0x01, 0xe9", X86_TRAPNO_UD, 0, 2, SIGILL);
TEST(1, "0xea", ".byte 0x0f, 0x01, 0xea", X86_TRAPNO_UD, 0, 2, SIGILL);
TEST(2, "0xeb", ".byte 0x0f, 0x01, 0xeb", X86_TRAPNO_UD, 0, 2, SIGILL);
TEST(3, "0xec", ".byte 0x0f, 0x01, 0xec", X86_TRAPNO_UD, 0, 2, SIGILL);
TEST(4, "0xed", ".byte 0x0f, 0x01, 0xed", X86_TRAPNO_UD, 0, 2, SIGILL);
// Skipped as these are real instructions.
// TEST(5, "rdpkru", "rdpkru", X86_TRAPNO_UD, 0, 2, SIGILL);
// TEST(6, "wrpkru", "wrpkru", X86_TRAPNO_UD, 0, 2, SIGILL);

// Testing with prefixes as well.
TEST(5, "0x66 0xe9", ".byte 0x66, 0x0f, 0x01, 0xe9", X86_TRAPNO_UD, 0, 2, SIGILL);
TEST(6, "0x66 0xea", ".byte 0x66, 0x0f, 0x01, 0xea", X86_TRAPNO_UD, 0, 2, SIGILL);
TEST(7, "0x66 0xeb", ".byte 0x66, 0x0f, 0x01, 0xeb", X86_TRAPNO_UD, 0, 2, SIGILL);
TEST(8, "0x66 0xec", ".byte 0x66, 0x0f, 0x01, 0xec", X86_TRAPNO_UD, 0, 2, SIGILL);
TEST(9, "0x66 0xed", ".byte 0x66, 0x0f, 0x01, 0xed", X86_TRAPNO_UD, 0, 2, SIGILL);

TEST(10, "0xf2 0xe9", ".byte 0xf2, 0x0f, 0x01, 0xe9", X86_TRAPNO_UD, 0, 2, SIGILL);
TEST(11, "0xf2 0xea", ".byte 0xf2, 0x0f, 0x01, 0xea", X86_TRAPNO_UD, 0, 2, SIGILL);
TEST(12, "0xf2 0xeb", ".byte 0xf2, 0x0f, 0x01, 0xeb", X86_TRAPNO_UD, 0, 2, SIGILL);
TEST(13, "0xf2 0xec", ".byte 0xf2, 0x0f, 0x01, 0xec", X86_TRAPNO_UD, 0, 2, SIGILL);
TEST(14, "0xf2 0xed", ".byte 0xf2, 0x0f, 0x01, 0xed", X86_TRAPNO_UD, 0, 2, SIGILL);

TEST(15, "0xf3 0xe9", ".byte 0xf3, 0x0f, 0x01, 0xe9", X86_TRAPNO_UD, 0, 2, SIGILL);
TEST(16, "0xf3 0xea", ".byte 0xf3, 0x0f, 0x01, 0xea", X86_TRAPNO_UD, 0, 2, SIGILL);
TEST(17, "0xf3 0xeb", ".byte 0xf3, 0x0f, 0x01, 0xeb", X86_TRAPNO_UD, 0, 2, SIGILL);
TEST(18, "0xf3 0xec", ".byte 0xf3, 0x0f, 0x01, 0xec", X86_TRAPNO_UD, 0, 2, SIGILL);
TEST(19, "0xf3 0xed", ".byte 0xf3, 0x0f, 0x01, 0xed", X86_TRAPNO_UD, 0, 2, SIGILL);

#ifdef __x86_64__
TEST(20, "0x48 0xe9", ".byte 0x48, 0x0f, 0x01, 0xe9", X86_TRAPNO_UD, 0, 2, SIGILL);
TEST(21, "0x48 0xea", ".byte 0x48, 0x0f, 0x01, 0xea", X86_TRAPNO_UD, 0, 2, SIGILL);
TEST(22, "0x48 0xeb", ".byte 0x48, 0x0f, 0x01, 0xeb", X86_TRAPNO_UD, 0, 2, SIGILL);
TEST(23, "0x48 0xec", ".byte 0x48, 0x0f, 0x01, 0xec", X86_TRAPNO_UD, 0, 2, SIGILL);
TEST(24, "0x48 0xed", ".byte 0x48, 0x0f, 0x01, 0xed", X86_TRAPNO_UD, 0, 2, SIGILL);
#endif

// Test row 5 with modrm.mod != 11, aka r/m == memory
// These are all reserved as #UD.
TEST(25, "0x28 mod==0", ".byte 0x0f, 0x01, 0x28", X86_TRAPNO_UD, 0, 2, SIGILL);
TEST(26, "0x29 mod==0", ".byte 0x0f, 0x01, 0x29", X86_TRAPNO_UD, 0, 2, SIGILL);
TEST(27, "0x2a mod==0", ".byte 0x0f, 0x01, 0x2a", X86_TRAPNO_UD, 0, 2, SIGILL);
TEST(28, "0x2b mod==0", ".byte 0x0f, 0x01, 0x2b", X86_TRAPNO_UD, 0, 2, SIGILL);
TEST(29, "0x2c mod==0", ".byte 0x0f, 0x01, 0x2c", X86_TRAPNO_UD, 0, 2, SIGILL);
TEST(30, "0x2d mod==0", ".byte 0x0f, 0x01, 0x2d", X86_TRAPNO_UD, 0, 2, SIGILL);
TEST(31, "0x2e mod==0", ".byte 0x0f, 0x01, 0x2e", X86_TRAPNO_UD, 0, 2, SIGILL);
TEST(32, "0x2f mod==0", ".byte 0x0f, 0x01, 0x2f", X86_TRAPNO_UD, 0, 2, SIGILL);
