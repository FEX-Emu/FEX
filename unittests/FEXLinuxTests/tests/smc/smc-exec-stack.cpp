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

TEST_CASE("smc-exec-stack: PT_GNU_STACK == RWX") {
  // Register signal handler
  struct sigaction act {};
  act.sa_sigaction = sigsegv_handler;
  act.sa_flags = SA_SIGINFO;
  sigaction(SIGSEGV, &act, nullptr);
  
  // Try executing from stack
  uint8_t stack_code = 0xC3; // ret
  ((void (*)())(&stack_code))();
  REQUIRE(got_signal == false);

  // Executing from other memory should fail
  size_t page_size = sysconf(_SC_PAGESIZE);
  uint8_t *mem_code = static_cast<uint8_t *>(mmap(NULL, page_size, PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
  REQUIRE(mem_code != nullptr);
  *mem_code = 0xC3; // ret
  ((void (*)())(mem_code))();
  REQUIRE(got_signal == true);

  munmap(mem_code, page_size);
}
