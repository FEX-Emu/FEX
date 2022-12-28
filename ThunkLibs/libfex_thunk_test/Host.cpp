/*
$info$
tags: thunklibs|asound
$end_info$
*/

#include <dlfcn.h>

#include <chrono>
#include <thread>

#include "common/Host.h"

#include "thunkgen_host_libfex_thunk_test.inl"

std::thread* thr;

void fexfn_impl_libfex_thunk_test_SetAsyncCallback(void(*cb)()) {
  auto host_cb = (void (*)())cb;
  FinalizeHostTrampolineForGuestFunction(host_cb);
  MakeHostTrampolineForGuestFunctionAsyncCallable(host_cb, 1);

  thr = new std::thread { [host_cb]() {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(500ms);
    host_cb();
  }};

//  host_cb();
  printf("HELLO WORLD\n");
}

EXPORTS(libfex_thunk_test)
