#include "FEXCore/fextl/string.h"
#include <FEXCore/Utils/Regex.h>
#include <catch2/catch_test_macros.hpp>

using namespace FEXCore::Utils;

TEST_CASE("Singular regex") {
  CHECK(Regex("a").matches("a") == true);
  CHECK(Regex("a*").matches("aaaaaaa") == true);
}

TEST_CASE("Concat regex") {
  CHECK(Regex("aaa").matches("aaa") == true);
  CHECK(Regex("ab").matches("ab") == true);
  CHECK(Regex("a").matches("ab") == false);
  CHECK(Regex("ab").matches("a") == false);
}

TEST_CASE("Dot regex") {
  CHECK(Regex("*").matches("") == true);
  CHECK(Regex("*").matches("setup.json") == true);
  CHECK(Regex("setup.*").matches("setupjson") == false);
  CHECK(Regex("setup*").matches("setup.json") == true);
  CHECK(Regex("setup*").matches("setup/setup.json") == true);
  CHECK(Regex("*setup*").matches("setup/setup.json") == true);
}


// Tests potential usage inside fex itself
TEST_CASE("FEX regex") {
  CHECK(Regex("*Config*").matches("/home/ubuntu/.fex-emu/Config.json") == true);
  CHECK(Regex("*Config.json").matches("/home/ubuntu/.fex-emu/Config.json") == true);
}
