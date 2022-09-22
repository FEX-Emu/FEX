// Simple test of timer_create + SIGEV_THREAD, glibc implements it via SIG32

#include <catch2/catch.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <cassert>
#include <optional>
#include <signal.h>
#include <time.h>

int test;

std::optional<bool> sigval_ack;

void timer_handler(union sigval sv) {
  sigval_ack = sv.sival_ptr == &test;
  printf("timer_handler called, ok = %d\n", *sigval_ack);
}

// These sometimes crash FEX with SIGSEGV
TEST_CASE("timer_create and SIGEV_THREAD", "[!mayfail]") {
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

  while (!sigval_ack) {
    usleep(10);
  }

  REQUIRE(sigval_ack.has_value());
  CHECK(*sigval_ack == true);
}
