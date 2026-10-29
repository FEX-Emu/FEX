#include <catch2/catch_test_macros.hpp>

#include <csetjmp>
#include <signal.h>
#include <stdint.h>

// A compare against memory whose flags are never read still has to perform
// the load, because the load can fault. HotSpot's C1 relies on this for its
// explicit null checks: `cmp rax, [reg]` with the flags dead, and the SIGSEGV
// at that address becomes the NullPointerException. FEX used to remove the
// load together with the dead flag calculation, so the null receiver went on
// to fault at a later instruction instead.

static sigjmp_buf JumpBuffer;
static volatile int Faulted;

static void SIGSEGVHandler(int, siginfo_t*, void*) {
  Faulted = 1;
  siglongjmp(JumpBuffer, 1);
}

TEST_CASE("Signals: load with dead flags still faults") {
  struct sigaction act {};
  act.sa_sigaction = SIGSEGVHandler;
  act.sa_flags = SA_SIGINFO;
  sigaction(SIGSEGV, &act, nullptr);

  uint32_t Counter = 0;
  uintptr_t Null = 0;
  Faulted = 0;

  if (sigsetjmp(JumpBuffer, 1) == 0) {
    // The add overwrites every flag the compare produced, so only the
    // fault from the load is observable.
    __asm volatile("cmp %[Null], [%[Null]];"
                   "add dword ptr [%[Counter]], 1;"
                   :
                   : [Null] "r"(Null), [Counter] "r"(&Counter)
                   : "cc", "memory");
  }

  CHECK(Faulted == 1);
  CHECK(Counter == 0);
}
