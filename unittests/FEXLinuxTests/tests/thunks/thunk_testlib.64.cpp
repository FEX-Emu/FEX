#include <dlfcn.h>

#include <stdexcept>

#include <catch2/catch.hpp>

#include "../../../../ThunkLibs/libfex_thunk_test/api.h"

struct Fixture {
  void* lib = []() {
    auto ret = dlopen("libfex_thunk_test.so", RTLD_LAZY);
    if (!ret) {
      throw std::runtime_error("Failed to open lib\n");
    }
    return ret;
  }();

#define GET_SYMBOL(name) decltype(&::name) name = (decltype(name))dlsym(lib, #name)
  GET_SYMBOL(GetDoubledValue);
};

TEST_CASE_METHOD(Fixture, "Trivial") {
  CHECK(GetDoubledValue(10) == 20);
}
