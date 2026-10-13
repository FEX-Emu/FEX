#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <pthread.h>
#include <sys/resource.h>
#include <unistd.h>

static void* ThreadFunc(void* Arg) {
  auto Semaphore = static_cast<std::atomic<bool>*>(Arg);
  while (*Semaphore) {
    // Sleep for a bit to avoid thrashing.
    usleep(100'000);
  }
  return nullptr;
}

// NB: At present, empirical testing shows that we can
// actually set this to ~45, and still pass. To give us wiggle room for
// any other per thread overhead let's set it to a nice round number instead.
#define NUM_THREADS 32

// Chromium's sandbox lowers RLIMIT_DATA to 16GB and then spawns threads.
// Historically, FEX would allocate a several TB writtable region
// for its host stack, which would blow past this limit.
TEST_CASE("pthread_create after lowering RLIMIT_DATA") {
  rlimit64 Limit {
    .rlim_cur = 16ULL * 1024 * 1024 * 1024,
    .rlim_max = 16ULL * 1024 * 1024 * 1024,
  };
  REQUIRE(setrlimit64(RLIMIT_DATA, &Limit) == 0);

  pthread_t Threads[NUM_THREADS];
  std::atomic<bool> Semaphore = true;
  for (int i = 0; i < NUM_THREADS; ++i) {
    REQUIRE(pthread_create(&Threads[i], nullptr, ThreadFunc, &Semaphore) == 0);
  }

  Semaphore = false;
  for (int i = 0; i < NUM_THREADS; ++i) {
    CHECK(pthread_join(Threads[i], nullptr) == 0);
  }
}
