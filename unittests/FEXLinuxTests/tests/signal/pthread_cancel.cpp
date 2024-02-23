#include <catch2/catch.hpp>

#include <atomic>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Derived from example in https://manual.cs50.io/3/pthread_cancel
// <<Manual pages for the C standard library, C POSIX library, and the CS50 Library>>

std::atomic<bool> thread_ready;
std::atomic<bool> cancel_sent;

static pthread_key_t key;

void key_dtor(void* ptr) {
  puts("key_dtor: Thread aborted\n");
  free(ptr);
}

#define handle_error_en(en, msg) \
  do { \
    errno = en; \
    perror(msg); \
    exit(EXIT_FAILURE); \
  } while (0)

static void* thread_func(void* ignored_argument) {
  pthread_key_create(&key, &key_dtor);
  pthread_setspecific(key, malloc(32));
  int s;

  /* Disable cancellation for a while, so that we don't
     immediately react to a cancellation request. */

  s = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
  if (s != 0) {
    handle_error_en(s, "pthread_setcancelstate");
  }

  printf("thread_func(): started; cancellation disabled\n");
  thread_ready = true;

  while (!cancel_sent.load())
    ;
  printf("thread_func(): about to enable cancellation\n");

  s = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  if (s != 0) {
    handle_error_en(s, "pthread_setcancelstate");
  }

  /* sleep() is a cancellation point. */

  for (;;) {
    sleep(1000); /* Should get canceled while we sleep */
  }

  /* Should never get here. */

  printf("thread_func(): not canceled!\n");
  return NULL;
}

TEST_CASE("pthreads cancel") {
  pthread_t thr;
  void* res;
  int s;

  /* Start a thread and then send it a cancellation request. */

  REQUIRE(pthread_create(&thr, NULL, &thread_func, NULL) == 0);

  while (!thread_ready.load())
    ;

  printf("main(): sending cancellation request\n");
  REQUIRE(pthread_cancel(thr) == 0);

  cancel_sent = true;

  /* Join with thread to see what its exit status was. */

  REQUIRE(pthread_join(thr, &res) == 0);

  CHECK(res == PTHREAD_CANCELED);
}
