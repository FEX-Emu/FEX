#include <catch2/catch_test_macros.hpp>
#include <signal.h>
#include <ucontext.h>
#include <sys/mman.h>

bool got_signal = false;

static void sigsegv_handler(int signal, siginfo_t* siginfo, void* context) {
  REQUIRE(siginfo->si_code == SEGV_ACCERR);
  got_signal = true;
  size_t page_size = sysconf(_SC_PAGESIZE);
  void* fault_page = (void*)((uintptr_t)(siginfo->si_addr) & ~(page_size - 1));
  REQUIRE(mprotect(fault_page, page_size, PROT_READ | PROT_WRITE | PROT_EXEC) == 0);
}

void register_signal_handler() {
  struct sigaction act {};
  act.sa_sigaction = sigsegv_handler;
  act.sa_flags = SA_SIGINFO;
  REQUIRE(sigaction(SIGSEGV, &act, nullptr) == 0);
}

TEST_CASE("smc-unexec-stack: PT_GNU_STACK == RW") {
  register_signal_handler();

  // Try executing from stack
  char stack[16384];
  auto stack_code = (char*)(((uintptr_t)stack + 4095) & ~4095);
  *stack_code = 0xC3; // ret instruction
  ((void (*)())(stack_code))();

  CHECK(got_signal == true);
}
