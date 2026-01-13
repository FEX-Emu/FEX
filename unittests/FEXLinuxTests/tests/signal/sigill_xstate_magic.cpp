#include "fpstate.h"
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <signal.h>
#include <sys/mman.h>
#include <stdint.h>
#include <cstdlib>
#include <optional>

#pragma GCC diagnostic ignored "-Wattributes" // Suppress warning in case control-flow checks aren't enabled
__attribute__((naked, nocf_check)) static void InvalidINT() {
  __asm volatile(R"(
    hlt;
    ret;
    )");
}

__attribute__((naked, nocf_check)) static void SafeRet() {
  __asm volatile(R"(
  ret;
  )");
}

struct capture_data {
  uint32_t magic1;
  uint32_t magic2;
};

std::optional<capture_data> data;
static void signal_check(int signal, siginfo_t* siginfo, void* context) {
  ucontext_t* _context = (ucontext_t*)context;
  auto mcontext = &_context->uc_mcontext;

#ifdef REG_RIP
#define FEX_IP_REG REG_RIP
#else
#define FEX_IP_REG REG_EIP
#endif
  auto xstate = reinterpret_cast<FEX::Unittests::xstate*>(_context->uc_mcontext.fpregs);
  auto magic1 = xstate->fpstate.sw_reserved.magic1;
  uint32_t magic2 {};
  if (magic1 == FEX::Unittests::fpx_sw_bytes::FP_XSTATE_MAGIC_1) {
    auto magic2_addr =
      reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(xstate) + xstate->fpstate.sw_reserved.extended_size - sizeof(uint32_t));
    magic2 = *magic2_addr;
  }

  data.emplace(capture_data {
    .magic1 = magic1,
    .magic2 = magic2,
  });

  // Change RIP to a safe return so we can continue testing.
  mcontext->gregs[FEX_IP_REG] = reinterpret_cast<greg_t>(&SafeRet);
}

TEST_CASE("Signals: SIGILL flags") {
  struct sigaction act {};
  act.sa_sigaction = signal_check;
  act.sa_flags = SA_SIGINFO;
  sigaction(SIGSEGV, &act, nullptr);
  sigaction(SIGTRAP, &act, nullptr);
  sigaction(SIGILL, &act, nullptr);

  InvalidINT();

  REQUIRE(data.has_value());
  CHECK(data->magic1 == FEX::Unittests::fpx_sw_bytes::FP_XSTATE_MAGIC_1);
  CHECK(data->magic2 == FEX::Unittests::fpx_sw_bytes::FP_XSTATE_MAGIC_2);
}
