#include "FEXCore/fextl/string.h"
#include <FEXCore/Utils/Regex.h>
#include <catch2/catch_test_macros.hpp>

using namespace FEXCore::Utils;

TEST_CASE("Singular regex") {
  CHECK(Regex("a").matches("a") == true);
  CHECK(Regex("a*").matches("aaaaaaa") == true);
  CHECK(Regex(".").matches("a") == true);
}

TEST_CASE("Concat regex") {
  CHECK(Regex("aaa").matches("aaa") == true);
  CHECK(Regex("ab").matches("ab") == true);
  CHECK(Regex("a").matches("ab") == false);
  CHECK(Regex("ab").matches("a") == false);
  CHECK(Regex("(aab)").matches("aab") == true);
}

TEST_CASE("Union regex") {
  CHECK(Regex("a|b").matches("a") == true);
  CHECK(Regex("a|b").matches("b") == true);
  CHECK(Regex("(ab)|b").matches("ab") == true);
  CHECK(Regex("(ab)|b").matches("b") == true);
  CHECK(Regex("(ab)|b").matches("abb") == false);
}

TEST_CASE("Dot regex") {
  CHECK(Regex(".*").matches("") == true);
  CHECK(Regex(".*").matches("setup.json") == true);
  CHECK(Regex("setup.*").matches("setup.json") == true);
  CHECK(Regex("setup.*").matches("setup/setup.json") == true);
  CHECK(Regex(".*setup.*").matches("setup/setup.json") == true);

  CHECK(Regex("setup\\.*").matches("setup/setup.json") == false);
  CHECK(Regex("setup\\.*").matches("setup.....") == true);
  CHECK(Regex("setup\\.*").matches("setup.aaaa") == false);
  CHECK(Regex("setup\\\.*").matches("setup\.aaaa") == false);
  CHECK(Regex("setup\\\.*").matches("setup\....") == true);
  CHECK(Regex("setup\\\.*").matches("setup\a") == false);
  CHECK(Regex("setup\\\.*").matches("setup\.\.\.\.") == true);
}

TEST_CASE("Plus regex") {
  CHECK(Regex("setup.+").matches("setup") == false);
  CHECK(Regex("aa").matches("aa") == true);
  CHECK(Regex("aa+").matches("aaa") == true);
  CHECK(Regex("aa+").matches("aab") == false);
}

TEST_CASE("Question regex") {
  CHECK(Regex(".?").matches("") == true);
  CHECK(Regex(".?").matches("aa") == false);
  CHECK(Regex("setup.?").matches("setup") == true);
  CHECK(Regex("setup.?").matches("setupa") == true);
  CHECK(Regex("setup.?").matches("setupb") == true);
  CHECK(Regex("aa?").matches("aa") == true);
}

// Tests potential usage inside fex itself
TEST_CASE("FEX regex") {
  CHECK(Regex(".*Config.*").matches("/home/ubuntu/.fex-emu/Config.json") == true);
}
