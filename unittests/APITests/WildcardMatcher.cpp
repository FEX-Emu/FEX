#include "FEXCore/fextl/string.h"
#include <FEXCore/Utils/WildcardMatcher.h>
#include <catch2/catch_test_macros.hpp>

using namespace FEXCore::Utils::Wildcard;

TEST_CASE("Singular regex") {
  CHECK(Matches("a", "a"));
  CHECK(Matches("a*", "a*"));
  CHECK(Matches("a*", "aaaaaaa"));
}

TEST_CASE("Concat regex") {
  CHECK(Matches("aaa", "aaa"));
  CHECK(Matches("ab", "ab"));
  CHECK(!Matches("a", "ab"));
  CHECK(!Matches("ab", "a"));
}
TEST_CASE("Wildcard beginning end") {
  CHECK(Matches("a*", "a"));
  CHECK(Matches("*a", "a"));
  CHECK(Matches("*a*", "a"));
}
TEST_CASE("Wildcard middle") {
  CHECK(Matches("test*pattern", "test__pattern"));
}

TEST_CASE("Wildcard mult") {
  CHECK(Matches("test*pattern*more", "test__pattern__more"));
  CHECK(Matches("test**pattern", "test_pattern"));
}

TEST_CASE("Wildcard regex simple") {
  CHECK(Matches("*", ""));
  CHECK(Matches("*", "setup.json"));
  CHECK(Matches("test*pattern", "test__pattern"));
  CHECK(!Matches("setup.*", "setupjson"));
  CHECK(Matches("setup*", "setup.json"));
  CHECK(Matches("setup*", "setup/setup.json"));
  CHECK(Matches("*setup*", "setup/setup.json"));
}


// Tests potential usage inside fex itself
TEST_CASE("FEX regex") {
  CHECK(Matches("*Config*", "/home/ubuntu/.fex-emu/Config.json"));
  CHECK(Matches("*Config.json", "/home/ubuntu/.fex-emu/Config.json"));
}
