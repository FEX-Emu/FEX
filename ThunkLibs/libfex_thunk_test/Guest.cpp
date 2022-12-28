/*
$info$
tags: thunklibs|fex_thunk_test
$end_info$
*/

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "common/Guest.h"

#include "thunkgen_guest_libfex_thunk_test.inl"

struct OnStart {
  std::thread thr;
  std::atomic<bool> done { false };
  std::mutex m;
  std::condition_variable var;

  OnStart() : thr([this]() {
    struct { unsigned id = 1; } args;
    fexthunks_fex_register_async_worker_thread(&args);
  }) {}

  ~OnStart() {
    struct { unsigned id = 1; } args;
    fexthunks_fex_unregister_async_worker_thread(&args);
    thr.join();
  }
} on_start;

extern "C" void SetAsyncCallback(void (*cb)()) {
  fexfn_pack_SetAsyncCallback(cb);
}

LOAD_LIB(libfex_thunk_test)
