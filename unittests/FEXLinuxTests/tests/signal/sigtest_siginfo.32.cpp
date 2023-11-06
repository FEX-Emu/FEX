#include "invalid_util.h"

#include <catch2/catch.hpp>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstdint>
#include <syscall.h>

extern "C" {
extern void IntInstruction();
}
__attribute__((naked))
static void CauseInt() {
  __asm volatile(R"(
  IntInstruction:
  int 1;
  ret; # For RIP modification
  )");
}

static uint32_t EXPECTED_RIP = reinterpret_cast<uint32_t>(&IntInstruction);
constexpr int EXPECTED_TRAPNO = 13;
constexpr int EXPECTED_ERR = 10;
constexpr int EXPECTED_SI_CODE = 128;
constexpr int EXPECTED_SIGNAL = SIGSEGV;

struct ActionHandler {
  void* handler;
  uint32_t sa_mask;
  uint32_t sa_flags;
  void* restorer;
};

struct rt_ActionHandler {
  void* handler;

  uint32_t sa_flags;
  void* restorer;
  uint64_t sa_mask;
};

TEST_CASE("sigaction: no siginfo") {
  // On 32-bit, non-realtime sigaction still receives a context on the stack.
  // This is an implementation detail of Linux and not enforced by POSIX.
  // This can be modified by the userspace application as it is part of the uapi.
  // This is how Linux allows an application to modify its context on signal return.
  // This unit test is testing this by incrementing EIP by 2.
  capturing_handler_skip = 2;
  ActionHandler act{};
  act.handler = (void*)CapturingHandler_non_realtime;
  act.sa_flags = 0;
  syscall(SYS_sigaction, SIGSEGV, &act, nullptr);

  CauseInt();

  REQUIRE(from_handler_32.has_value());
  CHECK(from_handler_32->mctx.ip == EXPECTED_RIP);
  CHECK(from_handler_32->mctx.trapno == EXPECTED_TRAPNO);
  CHECK(from_handler_32->mctx.err == EXPECTED_ERR);
  CHECK(from_handler_32->signal == EXPECTED_SIGNAL);
}

TEST_CASE("sigaction: siginfo - regparm") {
  // On 32-bit, siginfo sigaction supports receiving siginfo and context in regparm.
  // This can be modified by the userspace application as it is part of the uapi.
  // This unit test is testing this by incrementing EIP by 2.
  capturing_handler_skip = 2;
  ActionHandler act{};
  act.handler = (void*)CapturingHandler_realtime_regparm;
  act.sa_flags = SA_SIGINFO;
  syscall(SYS_sigaction, SIGSEGV, &act, nullptr);

  CauseInt();

  REQUIRE(from_handler.has_value());
  CHECK((uint32_t)from_handler->mctx.gregs[REG_EIP] == EXPECTED_RIP);
  CHECK(from_handler->mctx.gregs[REG_TRAPNO] == EXPECTED_TRAPNO);
  CHECK(from_handler->mctx.gregs[REG_ERR] == EXPECTED_ERR);
  CHECK(from_handler->signal == EXPECTED_SIGNAL);
}

TEST_CASE("sigaction: no siginfo - regparm") {
  // On 32-bit, siginfo sigaction supports receiving siginfo and context in regparm.
  // This can be modified by the userspace application as it is part of the uapi.
  // This unit test is testing this by incrementing EIP by 2.
  capturing_handler_skip = 2;
  ActionHandler act{};
  act.handler = (void*)CapturingHandler_non_realtime_regparm;
  act.sa_flags = 0;
  syscall(SYS_sigaction, SIGSEGV, &act, nullptr);

  CauseInt();

  REQUIRE(from_handler_regparm_32.has_value());
  CHECK(from_handler_regparm_32->signal == EXPECTED_SIGNAL);
  CHECK(from_handler_regparm_32->siginfo == nullptr);
  CHECK(from_handler_regparm_32->context == nullptr);
}

