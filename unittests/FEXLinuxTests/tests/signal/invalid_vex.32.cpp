#include "invalid_util.h"

#include <catch2/catch_test_macros.hpp>

#include <signal.h>
#include <sys/mman.h>
#include <stdint.h>
#include <cstdlib>

extern "C" void IntInstruction();

#pragma GCC diagnostic ignored "-Wattributes" // Suppress warning in case control-flow checks aren't enabled
__attribute__((naked, nocf_check)) static void InvalidINT() {
  __asm volatile(R"(
  IntInstruction:
  // vaddss xmm0,xmm15,xmm2
  .byte 0xc5, 0x82, 0x58, 0xc2;
  ret;
  )");
}

unsigned long EXPECTED_RIP = reinterpret_cast<unsigned long>(&IntInstruction);

TEST_CASE("Signals: Invalid VEX.vvvv") {
  capturing_handler_skip = 4;
  struct sigaction act {};
  act.sa_sigaction = CapturingHandler;
  act.sa_flags = SA_SIGINFO;
  sigaction(SIGSEGV, &act, nullptr);
  sigaction(SIGTRAP, &act, nullptr);
  sigaction(SIGILL, &act, nullptr);

  InvalidINT();

#ifndef REG_RIP
#define REG_RIP REG_EIP
#endif

  REQUIRE(from_handler.has_value());
  CHECK(from_handler->mctx.gregs[REG_RIP] == EXPECTED_RIP);
}
