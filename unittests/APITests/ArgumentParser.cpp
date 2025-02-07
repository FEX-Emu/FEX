#include <catch2/catch_test_macros.hpp>

#include <FEXHeaderUtils/StringArgumentParser.h>

TEST_CASE("Basic") {
  const auto ArgString = "Test a b c";
  auto Args = FHU::ParseArgumentsFromString(ArgString);
  REQUIRE(Args.size() == 2);
  CHECK(Args.at(0) == "Test");
  CHECK(Args.at(1) == "a b c");
}

TEST_CASE("Basic - Empty") {
  const auto ArgString = "";
  auto Args = FHU::ParseArgumentsFromString(ArgString);
  REQUIRE(Args.size() == 0);
}

TEST_CASE("Basic - Empty spaces") {
  const auto ArgString = "                       ";
  auto Args = FHU::ParseArgumentsFromString(ArgString);
  REQUIRE(Args.size() == 0);
}

TEST_CASE("Basic - Whitespace") {
  const auto ArgString = " \t   \f  \r   \n  \v   ";
  auto Args = FHU::ParseArgumentsFromString(ArgString);
  REQUIRE(Args.size() == 0);
}

TEST_CASE("Basic - Interpreter only") {
  const auto ArgString = "Test";
  auto Args = FHU::ParseArgumentsFromString(ArgString);
  REQUIRE(Args.size() == 1);
  CHECK(Args.at(0) == "Test");
}

TEST_CASE("Basic - Interpreter only with spaces") {
  const auto ArgString = "    Test    ";
  auto Args = FHU::ParseArgumentsFromString(ArgString);
  REQUIRE(Args.size() == 1);
  CHECK(Args.at(0) == "Test");
}

TEST_CASE("Basic - Space at start") {
  const auto ArgString = "      Test a b c";
  auto Args = FHU::ParseArgumentsFromString(ArgString);
  REQUIRE(Args.size() == 2);
  CHECK(Args.at(0) == "Test");
  CHECK(Args.at(1) == "a b c");
}

TEST_CASE("Basic - Bonus spaces between args") {
  const auto ArgString = "Test       a      b      c";
  auto Args = FHU::ParseArgumentsFromString(ArgString);
  REQUIRE(Args.size() == 2);
  CHECK(Args.at(0) == "Test");
  CHECK(Args.at(1) == "a      b      c");
}

TEST_CASE("Basic - non printable") {
  const auto ArgString = "Test a b \x01c";
  auto Args = FHU::ParseArgumentsFromString(ArgString);
  REQUIRE(Args.size() == 2);
  CHECK(Args.at(0) == "Test");
  CHECK(Args.at(1) == "a b \x01c");
}

TEST_CASE("Basic - Emoji") {
  const auto ArgString = "Test a b 🐸";
  auto Args = FHU::ParseArgumentsFromString(ArgString);
  REQUIRE(Args.size() == 2);
  CHECK(Args.at(0) == "Test");
  CHECK(Args.at(1) == "a b 🐸");
}

TEST_CASE("Basic - space at the end") {
  const auto ArgString = "Test a b 🐸        ";
  auto Args = FHU::ParseArgumentsFromString(ArgString);
  REQUIRE(Args.size() == 2);
  CHECK(Args.at(0) == "Test");
  CHECK(Args.at(1) == "a b 🐸");
}

TEST_CASE("Basic - whitespace between parts") {
  const auto ArgString = "\t\f\rTest\t\f\ra b 🐸\t\f\r";
  auto Args = FHU::ParseArgumentsFromString(ArgString);
  REQUIRE(Args.size() == 2);
  CHECK(Args.at(0) == "Test");
  CHECK(Args.at(1) == "a b 🐸");
}
