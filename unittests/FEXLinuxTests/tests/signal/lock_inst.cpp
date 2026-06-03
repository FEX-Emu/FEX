#include "invalid_util.h"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <signal.h>
#include <sys/mman.h>
#include <stdint.h>
#include <stdio.h>
#include <cstdlib>

extern "C" void FaultInstruction();
extern "C" void FaultInstruction2();

extern "C" void RetInstruction();

#pragma GCC diagnostic ignored "-Wattributes" // Suppress warning in case control-flow checks aren't enabled

__attribute__((naked, nocf_check, fastcall)) static void TestFunction(size_t* Data, size_t Incoming) {
  __asm volatile("FaultInstruction:\n"
#ifndef REG_RIP
                 "lock shrd [ecx], edx, 1;"
#else
                 "lock shrd [rdi], rsi, 1;"
#endif
                 "RetInstruction:\n"
                 "ret;\n");
}

__attribute__((naked, nocf_check, fastcall)) static void TestFunction2(size_t Data, size_t Incoming) {
  __asm volatile("FaultInstruction2:\n"
                 ".byte 0xf0;\n" // Lock prefix
#ifndef REG_RIP
                 "xchg ecx, edx;"
#else
                 "xchg rdi, rsi;"
#endif
  );
}

unsigned long EXPECTED_RIP = reinterpret_cast<unsigned long>(&FaultInstruction);
constexpr int EXPECTED_TRAPNO = 6;
constexpr int EXPECTED_ERR = 0;
constexpr int EXPECTED_SI_CODE = 2;
constexpr int EXPECTED_SIGNAL = SIGILL;
constexpr int EXPECTED_SIGNAL_COUNT = 1;

TEST_CASE("Signals: Invalid Lock") {
  // FEX had a bug where it didn't check for invalid LOCK prefix.
  // Ensure that instructions that don't support LOCK are considered illegal.
  // Ensure that instructions that support LOCK raise illegal instruction when destination isn't memory.
  capturing_handler_goto = reinterpret_cast<void*>(&RetInstruction);
  struct sigaction act {};
  act.sa_sigaction = CapturingHandler;
  act.sa_flags = SA_SIGINFO;
  sigaction(SIGILL, &act, nullptr);

  static size_t Data {~0U};
  size_t DataIncoming {0x41424344U};

#ifndef REG_RIP
#define REG_RIP REG_EIP
#endif

  EXPECTED_RIP = reinterpret_cast<unsigned long>(&FaultInstruction);
  TestFunction(&Data, DataIncoming);

  REQUIRE(from_handler.has_value());
  CHECK(from_handler->mctx.gregs[REG_RIP] == EXPECTED_RIP);
  CHECK(from_handler->mctx.gregs[REG_TRAPNO] == EXPECTED_TRAPNO);
  CHECK(from_handler->mctx.gregs[REG_ERR] == EXPECTED_ERR);
  CHECK(from_handler->si_code == EXPECTED_SI_CODE);
  CHECK(from_handler->signal == EXPECTED_SIGNAL);
  from_handler.reset();

  EXPECTED_RIP = reinterpret_cast<unsigned long>(&FaultInstruction2);
  TestFunction2(Data, DataIncoming);

  REQUIRE(from_handler.has_value());
  CHECK(from_handler->mctx.gregs[REG_RIP] == EXPECTED_RIP);
  CHECK(from_handler->mctx.gregs[REG_TRAPNO] == EXPECTED_TRAPNO);
  CHECK(from_handler->mctx.gregs[REG_ERR] == EXPECTED_ERR);
  CHECK(from_handler->si_code == EXPECTED_SI_CODE);
  CHECK(from_handler->signal == EXPECTED_SIGNAL);
  from_handler.reset();
}
