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

  GET_SYMBOL(MakeReorderingType);
  GET_SYMBOL(GetReorderingTypeMember);
  GET_SYMBOL(GetReorderingTypeMemberWithoutRepacking);
  GET_SYMBOL(ModifyReorderingTypeMembers);
  GET_SYMBOL(QueryOffsetOf);

  GET_SYMBOL(RanCustomRepack);

  GET_SYMBOL(FunctionWithDivergentSignature);
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

    // Test that we can force reinterpreting the data in guest layout as host layout
    CHECK(GetReorderingTypeMemberWithoutRepacking(&test_struct, 0) == 0x5678);
    CHECK(GetReorderingTypeMemberWithoutRepacking(&test_struct, 1) == 0x1234);

    // Test repacking of output pointers
    ModifyReorderingTypeMembers(&test_struct);
    CHECK(GetReorderingTypeMember(&test_struct, 0) == 0x1235);
    CHECK(GetReorderingTypeMember(&test_struct, 1) == 0x567a);
  };
}

TEST_CASE_METHOD(Fixture, "Assisted struct repacking") {
  CustomRepackedType data {};
  CHECK(RanCustomRepack(&data) == 1);
}

TEST_CASE_METHOD(Fixture, "Function signature with differing parameter sizes") {
  CHECK(FunctionWithDivergentSignature(DivType{1}, DivType{2}, DivType{3}, DivType{4}) == 0x01020304);
}
