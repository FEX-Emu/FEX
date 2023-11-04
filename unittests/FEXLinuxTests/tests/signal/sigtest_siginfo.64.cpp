#include "invalid_util.h"

#include <catch2/catch.hpp>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstdint>

extern "C" {
  extern void IntInstruction();
}
__attribute__((naked, nocf_check))
static void CauseInt() {
  __asm volatile(R"(
  IntInstruction:
  int 1;
  ret; # For RIP modification
  )");
}

static uint64_t EXPECTED_RIP = reinterpret_cast<uint64_t>(&IntInstruction);
constexpr int EXPECTED_TRAPNO = 13;
constexpr int EXPECTED_ERR = 10;
constexpr int EXPECTED_SI_CODE = 128;
constexpr int EXPECTED_SIGNAL = SIGSEGV;

TEST_CASE("siginfo") {
  // On x86-64, the signal handler receives siginfo even if SA_SIGINFO isn't set.
  // This flag is effectively a no-op, not changing behaviour.
  capturing_handler_skip = 2;
  struct sigaction act{};
  act.sa_sigaction = CapturingHandler;
  act.sa_flags = 0;
  sigaction(SIGSEGV, &act, nullptr);

  CauseInt();

  REQUIRE(from_handler.has_value());
  CHECK(from_handler->mctx.gregs[REG_RIP] == EXPECTED_RIP);
  CHECK(from_handler->mctx.gregs[REG_TRAPNO] == EXPECTED_TRAPNO);
  CHECK(from_handler->mctx.gregs[REG_ERR] == EXPECTED_ERR);
  CHECK(from_handler->signal == EXPECTED_SIGNAL);
}
