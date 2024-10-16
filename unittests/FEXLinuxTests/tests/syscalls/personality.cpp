#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <sys/personality.h>
#include <sys/utsname.h>
#include <string_view>

constexpr uint32_t QUERY_PERSONA = ~0U;
TEST_CASE("default - query") {
  REQUIRE(::personality(0) != -1);

  auto persona = ::personality(QUERY_PERSONA);
  CHECK(persona == 0);
}

TEST_CASE("default - set all") {
  REQUIRE(::personality(-2U) != -1);
  auto persona = ::personality(QUERY_PERSONA);
  CHECK(persona == -2U);
}

TEST_CASE("default - check linux32") {
  REQUIRE(::personality(0) != -1);

  struct utsname name {};
  uname(&name);
  CHECK(std::string_view(name.machine) == "x86_64");

  CHECK(::personality(PER_LINUX32) != -1);
  auto persona = ::personality(QUERY_PERSONA);
  CHECK(persona == PER_LINUX32);

  uname(&name);
  CHECK(std::string_view(name.machine) == "i686");
}

TEST_CASE("default - check uname26") {
  REQUIRE(::personality(UNAME26) != -1);
  auto persona = ::personality(QUERY_PERSONA);
  CHECK(persona == UNAME26);

  struct utsname name {};
  uname(&name);
  CHECK(std::string_view(name.release).starts_with("2.6."));
}
