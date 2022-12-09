// Triggering synchronous POSIX signals while they're masked triggers a
// process exit. In contrast, if an asynchronous signals is triggered, the
// corresponding signal handler will be invoked once the signal is unmasked.
//
// To test synchronous signals, the test forks and triggers the signal in the
// child process. For asynchronous signals, a signal handler that sets a global
// variable is used.

#include <sys/mman.h>
#include <sys/wait.h>

#include <cinttypes>
#include <cstdint>
#include <optional>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#include <catch2/catch.hpp>

static jmp_buf jmpbuf;

struct HandledSignal {
  int signal;
  uintptr_t addr;
};

struct HandledSignal_ERR {
  int signal;
  size_t ERR;
};

static std::optional<HandledSignal> handled_signal;
static std::optional<HandledSignal_ERR> handled_signal_err;

static void handler(int sig, siginfo_t *si, void *context) {
  printf("Got %d at address: 0x%lx\n", sig, (long)si->si_addr);
  handled_signal = { sig, reinterpret_cast<uintptr_t>(si->si_addr) };
  siglongjmp(jmpbuf, 1);
}

// Helper that masks all signals and unmasks them on destruction
struct GuardedSignalMask {
  sigset_t oldset {};

  GuardedSignalMask() {
    sigset_t set;
    sigfillset(&set);
    sigprocmask(SIG_SETMASK, &set, &oldset);
  }

  ~GuardedSignalMask() {
    sigprocmask(SIG_SETMASK, &oldset, nullptr);
  }
};

// Checks if the given function causes the process to exit.
// The function is executed in a process fork.
template<typename F>
std::optional<int> CheckIfExitsFromSignal(F&& f) {
  if (fork() == 0) {
    GuardedSignalMask guard;
    std::forward<F>(f)();
    exit(1);
  } else {
    int status = 0;
    wait(&status);
    return status;
  }
}

// Checks if the given function causes a signal handler to be invoked
template<typename F>
std::optional<HandledSignal> CheckIfSignalHandlerCalled(F&& f) {
  handled_signal = {};
  struct sigaction oldsa[4];

  if (!sigsetjmp(jmpbuf, 1)) {
    // Handle all signals by the test handler
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = handler;
    sigaction(SIGSEGV, &sa, &oldsa[0]);
    sigaction(SIGBUS, &sa, &oldsa[1]);
    sigaction(SIGILL, &sa, &oldsa[2]);
    sigaction(SIGFPE, &sa, &oldsa[3]);

    // Mask signals and run given callback
    GuardedSignalMask guard;
    std::forward<F>(f)();
  }

  // Restore previous signal handlers
  sigaction(SIGSEGV, &oldsa[0], nullptr);
  sigaction(SIGBUS, &oldsa[1], nullptr);
  sigaction(SIGILL, &oldsa[2], nullptr);
  sigaction(SIGFPE, &oldsa[3], nullptr);

  return handled_signal;
}

static void handler_read(int sig, siginfo_t *si, void *context) {
  ucontext_t* _context = (ucontext_t*)context;
  auto mcontext = &_context->uc_mcontext;
  printf("Got %d at address: 0x%lx with 0x%zx\n", sig, (long)si->si_addr, (size_t)mcontext->gregs[REG_ERR]);
  handled_signal_err = { sig, (size_t)mcontext->gregs[REG_ERR] };
  siglongjmp(jmpbuf, 1);
}

template<typename F>
std::optional<HandledSignal> CheckIfSignalHandlerCalledWithRegERR(F&& f) {
  handled_signal = {};
  struct sigaction oldsa[4];

  if (!sigsetjmp(jmpbuf, 1)) {
    // Handle all signals by the test handler
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = handler_read;
    sigaction(SIGSEGV, &sa, &oldsa[0]);
    sigaction(SIGBUS, &sa, &oldsa[1]);
    sigaction(SIGILL, &sa, &oldsa[2]);
    sigaction(SIGFPE, &sa, &oldsa[3]);

    // Mask signals and run given callback
    std::forward<F>(f)();
  }

  // Restore previous signal handlers
  sigaction(SIGSEGV, &oldsa[0], nullptr);
  sigaction(SIGBUS, &oldsa[1], nullptr);
  sigaction(SIGILL, &oldsa[2], nullptr);
  sigaction(SIGFPE, &oldsa[3], nullptr);

  return handled_signal;
}

