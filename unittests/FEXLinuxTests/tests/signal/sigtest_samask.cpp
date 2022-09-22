#include <catch2/catch.hpp>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

volatile bool loop = true;
volatile bool last = false;
volatile int count = 0;
volatile int count2 = 0;

// OPTIONS
// TESTSIGPROCMASK

#define NUMCOUNT 10
#define SIGN SIGTSTP

void sig_handler(int signum) {
  loop = false;
  printf("Inside handler function\n");

  if (last) {
    printf("Handling last raise\n");
    return;
  }

  if (count2 != count) {
    printf("Signal reentering bug\n");
    exit(-1);
  }

  if (count < NUMCOUNT) {
    printf("Nested Raising sig%d, %d of %d times\n", signum, 1 + count, NUMCOUNT);
    count2++;
    raise(signum);
    printf("Nested raise correctly blocked, trying sigprocmask\n");
    sigset_t old;
    // test if sigmask returned by sigprocmask is the one currently active
    sigprocmask(0, 0, &old);
    sigprocmask(SIG_SETMASK, &old, 0);
    printf("sigprocmask worked correctly, should trigger next iteration on signal return\n");
    count++;
  }
}

TEST_CASE("Signals: samask") {
  REQUIRE(signal(SIGN, sig_handler) == 0);

  // test if sigmask blocks during execution as expected
  last = false;
  loop = true;
  while (loop) {
    printf("Inside main loop, raising signal\n");
    raise(SIGN);
    REQUIRE_FALSE(loop);
  }
  last = true;
  loop = true;

  // test if sigmask returned by sigprocmask is the one set by the signal return
  sigset_t old;
  sigprocmask(0, 0, &old);
  sigprocmask(SIG_SETMASK, &old, 0);

  while (loop) {
    printf("Inside last loop, raising signal\n");
    raise(SIGN);
    REQUIRE_FALSE(loop);
  }
}
