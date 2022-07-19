#include <atomic>
#include <signal.h>
#include <sys/mman.h>
#include <stdint.h>
#include <stdio.h>
#include <cstdlib>

__attribute__((naked, nocf_check))
  static void InvalidINT() {
  __asm volatile(R"(
    hlt;
    ret; # Just incase it gets past the int
    )");
  }

unsigned long EXPECTED_RIP = reinterpret_cast<unsigned long>(&InvalidINT);
constexpr int EXPECTED_TRAPNO = 0xD;
constexpr int EXPECTED_ERR = 0;
constexpr int EXPECTED_SI_CODE = 128;
constexpr int EXPECTED_SIGNAL = SIGSEGV;

static void handler(int signal, siginfo_t *siginfo, void* context) {
  ucontext_t* _context = (ucontext_t*)context;
#ifndef REG_RIP
#define REG_RIP REG_EIP
#endif
  if (_context->uc_mcontext.gregs[REG_RIP] == EXPECTED_RIP &&
      _context->uc_mcontext.gregs[REG_TRAPNO] == EXPECTED_TRAPNO &&
      _context->uc_mcontext.gregs[REG_ERR] == EXPECTED_ERR &&
      siginfo->si_code == EXPECTED_SI_CODE &&
      signal == EXPECTED_SIGNAL) {
    exit(0);
  }
  else {
    exit(1);
  }
}

int main() {
  struct sigaction act{};
  act.sa_sigaction = handler;
  act.sa_flags = SA_SIGINFO;
  sigaction(SIGSEGV, &act, nullptr);
  sigaction(SIGTRAP, &act, nullptr);
  sigaction(SIGILL, &act, nullptr);

  InvalidINT();
  return 1;
}