TEST_CASE("Signals: Error Flag - Read") {
  // Check that the signal handler is delayed until unmasking.
  auto handled_signal = CheckIfSignalHandlerCalledWithRegERR([&]() {
    uint8_t *Code = (uint8_t*)mmap(nullptr, 4096, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    printf("Read: %d\n", Code[0]);
  });
  REQUIRE(handled_signal_err.has_value());
  CHECK(handled_signal_err->signal == SIGSEGV);
  constexpr size_t Expected = 0x4;
  CHECK(handled_signal_err->ERR == Expected); // USER
}

TEST_CASE("Signals: Error Flag - Write") {
  // Check that the signal handler is delayed until unmasking.
  auto handled_signal = CheckIfSignalHandlerCalledWithRegERR([&]() {
    uint8_t *Code = (uint8_t*)mmap(nullptr, 4096, PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    Code[0] = 1;
  });
  REQUIRE(handled_signal_err.has_value());
  CHECK(handled_signal_err->signal == SIGSEGV);
  constexpr size_t Expected = 0x6;
  CHECK(handled_signal_err->ERR == Expected); // USER + WRITE
}

// For ssegv, we fail to do default signal catching behaviour
TEST_CASE("Signals: ssegv") {
  auto status = CheckIfExitsFromSignal([]() { *(int*)0x32 = 0x64; });
  REQUIRE(status.has_value());
  CHECK(WIFSIGNALED(*status) == true);
  CHECK(WTERMSIG(*status) == SIGSEGV);
}

// For sill, we fail to do default signal catching behaviour
TEST_CASE("Signals: sill") {
  auto status = CheckIfExitsFromSignal([]() { asm volatile("ud2\n"); });
  REQUIRE(status.has_value());
  CHECK(WIFSIGNALED(*status) == true);
  CHECK(WTERMSIG(*status) == SIGILL);
}

// sbus and abus fail on arm because of sigbus handling
TEST_CASE("Signals: sbus") {
  auto map1 = mmap(nullptr, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
  auto map2 = (char *)mremap(map1, 4096, 8192, MREMAP_MAYMOVE);

  auto status = CheckIfExitsFromSignal([&]() { map2[4096] = 2; });
  REQUIRE(status.has_value());
  CHECK(WIFSIGNALED(*status) == true);
  CHECK(WTERMSIG(*status) == SIGBUS);
}

// sfpe and afpe fail on arm because we don't raise FPE
TEST_CASE("Signals: sfpe") {
  auto status = CheckIfExitsFromSignal([&]() {
    volatile int a = 10;
    volatile int b = 0;
    volatile int c = a / b;
    printf("result: %d\n", c);
  });
  REQUIRE(status.has_value());
  CHECK(WIFSIGNALED(*status) == true);
  CHECK(WTERMSIG(*status) == SIGFPE);
}

// These fail to queue the signals
TEST_CASE("Signals: asynchronous") {
  int tested_signal = GENERATE(SIGSEGV, SIGILL, SIGBUS, SIGFPE);

  // Check that the signal handler is delayed until unmasking.
  bool handled_asynchronously = false;
  auto handled_signal = CheckIfSignalHandlerCalled([&]() {
    GuardedSignalMask guard{};
    raise(tested_signal);

    // Verify the rest of this function is still executed
    handled_asynchronously = true;

    // Destructor of GuardedSignalMask will unmask signals now,
    // after which the signal handler should run
  });
  REQUIRE(handled_signal.has_value());
  CHECK(handled_signal->signal == tested_signal);
  CHECK(handled_asynchronously);
}
