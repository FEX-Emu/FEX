#include "invalid_util.h"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <signal.h>
#include <sys/mman.h>
#include <stdint.h>
#include <stdio.h>
#include <cstdlib>

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
  }

// Instructions that explicitly are supported but must only work in CPL-0
TEST(0, "rdmsr", "rdmsr", X86_TRAPNO_GP, 0, 0x80, SIGSEGV);
TEST(1, "outs", "outs dx, byte ptr [rsi]", X86_TRAPNO_GP, 0, 0x80, SIGSEGV);
TEST(2, "ins", "ins byte ptr [rdi], dx", X86_TRAPNO_GP, 0, 0x80, SIGSEGV);
TEST(3, "cli", "cli", X86_TRAPNO_GP, 0, 0x80, SIGSEGV);
TEST(4, "clts", "clts", X86_TRAPNO_GP, 0, 0x80, SIGSEGV);
TEST(5, "invlpg", "invlpg [rax]", X86_TRAPNO_GP, 0, 0x80, SIGSEGV);
TEST(6, "lmsw", "lmsw [rax]", X86_TRAPNO_GP, 0, 0x80, SIGSEGV);
TEST(7, "ltr", "ltr [rax]", X86_TRAPNO_GP, 0, 0x80, SIGSEGV);
TEST(8, "mov cr0", "mov cr0, rax", X86_TRAPNO_GP, 0, 0x80, SIGSEGV);
TEST(9, "mov cr8", "mov cr8, rax", X86_TRAPNO_GP, 0, 0x80, SIGSEGV);
TEST(10, "mov rax, cr0", "mov rax, cr0", X86_TRAPNO_GP, 0, 0x80, SIGSEGV);
TEST(11, "mov rax, cr8", "mov rax, cr8", X86_TRAPNO_GP, 0, 0x80, SIGSEGV);
TEST(12, "mov rax, dr0", "mov rax, dr0", X86_TRAPNO_GP, 0, 0x80, SIGSEGV);
TEST(13, "mov dr0, rax", "mov dr0, rax", X86_TRAPNO_GP, 0, 0x80, SIGSEGV);
TEST(14, "rdpmc", "rdpmc", X86_TRAPNO_GP, 0, 0x80, SIGSEGV);
TEST(15, "sti", "sti", X86_TRAPNO_GP, 0, 0x80, SIGSEGV);
TEST(16, "swapgs", "swapgs", X86_TRAPNO_GP, 0, 0x80, SIGSEGV);
TEST(17, "sysret", "sysret", X86_TRAPNO_GP, 0, 0x80, SIGSEGV);
TEST(18, "wrmsr", "wrmsr", X86_TRAPNO_GP, 0, 0x80, SIGSEGV);

// Instructions not implemented
TEST(19, "monitor", "monitor", X86_TRAPNO_UD, 0, 2, SIGILL);
TEST(20, "mwait", "mwait", X86_TRAPNO_UD, 0, 2, SIGILL);
TEST(21, "sysenter", "sysenter", X86_TRAPNO_UD, 0, 2, SIGILL);
TEST(22, "sysexit", "sysexit", X86_TRAPNO_UD, 0, 2, SIGILL);

// Differs between dr8 and dr0-7 variants.
// dr0-7: SIGSEGV
// dr8-15: SIGILL
// TEST(20, "mov rax, dr8", "mov rax, dr8", X86_TRAPNO_UD, 0, 2, SIGILL);
