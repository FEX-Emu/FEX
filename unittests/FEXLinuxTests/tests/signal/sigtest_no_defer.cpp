#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

volatile bool loop = false;
volatile int count = 0;
volatile int count2 = 0;

#define NUMCOUNT 10
#define SIGN SIGTSTP

void sig_handler(int signum, siginfo_t *info, void *context) {
  loop = false;
  
  printf("Inside handler function\n");
  if (count != 0) {
    printf("SA_NODEFER bug\n");
    exit(-1);
  }

  if (count2 != 0) {
    printf("Nested raise correctly raised, trying sigprocmask\n");
    sigset_t old;
    // test if sigmask returned by sigprocmask is the one currently active
    sigprocmask(0, 0, &old);
    sigprocmask(SIG_SETMASK, &old, 0);
  }
  
  if (count2 < NUMCOUNT) {
    printf("Nested Raising %d, %d of %d times\n", signum, 1 + count, NUMCOUNT);
    count2++;
    raise(signum);
    count++;
  } else {
    exit(0);
    printf("Exiting\n");
  }
}

int main() {
  struct sigaction act = {0};

  act.sa_flags = SA_SIGINFO | SA_NODEFER;
  act.sa_sigaction = &sig_handler;
  if (sigaction(SIGN, &act, NULL) != 0) {
    printf("sigaction failed\n");
    return -3;
  }
  loop = true;
  while (loop) {
    printf("Inside main loop, raising signal\n");
    raise(SIGN);
    if (loop) {
      printf("Error: Signal did not get raised\n");
      return -3;
    }
  }
  return -2;
}
