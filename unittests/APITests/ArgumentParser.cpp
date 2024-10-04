#include <catch2/catch_test_macros.hpp>

#include <FEXHeaderUtils/StringArgumentParser.h>

TEST_CASE("Basic") {
  const auto ArgString = "Test a b c";
  auto Args = FHU::ParseArgumentsFromString(ArgString);
  REQUIRE(Args.size() == 4);
  CHECK(Args.at(0) == "Test");
  CHECK(Args.at(1) == "a");
  CHECK(Args.at(2) == "b");
  CHECK(Args.at(3) == "c");
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

TEST_CASE("Basic - Space at start") {
  const auto ArgString = "      Test a b c";
  auto Args = FHU::ParseArgumentsFromString(ArgString);
  REQUIRE(Args.size() == 4);
  CHECK(Args.at(0) == "Test");
  CHECK(Args.at(1) == "a");
  CHECK(Args.at(2) == "b");
  CHECK(Args.at(3) == "c");
}

TEST_CASE("Basic - Bonus spaces between args") {
  const auto ArgString = "Test       a      b      c";
  auto Args = FHU::ParseArgumentsFromString(ArgString);
  REQUIRE(Args.size() == 4);
  CHECK(Args.at(0) == "Test");
  CHECK(Args.at(1) == "a");
  CHECK(Args.at(2) == "b");
  CHECK(Args.at(3) == "c");
}

TEST_CASE("Basic - non printable") {
  const auto ArgString = "Test a b \x01c";
  auto Args = FHU::ParseArgumentsFromString(ArgString);
  REQUIRE(Args.size() == 4);
  CHECK(Args.at(0) == "Test");
  CHECK(Args.at(1) == "a");
  CHECK(Args.at(2) == "b");
  CHECK(Args.at(3) == "\x01c");
}

TEST_CASE("Basic - Emoji") {
  const auto ArgString = "Test a b 🐸";
  auto Args = FHU::ParseArgumentsFromString(ArgString);
  REQUIRE(Args.size() == 4);
  CHECK(Args.at(0) == "Test");
  CHECK(Args.at(1) == "a");
  CHECK(Args.at(2) == "b");
  CHECK(Args.at(3) == "🐸");
}

TEST_CASE("Basic - space at the end") {
  const auto ArgString = "Test a b 🐸        ";
  auto Args = FHU::ParseArgumentsFromString(ArgString);
  REQUIRE(Args.size() == 4);
  CHECK(Args.at(0) == "Test");
  CHECK(Args.at(1) == "a");
  CHECK(Args.at(2) == "b");
  CHECK(Args.at(3) == "🐸");
}
