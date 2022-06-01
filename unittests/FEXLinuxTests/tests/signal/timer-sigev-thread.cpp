//libs: rt pthread

// Simple test of timer_create + SIGEV_THREAD, glibc implements it via SIG32

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <cassert>
#include <signal.h>
#include <time.h>

int test;

void timer_handler(union sigval sv) {
  auto ok = sv.sival_ptr == &test;
  printf("timer_handler called, ok = %d\n", ok);

  exit(ok ? 0 : -1);
}

int main() {

  timer_t timer;
  sigevent sige;
  itimerspec spec;

  memset(&sige, 0, sizeof(sige));

  sige.sigev_notify = SIGEV_THREAD;
  sige.sigev_notify_function = &timer_handler;
  sige.sigev_value.sival_ptr = &test;

  timer_create(CLOCK_REALTIME, &sige, &timer);

  memset(&spec, 0, sizeof(spec));

  spec.it_value.tv_sec = 0;
  spec.it_value.tv_nsec = 1;

  timer_settime(timer, 0, &spec, NULL);

  for (;;)
    sleep(1);

  assert(false && "should never get here");
  return -2;
}