#include "FEXCore/fextl/string.h"
#include <FEXCore/Utils/Regex.h>
#include <catch2/catch_test_macros.hpp>

using namespace FEXCore::Utils;

TEST_CASE("Singular regex") {
  CHECK(Regex::matches("a", "a") == true);
  CHECK(Regex::matches("a*", "aaaaaaa") == true);
}

TEST_CASE("Concat regex") {
  CHECK(Regex::matches("aaa", "aaa") == true);
  CHECK(Regex::matches("ab", "ab") == true);
  CHECK(Regex::matches("a", "ab") == false);
  CHECK(Regex::matches("ab", "a") == false);
}

TEST_CASE("Wildcard regex") {
  CHECK(Regex::matches("*", "") == true);
  CHECK(Regex::matches("*", "setup.json") == true);
  CHECK(Regex::matches("setup.*", "setupjson") == false);
  CHECK(Regex::matches("setup*", "setup.json") == true);
  CHECK(Regex::matches("setup*", "setup/setup.json") == true);
  CHECK(Regex::matches("*setup*", "setup/setup.json") == true);
}


// Tests potential usage inside fex itself
TEST_CASE("FEX regex") {
  CHECK(Regex::matches("*Config*", "/home/ubuntu/.fex-emu/Config.json") == true);
  CHECK(Regex::matches("*Config.json", "/home/ubuntu/.fex-emu/Config.json") == true);
}
