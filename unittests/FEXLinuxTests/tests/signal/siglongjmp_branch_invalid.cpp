// If guest code registers a signal handler that does not sigreturn,
// then deadlocks internal to FEX are possible if the guest code jumps to an invalid address.
// This test checks that FEX does not deadlock in this case.

#include <catch2/catch_test_macros.hpp>

#include <csetjmp>
#include <cstdint>

#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>

static sigjmp_buf FaultReturn;

static void Handler(int, siginfo_t*, void*) {
  siglongjmp(FaultReturn, 1);
}

TEST_CASE("Signals: siglongjmp from signal handler after branch to invalid address") {
  struct sigaction Act {};
  Act.sa_sigaction = Handler;
  Act.sa_flags = SA_SIGINFO | SA_NODEFER;
  sigemptyset(&Act.sa_mask);

  REQUIRE(sigaction(SIGSEGV, &Act, nullptr) == 0);
  REQUIRE(sigaction(SIGBUS, &Act, nullptr) == 0);
  REQUIRE(sigaction(SIGILL, &Act, nullptr) == 0);

  const int64_t PageSize = sysconf(_SC_PAGESIZE);
  REQUIRE(PageSize > 0);

  // Branch to a range of addresses around 0.
  int64_t Faults = 0;
  const int64_t Range = 2 * PageSize;

  for (int64_t Offset = -Range; Offset <= Range; ++Offset) {
    if (sigsetjmp(FaultReturn, 1) == 0) {
      reinterpret_cast<void (*)()>(static_cast<uintptr_t>(Offset))();
      FAIL("branch to invalid address did not fault");
    } else {
      Faults++;
    }
  }

  CHECK(Faults == 2 * Range + 1);

  // Force code invalidation so that we take the lock
  void* Code = mmap(nullptr, PageSize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  REQUIRE(Code != MAP_FAILED);
  REQUIRE(munmap(Code, PageSize) == 0);

  SUCCEED();
}
