#include <catch2/catch_test_macros.hpp>
#include <signal.h>
#include <ucontext.h>
#include <sys/mman.h>

bool got_signal = false;

static void sigsegv_handler(int signal, siginfo_t *siginfo, void* context) {
  REQUIRE(siginfo->si_code == SEGV_ACCERR);
  got_signal = true;
  size_t page_size = sysconf(_SC_PAGESIZE);
  void *fault_addr = (void *)((uintptr_t)(siginfo->si_addr) & ~(page_size - 1));
  REQUIRE(mprotect(fault_addr, page_size, PROT_READ | PROT_WRITE | PROT_EXEC) == 0);
}

TEST_CASE("smc-missing-gnustack: PT_GNU_STACK missing") {
  // Register signal handler
  struct sigaction act {};
  act.sa_sigaction = sigsegv_handler;
  act.sa_flags = SA_SIGINFO;
  sigaction(SIGSEGV, &act, nullptr);

  // Try executing from stack
  uint8_t stack_code = 0xC3; // ret
  ((void (*)())(&stack_code))();

#ifdef __i386__
  REQUIRE(got_signal == false);
#else
  REQUIRE(got_signal == true);
  got_signal = false;
#endif

  // Executing from other memory should fail on 64 bit but work on 32 bit
  size_t page_size = sysconf(_SC_PAGESIZE);
  uint8_t *mem_code = static_cast<uint8_t *>(mmap(NULL, page_size, PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
  REQUIRE(mem_code != nullptr);
  *mem_code = 0xC3; // ret
  ((void (*)())(mem_code))();

#ifdef __i386__
  REQUIRE(got_signal == false);
#else
  REQUIRE(got_signal == true);
#endif

  munmap(mem_code, page_size);
}
