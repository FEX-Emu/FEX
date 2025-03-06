// SPDX-License-Identifier: MIT
#include <catch2/catch_test_macros.hpp>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <ucontext.h>
#include <unistd.h>

int Page {};
void* signal_page {};
void* signal_second_page {};

void* backwards_page {};
void* backwards_second_page {};

__attribute__((naked)) void backwards_set(void* dest, uint8_t value, uint32_t pad, size_t size) {
  __asm volatile(R"(
    std;
    jmp 1f;
    1:
    mov rax, rsi;
    rep stosb;
    jmp 2f;
    2:
    cld;
    ret;
  )" ::
                   : "memory", "cc");
}

__attribute__((naked)) void forward_set(void* dest, uint8_t value, uint32_t pad, size_t size) {
  __asm volatile(R"(
    mov rax, rsi;
    rep stosb;
    ret;
  )" ::
                   : "memory", "cc");
}

void sig_handler(int signum, siginfo_t* info, void* context) {
  // Ensure the fault address isn't in the signal page.
  auto addr = reinterpret_cast<uint64_t>(info->si_addr);
  // This REQUIRE will fail if DF isn't reset on signal handler.
  REQUIRE(!(addr >= (uint64_t)signal_page && addr <= (uint64_t)signal_second_page));
  forward_set(signal_second_page, 1, 0, 4096);

  // mprotect the page that was originally written to, allowing the code to continue.
  REQUIRE(mprotect(backwards_page, Page, PROT_READ | PROT_WRITE) == 0);
}

TEST_CASE("DF flaga reset on signal") {
  struct sigaction act {};

  act.sa_flags = SA_SIGINFO;
  act.sa_sigaction = &sig_handler;
  REQUIRE(sigaction(SIGSEGV, &act, NULL) == 0);

  Page = sysconf(_SC_PAGESIZE);

  // Allocate pages with protections to ensure correct direction.
  signal_page = ::mmap(nullptr, Page * 3, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  REQUIRE(signal_page != MAP_FAILED);
  signal_second_page = reinterpret_cast<void*>(reinterpret_cast<uint64_t>(signal_page) + Page);

  // Again for backward direction.
  backwards_page = ::mmap(nullptr, Page * 3, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  REQUIRE(backwards_page != MAP_FAILED);
  backwards_second_page = reinterpret_cast<void*>(reinterpret_cast<uint64_t>(backwards_page) + Page);

  // Allow the middle page to read/write.
  REQUIRE(mprotect(signal_second_page, Page, PROT_READ | PROT_WRITE) == 0);

  // Allow the middle page to read/write.
  REQUIRE(mprotect(backwards_second_page, Page, PROT_READ | PROT_WRITE) == 0);

  // Copy backwards and cause a signal.
  backwards_set(backwards_second_page, 1, 0, 4096);
}
