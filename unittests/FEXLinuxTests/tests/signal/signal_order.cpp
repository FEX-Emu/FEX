#include <catch2/catch_test_macros.hpp>
#include <unistd.h>
#include <signal.h>
#include <sys/syscall.h>
#include <stdio.h>

constexpr uint64_t ExpectedOrder[64] = {
  0,  1,  2,  3,  4,  5,  6,  7,  0,  8,  9,  10, 11, 12, 13, 14, 15, 16, 0,  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
  30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
};

uint64_t Count {};
uint64_t Order[64] = {};

#ifndef __x86_64__
[[gnu::regparm(3)]]
#endif
static void handler(int signal, siginfo_t* siginfo, void* context) {
  REQUIRE((signal > 0 && signal < 65));
  if (signal < 1 || signal > 64) {
    return;
  }
  Order[signal - 1] = Count;
  ++Count;
}

#ifdef __x86_64__
__attribute__((naked)) void asm_handler(int signal, siginfo_t* siginfo, void* context) {
  __asm volatile(R"(
  call %[Handler];
  ret;
  )" ::[Handler] "r"(handler)
                 : "memory");
}

__attribute__((naked)) void restorer() {
  __asm volatile(R"(
  mov eax, %[sigreturn];
  syscall;
  )" ::[sigreturn] "i"(SYS_rt_sigreturn)
                 : "memory");
}
#else
__attribute__((naked)) void asm_handler(int signal, siginfo_t* siginfo, void* context) {
  __asm volatile(R"(
  mov ebx, %[Handler];
  mov eax, [esp + 4];
  mov ecx, [esp + 8];
  mov edx, [esp + 12];
  call ebx;
  ret;
  )" ::[Handler] "r"(handler)
                 : "eax", "ecx", "edx", "memory");
}


__attribute__((naked)) void restorer() {
  __asm volatile(R"(
  mov eax, %[sigreturn];
  int 0x80;
  )" ::[sigreturn] "i"(SYS_rt_sigreturn)
                 : "memory");
}
#endif


struct __attribute__((packed)) GuestSAMask {
  uint64_t Val;
};

struct __attribute__((packed)) GuestSigAction {
  union {
    void (*handler)(int);
    void (*sigaction)(int, siginfo_t*, void*);
  } sigaction_handler;

  size_t sa_flags;
  void (*restorer)(void);
  GuestSAMask sa_mask;
};

TEST_CASE("signal order") {

#define SA_RESTORER 0x04000000
  struct GuestSigAction act {};
  act.sigaction_handler.sigaction = (decltype(act.sigaction_handler.sigaction))asm_handler;
  act.restorer = restorer;
  act.sa_flags = SA_SIGINFO | SA_RESTORER;
  for (size_t i = 1; i <= 64; ++i) {
    ::syscall(SYS_rt_sigaction, i, &act, nullptr, 8);
  }

  auto pid = ::getpid();
  auto tid = ::gettid();
  for (size_t i = 1; i <= 64; ++i) {
    if (i == SIGKILL || i == SIGSTOP) {
      continue;
    }
    tgkill(pid, tid, i);
  }

  for (size_t i = 1; i <= 64; ++i) {
    CHECK(Order[i - 1] == ExpectedOrder[i - 1]);
  }
}