TEST_CASE("sigaction: siginfo - stack") {
  // On 32-bit, siginfo sigaction put the frame on the stack.
  // This can be modified by the userspace application as it is part of the uapi.
  // This unit test is testing this by incrementing EIP by 2.
  capturing_handler_skip = 2;
  ActionHandler act{};
  act.handler = (void*)CapturingHandler_realtime;
  act.sa_flags = SA_SIGINFO;
  syscall(SYS_sigaction, SIGSEGV, &act, nullptr);

  CauseInt();

  REQUIRE(from_handler.has_value());
  CHECK(from_handler->mctx.gregs[REG_EIP] == EXPECTED_RIP);
  CHECK(from_handler->mctx.gregs[REG_TRAPNO] == EXPECTED_TRAPNO);
  CHECK(from_handler->mctx.gregs[REG_ERR] == EXPECTED_ERR);
  CHECK(from_handler->signal == EXPECTED_SIGNAL);
}

TEST_CASE("rt_sigaction: no siginfo") {
  // On 32-bit, classic rt_sigaction still receives a context on the stack.
  // This can be modified by the userspace application as it is part of the uapi.
  // This is how to modify the context on sigreturn.
  capturing_handler_skip = 2;
  rt_ActionHandler act{};
  act.handler = (void*)CapturingHandler_non_realtime;
  act.sa_flags = 0;
  syscall(SYS_rt_sigaction, SIGSEGV, &act, nullptr, 8);

  CauseInt();

  REQUIRE(from_handler_32.has_value());
  CHECK(from_handler_32->mctx.ip == EXPECTED_RIP);
  CHECK(from_handler_32->mctx.trapno == EXPECTED_TRAPNO);
  CHECK(from_handler_32->mctx.err == EXPECTED_ERR);
  CHECK(from_handler_32->signal == EXPECTED_SIGNAL);
}

TEST_CASE("rt_sigaction: siginfo - regparm") {
  // On 32-bit, a realtime sigaction supports arguments being received on the stack AND regparm.
  // This unit test ensures that the regparm implementation is working correctly.
  capturing_handler_skip = 2;
  rt_ActionHandler act{};
  act.handler = (void*)CapturingHandler_realtime_regparm;
  act.sa_flags = SA_SIGINFO;
  syscall(SYS_rt_sigaction, SIGSEGV, &act, nullptr, 8);

  CauseInt();

  REQUIRE(from_handler.has_value());
  CHECK(from_handler->mctx.gregs[REG_EIP] == EXPECTED_RIP);
  CHECK(from_handler->mctx.gregs[REG_TRAPNO] == EXPECTED_TRAPNO);
  CHECK(from_handler->mctx.gregs[REG_ERR] == EXPECTED_ERR);
  CHECK(from_handler->signal == EXPECTED_SIGNAL);
}

TEST_CASE("rt_sigaction: siginfo - stack") {
  // On 32-bit, a realtime sigaction supports arguments being received on the stack AND regparm.
  // This unit test ensures that the stack implementation is working correctly.
  capturing_handler_skip = 2;
  rt_ActionHandler act{};
  act.handler = (void*)CapturingHandler_realtime;
  act.sa_flags = SA_SIGINFO;
  syscall(SYS_rt_sigaction, SIGSEGV, &act, nullptr, 8);

  CauseInt();

  REQUIRE(from_handler.has_value());
  CHECK(from_handler->mctx.gregs[REG_EIP] == EXPECTED_RIP);
  CHECK(from_handler->mctx.gregs[REG_TRAPNO] == EXPECTED_TRAPNO);
  CHECK(from_handler->mctx.gregs[REG_ERR] == EXPECTED_ERR);
  CHECK(from_handler->signal == EXPECTED_SIGNAL);
}

TEST_CASE("sigaction: siginfo - glibc") {
  // Test to ensure that regular glibc sigaction works.
  capturing_handler_skip = 2;
  struct sigaction act{};
  act.sa_sigaction = CapturingHandler_realtime_glibc_helper;
  act.sa_flags = SA_SIGINFO;
  sigaction(SIGSEGV, &act, nullptr);

  CauseInt();

  REQUIRE(from_handler.has_value());
  CHECK(from_handler->mctx.gregs[REG_EIP] == EXPECTED_RIP);
  CHECK(from_handler->mctx.gregs[REG_TRAPNO] == EXPECTED_TRAPNO);
  CHECK(from_handler->mctx.gregs[REG_ERR] == EXPECTED_ERR);
  CHECK(from_handler->signal == EXPECTED_SIGNAL);
}
