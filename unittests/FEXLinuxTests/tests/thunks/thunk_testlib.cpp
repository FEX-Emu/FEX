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

  GET_SYMBOL(MakeOpaqueType);
  GET_SYMBOL(ReadOpaqueTypeData);
  GET_SYMBOL(DestroyOpaqueType);

  GET_SYMBOL(MakeUnionType);
  GET_SYMBOL(GetUnionTypeA);
};

TEST_CASE_METHOD(Fixture, "Trivial") {
  CHECK(GetDoubledValue(10) == 20);
}

TEST_CASE_METHOD(Fixture, "Opaque data types") {
  {
    auto data = MakeOpaqueType(0x1234);
    CHECK(ReadOpaqueTypeData(data) == 0x1234);
    DestroyOpaqueType(data);
  }

  {
    auto data = MakeUnionType(0x1, 0x2, 0x3, 0x4);
    CHECK(GetUnionTypeA(&data) == 0x04030201);
  }
}
