#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <unistd.h>

volatile bool loop = false;
volatile bool inhandler = false;

#define SIGN SIGTSTP

void sig_handler(int signum, siginfo_t *info, void *context) {
  loop = false;
  printf("Inside handler function\n");
  if (inhandler) {
    printf("Signal reentering bug\n");
    exit(-1);
  }
  inhandler = true;
  raise(signum);

  auto uctx = (ucontext_t *)context;
  sigfillset(&uctx->uc_sigmask);
}

int main() {
  struct sigaction act = {0};

  act.sa_flags = SA_SIGINFO;
  act.sa_sigaction = &sig_handler;
  if (sigaction(SIGN, &act, NULL) != 0) {
    printf("sigaction() failed\n");
    return -2;
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

  printf("Exiting\n");
  return 0;
}
