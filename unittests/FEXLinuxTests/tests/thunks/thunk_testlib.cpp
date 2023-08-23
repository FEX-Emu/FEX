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
  GET_SYMBOL(MakeReorderingType);
  GET_SYMBOL(GetReorderingTypeMember);
  GET_SYMBOL(ModifyReorderingTypeMembers);
  GET_SYMBOL(QueryOffsetOf);
};

TEST_CASE_METHOD(Fixture, "Trivial") {
  CHECK(GetDoubledValue(10) == 20);
}

TEST_CASE_METHOD(Fixture, "Automatic struct repacking") {
  {
    // Test repacking of return values
    ReorderingType test_struct = MakeReorderingType(0x1234, 0x5678);
    REQUIRE(test_struct.a == 0x1234);
    REQUIRE(test_struct.b == 0x5678);

    // Test offsets of the host-side guest_layout wrapper match the guest-side ones
    CHECK(QueryOffsetOf(&test_struct, 0) == offsetof(ReorderingType, a));
    CHECK(QueryOffsetOf(&test_struct, 1) == offsetof(ReorderingType, b));

    // Test repacking of input pointers
    CHECK(GetReorderingTypeMember(&test_struct, 0) == 0x1234);
    CHECK(GetReorderingTypeMember(&test_struct, 1) == 0x5678);

    // Test repacking of output pointers
    ModifyReorderingTypeMembers(&test_struct);
    CHECK(GetReorderingTypeMember(&test_struct, 0) == 0x1235);
    CHECK(GetReorderingTypeMember(&test_struct, 1) == 0x567a);
  };
}
