#include <catch2/catch_test_macros.hpp>

#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>
#include <csetjmp>
#include <cstdint>

static jmp_buf LongJump {};
static volatile sig_atomic_t CaughtSignal {};

static void SignalHandler(int Signal, siginfo_t*, void*) {
  CaughtSignal = Signal;
  longjmp(LongJump, 1);
}

TEST_CASE("Signals: Instruction fetch faults take priority over decode errors") {
  struct sigaction Act {};
  Act.sa_sigaction = SignalHandler;
  Act.sa_flags = SA_SIGINFO;

  REQUIRE(sigaction(SIGILL, &Act, nullptr) == 0);
  REQUIRE(sigaction(SIGSEGV, &Act, nullptr) == 0);

  const auto PageSize = sysconf(_SC_PAGESIZE);
  REQUIRE(PageSize > 0);

  void* Ptr = mmap(nullptr, PageSize * 2, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  REQUIRE(Ptr != MAP_FAILED);

  auto Code = static_cast<uint8_t*>(Ptr) + PageSize - 2;
  // movzx r8d, byte [rax]
  Code[0] = 0x44;
  Code[1] = 0x0f;
  Code[2] = 0xb6;
  Code[3] = 0x00;
  Code[4] = 0xc3;

  REQUIRE(mprotect(static_cast<uint8_t*>(Ptr) + PageSize, PageSize, PROT_NONE) == 0);

  using FuncPtr = void (*)();
  auto Func = reinterpret_cast<FuncPtr>(Code);

  CaughtSignal = 0;
  if (setjmp(LongJump) == 0) {
    Func();
    FAIL("Cross-page instruction unexpectedly executed");
  }

  CHECK(CaughtSignal == SIGSEGV);
  REQUIRE(munmap(Ptr, PageSize * 2) == 0);
}
