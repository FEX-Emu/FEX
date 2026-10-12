#include <catch2/catch_test_macros.hpp>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <thread>

void sig_handler(int signum, siginfo_t* info, void* context) {
  printf("Inside handler function\n");
}

TEST_CASE("Signals: Check waitpid works") {
  int pid = ::fork();
  if (pid == 0) {
    _exit(0);
  }

  int status {};
  auto res = ::waitpid(pid, &status, 0);
  REQUIRE(res != -1);
  CHECK(WIFEXITED(status));
  CHECK(WEXITSTATUS(status) == 0);
}

TEST_CASE("Signals: Check terminate works.") {
  int pid = ::fork();
  if (pid == 0) {
    // Reset Catch2's signal handler for this fork.
    struct sigaction act = {};
    act.sa_flags = 0;
    act.sa_handler = SIG_DFL;
    REQUIRE(sigaction(SIGTERM, &act, nullptr) == 0);

    ::tgkill(::getpid(), ::gettid(), SIGTERM);
  }

  int status {};
  auto res = ::waitpid(pid, &status, 0);
  REQUIRE(res != -1);
  CHECK(!WIFEXITED(status));
  CHECK(WIFSIGNALED(status));
  CHECK(WTERMSIG(status) == SIGTERM);
}

TEST_CASE("Signals: Check flags.") {
  constexpr static uint32_t SIGNAL_FLAGS = SA_SIGINFO | SA_NODEFER;

  int pid = ::fork();
  if (pid == 0) {
    static struct sigaction act = {};
    auto sig_handler = [](int signum, siginfo_t* info, void* context) -> void {
      REQUIRE(sigaction(SIGTERM, nullptr, &act) == 0);
    };

    // Reset Catch2's signal handler for this fork.
    act.sa_flags = SIGNAL_FLAGS;
    act.sa_sigaction = sig_handler;
    REQUIRE(sigaction(SIGTERM, &act, nullptr) == 0);

    ::tgkill(::getpid(), ::gettid(), SIGTERM);

#define SA_RESTORER 0x04000000
    if ((act.sa_flags & ~SA_RESTORER) != SIGNAL_FLAGS) {
      _exit(1);
    }

    if (act.sa_sigaction != sig_handler) {
      _exit(1);
    }

    _exit(0);
  }

  int status {};
  auto res = ::waitpid(pid, &status, 0);
  REQUIRE(res != -1);
  CHECK(WIFEXITED(status));
  CHECK(WEXITSTATUS(status) == 0);
}

TEST_CASE("Signals: Check flags reset handler.") {
  constexpr static uint32_t SIGNAL_FLAGS = SA_SIGINFO | SA_NODEFER | SA_RESETHAND;

  int pid = ::fork();
  if (pid == 0) {
    static struct sigaction act = {};
    auto sig_handler = [](int signum, siginfo_t* info, void* context) -> void {
      REQUIRE(sigaction(SIGTERM, nullptr, &act) == 0);
    };

    // Reset Catch2's signal handler for this fork.
    act.sa_flags = SIGNAL_FLAGS;
    act.sa_sigaction = sig_handler;
    REQUIRE(sigaction(SIGTERM, &act, nullptr) == 0);

    ::tgkill(::getpid(), ::gettid(), SIGTERM);

#define SA_RESTORER 0x04000000
    // With SA_RESETHAND, sa_handler resets to SIG_DFL but everything else stays the same.
    REQUIRE((act.sa_flags & ~SA_RESTORER) == SIGNAL_FLAGS);
    REQUIRE(act.sa_handler == SIG_DFL);
    _exit(0);
  }

  int status {};
  auto res = ::waitpid(pid, &status, 0);
  REQUIRE(res != -1);
  CHECK(WIFEXITED(status));
  CHECK(WEXITSTATUS(status) == 0);
}

TEST_CASE("Signals: Check flags reset handler ignore.") {
  // SIGWINCH by default is type SIG_DFL but that means SIG_IGN in the kernel.
  // Ensure SA_RESETHAND resets it to SIG_DFL, not SIG_IGN.
  constexpr static uint32_t SIGNAL_FLAGS = SA_SIGINFO | SA_NODEFER | SA_RESETHAND;

  int pid = ::fork();
  if (pid == 0) {
    static struct sigaction act = {};
    auto sig_handler = [](int signum, siginfo_t* info, void* context) -> void {
      REQUIRE(sigaction(SIGWINCH, nullptr, &act) == 0);
    };

    // Reset Catch2's signal handler for this fork.
    act.sa_flags = SIGNAL_FLAGS;
    act.sa_sigaction = sig_handler;
    REQUIRE(sigaction(SIGWINCH, &act, nullptr) == 0);

    ::tgkill(::getpid(), ::gettid(), SIGWINCH);

#define SA_RESTORER 0x04000000
    // With SA_RESETHAND, sa_handler resets to SIG_DFL but everything else stays the same.
    REQUIRE((act.sa_flags & ~SA_RESTORER) == SIGNAL_FLAGS);
    REQUIRE(act.sa_handler == SIG_DFL);
    _exit(0);
  }

  int status {};
  auto res = ::waitpid(pid, &status, 0);
  REQUIRE(res != -1);
  CHECK(WIFEXITED(status));
  CHECK(WEXITSTATUS(status) == 0);
}

TEST_CASE("Signals: Check flags reset handler thread.") {
  constexpr static uint32_t SIGNAL_FLAGS = SA_SIGINFO | SA_NODEFER | SA_RESETHAND;

  static struct sigaction act = {};
  static struct sigaction orig_act = {};

  auto sig_handler = [](int signum, siginfo_t* info, void* context) -> void {
    REQUIRE(sigaction(SIGTERM, nullptr, &act) == 0);
  };

  // Reset Catch2's signal handler for this fork.
  act.sa_flags = SIGNAL_FLAGS;
  act.sa_sigaction = sig_handler;
  REQUIRE(sigaction(SIGTERM, &act, &orig_act) == 0);

  std::thread t([]() {
    ::tgkill(::getpid(), ::gettid(), SIGTERM);

#define SA_RESTORER 0x04000000
    // With SA_RESETHAND, sa_handler resets to SIG_DFL but everything else stays the same.
    REQUIRE((act.sa_flags & ~SA_RESTORER) == SIGNAL_FLAGS);
    REQUIRE(act.sa_handler == SIG_DFL);
  });

  // DFL switch should be visible from parent thread.
  static struct sigaction act_parent = {};
  while (sigaction(SIGTERM, nullptr, &act_parent) == 0) {
    if (act_parent.sa_handler == SIG_DFL) {
      fprintf(stderr, "Hit DFL!\n");
      break;
    }
  }

  t.join();

  // Set the original handler back for any remaining tests.
  REQUIRE(sigaction(SIGTERM, &orig_act, nullptr) == 0);
}
