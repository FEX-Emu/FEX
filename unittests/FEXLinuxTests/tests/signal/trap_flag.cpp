#include "invalid_util.h"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <signal.h>
#include <sys/mman.h>
#include <stdint.h>
#include <stdio.h>
#include <cstdlib>

extern "C" void RetInstruction();

#pragma GCC diagnostic ignored "-Wattributes" // Suppress warning in case control-flow checks aren't enabled

#if __SIZEOF_POINTER__ == 4
__attribute__((naked, nocf_check)) static void TestTF() {
  __asm volatile(R"(
  pushfd;
  or dword ptr [esp], 0x100;
  popfd;
  nop;
  nop;
  nop;
  pushfd;
  and dword ptr [esp], ~0x100;
  popfd;
  RetInstruction:
  ret;
  )");
}
#else
__attribute__((naked, nocf_check)) static void TestTF() {
  __asm volatile(R"(
  pushfq;
  or qword ptr [rsp], 0x100;
  popfq;
  nop;
  nop;
  nop;
  pushfq;
  and qword ptr [rsp], ~0x100;
  popfq;
  RetInstruction:
  ret;
  )");
}
#endif

unsigned long EXPECTED_RIP = reinterpret_cast<unsigned long>(&RetInstruction);
constexpr int EXPECTED_TRAPNO = 1;
constexpr int EXPECTED_ERR = 0;
constexpr int EXPECTED_SI_CODE = 2;
constexpr int EXPECTED_SIGNAL = SIGTRAP;
constexpr int EXPECTED_SIGNAL_COUNT = 6;

TEST_CASE("Signals: Trap Flag") {
  capturing_handler_skip = 0;
  struct sigaction act {};
  act.sa_sigaction = CapturingHandler;
  act.sa_flags = SA_SIGINFO;
  sigaction(SIGTRAP, &act, nullptr);

  TestTF();

#ifndef REG_RIP
#define REG_RIP REG_EIP
#endif

  REQUIRE(from_handler.has_value());
  CHECK(from_handler->mctx.gregs[REG_RIP] == EXPECTED_RIP);
  CHECK(from_handler->mctx.gregs[REG_TRAPNO] == EXPECTED_TRAPNO);
  CHECK(from_handler->mctx.gregs[REG_ERR] == EXPECTED_ERR);
  CHECK(from_handler->si_code == EXPECTED_SI_CODE);
  CHECK(from_handler->signal == EXPECTED_SIGNAL);
  CHECK(capturing_handler_calls == EXPECTED_SIGNAL_COUNT);
}
