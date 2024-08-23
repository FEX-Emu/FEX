// A test where a signal is masked, a value is set that the signal handler will overwrite, and then the signal is unmasked to allow it to
// fire. FEX-Emu had a bug where it wasn't properly deferring signals from sigprocmask if one of the signals was masked by the guest application.
// In older glibc versions (glibc-2.26), the `raise` implementation would block signals, tgkill, and then unblock signals,
// expecting the signal to fire once the signals were unblocked.

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include <signal.h>
#include <unistd.h>

static uint32_t CheckValue {};
void sig_handler(int signum, siginfo_t* info, void* context) {
  REQUIRE(CheckValue == 2);
  CheckValue = 0x1;
}

static void RaiseSignal(int Signal) {
  sigset_t Prev {};
  sigset_t New {};

  // Mask all signals
  sigfillset(&New);
  int Ret = sigprocmask(SIG_BLOCK, &New, &Prev);

  REQUIRE(Ret != -1);

  // Try to raise the signal, even though it is blocked
  Ret = tgkill(::getpid(), ::gettid(), Signal);
  REQUIRE(Ret != -1);

  // Set the check value
  CHECK(CheckValue == 0);
  CheckValue = 0x2;

  // Unmask the signal
  Ret = sigprocmask(SIG_SETMASK, &Prev, nullptr);
  REQUIRE(Ret != -1);
}

TEST_CASE("Signals: Defer Signals") {
  auto tested_signal = GENERATE(range(1, 65));

  if (tested_signal != SIGKILL && tested_signal != SIGSTOP && tested_signal != 32 && tested_signal != 33) {
    struct sigaction sa {};
    sa.sa_sigaction = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(tested_signal, &sa, nullptr);
    CheckValue = 0;
    RaiseSignal(tested_signal);
    CHECK(CheckValue == 1);
  }
}
