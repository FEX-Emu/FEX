#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <signal.h>

extern "C" void IntInstruction();

#pragma GCC diagnostic ignored "-Wattributes" // Suppress warning in case control-flow checks aren't enabled
__attribute__((naked, nocf_check)) static void InvalidINT() {
  __asm volatile(R"(
    IntInstruction:
    hlt;
    ret;
    )");
}

__attribute__((noinline)) uint32_t GetMXCSR() {
  uint32_t Result {};
  asm volatile("stmxcsr %[Result]" : [Result] "=m"(Result)::"memory");

  return Result;
}

void SetMXCSR(uint32_t MXCSR) {
  asm volatile("ldmxcsr %[MXCSR]" ::[MXCSR] "m"(MXCSR) : "memory");
}

static uint32_t MXCSRFromContext {};
static void MXCSR_Handler(int signal, siginfo_t* siginfo, void* context) {
  ucontext_t* _context = (ucontext_t*)context;

  // Enable DAZ. This will get masked off on signal return.
  SetMXCSR(0x1fc0);

#ifdef REG_RIP
#define FEX_IP_REG REG_RIP
#else
#define FEX_IP_REG REG_EIP
#endif
  _context->uc_mcontext.gregs[FEX_IP_REG] += 1;

  // Save the MXCSR from the context.
  MXCSRFromContext = _context->uc_mcontext.fpregs->mxcsr;
#undef FEX_IP_REG
}

TEST_CASE("Signals: Restoring MXCSR") {
  struct sigaction act {};
  act.sa_sigaction = MXCSR_Handler;
  act.sa_flags = SA_SIGINFO;
  sigaction(SIGSEGV, &act, nullptr);

  // Get the old MXCSR mask.
  auto MXCSROld = GetMXCSR();

  InvalidINT();

  // Get the potentially broken MXCSR mask.
  auto MXCSRNew = GetMXCSR();

  // Ensure that all three masks match.
  CHECK(MXCSRFromContext == MXCSROld);
  CHECK(MXCSROld == MXCSRNew);
  printf("0x%08x 0x%08x\n", MXCSROld, MXCSRNew);
}
