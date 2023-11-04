#include "invalid_util.h"

#include <catch2/catch.hpp>

#include <atomic>
#include <signal.h>
#include <sys/mman.h>
#include <stdint.h>
#include <stdio.h>
#include <cstdlib>

__attribute__((naked))
  static void InvalidINT_SetPF() {
  __asm volatile(R"(
  mov eax, 0x80
  inc eax
  int3;
  ret;
  )");
  }

__attribute__((naked))
  static void InvalidINT_ClearPF() {
  __asm volatile(R"(
  mov eax, 0
  inc eax
  int3;
  ret;
  )");
  }

__attribute__((naked))
  static void InvalidINT_SetCF() {
  __asm volatile(R"(
  stc
  int3;
  ret;
  )");
  }

__attribute__((naked))
  static void InvalidINT_ClearCF() {
  __asm volatile(R"(
  clc
  int3;
  ret;
  )");
  }

__attribute__((naked))
  static void InvalidINT_SetZF() {
  __asm volatile(R"(
  mov eax, 1
  dec eax
  int3;
  ret;
  )");
  }

__attribute__((naked))
  static void InvalidINT_ClearZF() {
  __asm volatile(R"(
  mov eax, 2
  dec eax
  int3;
  ret;
  )");
  }

__attribute__((naked))
  static void InvalidINT_SetSF() {
  __asm volatile(R"(
  mov eax, 0
  dec eax
  int3;
  ret;
  )");
  }

__attribute__((naked))
  static void InvalidINT_ClearSF() {
  __asm volatile(R"(
  mov eax, 1
  dec eax
  int3;
  ret;
  )");
  }

__attribute__((naked))
  static void InvalidINT_SetOF() {
  __asm volatile(R"(
  mov eax, 0x7fffffff
  inc eax
  int3;
  ret;
  )");
  }

__attribute__((naked))
  static void InvalidINT_ClearOF() {
  __asm volatile(R"(
  mov eax, 0
  inc eax
  int3;
  ret;
  )");
  }

constexpr int EXPECTED_TRAPNO = 3;
constexpr int EXPECTED_ERR = 0;
constexpr int EXPECTED_SI_CODE = 128;
constexpr int EXPECTED_SIGNAL = SIGTRAP;

constexpr uint32_t EFL_CF = 0;
constexpr uint32_t EFL_PF = 2;
constexpr uint32_t EFL_ZF = 6;
constexpr uint32_t EFL_SF = 7;
constexpr uint32_t EFL_OF = 11;

#ifndef REG_RIP
#define REG_RIP REG_EIP
#endif

using FunctionPtr = void(*)();
void SetupAndCallTest(FunctionPtr Func, uint32_t FlagOffset, uint32_t ExpectedFlag) {
  capturing_handler_skip = 0;
  struct sigaction act{};
  act.sa_sigaction = CapturingHandler;
  act.sa_flags = SA_SIGINFO;
  sigaction(SIGSEGV, &act, nullptr);
  sigaction(SIGTRAP, &act, nullptr);
  sigaction(SIGILL, &act, nullptr);

  Func();

  REQUIRE(from_handler.has_value());
  CHECK(from_handler->mctx.gregs[REG_TRAPNO] == EXPECTED_TRAPNO);
  CHECK(from_handler->mctx.gregs[REG_ERR] == EXPECTED_ERR);
  CHECK(from_handler->si_code == EXPECTED_SI_CODE);
  CHECK(from_handler->signal == EXPECTED_SIGNAL);
  // Extract Flag
  CHECK(((from_handler->mctx.gregs[REG_EFL] >> FlagOffset) & 1) == ExpectedFlag);
}

TEST_CASE("Signals: PF on Signal") {
  SetupAndCallTest(InvalidINT_SetPF, EFL_PF, 1);
}

TEST_CASE("Signals: NoPF on Signal") {
  SetupAndCallTest(InvalidINT_ClearPF, EFL_PF, 0);
}

TEST_CASE("Signals: CF on Signal") {
  SetupAndCallTest(InvalidINT_SetCF, EFL_CF, 1);
}

TEST_CASE("Signals: NoCF on Signal") {
  SetupAndCallTest(InvalidINT_ClearCF, EFL_CF, 0);
}

TEST_CASE("Signals: ZF on Signal") {
  SetupAndCallTest(InvalidINT_SetZF, EFL_ZF, 1);
}

TEST_CASE("Signals: NoZF on Signal") {
  SetupAndCallTest(InvalidINT_ClearZF, EFL_ZF, 0);
}

TEST_CASE("Signals: SF on Signal") {
  SetupAndCallTest(InvalidINT_SetSF, EFL_SF, 1);
}

TEST_CASE("Signals: NoSF on Signal") {
  SetupAndCallTest(InvalidINT_ClearSF, EFL_SF, 0);
}

TEST_CASE("Signals: OF on Signal") {
  SetupAndCallTest(InvalidINT_SetOF, EFL_OF, 1);
}

TEST_CASE("Signals: NoOF on Signal") {
  SetupAndCallTest(InvalidINT_ClearOF, EFL_OF, 0);
}
