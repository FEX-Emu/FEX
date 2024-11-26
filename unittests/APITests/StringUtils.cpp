#include <catch2/catch_test_macros.hpp>
#include <Common/StringUtil.h>

static fextl::string ltrim_string(fextl::string in) {
  FEX::StringUtil::ltrim(in);
  return in;
}

static fextl::string rtrim_string(fextl::string in) {
  FEX::StringUtil::rtrim(in);
  return in;
}

static fextl::string trim_string(fextl::string in) {
  FEX::StringUtil::trim(in);
  return in;
}

TEST_CASE("ltrim") {
  CHECK(ltrim_string("") == "");
  CHECK(ltrim_string("FEXLoader") == "FEXLoader");

  CHECK(ltrim_string("FEXLoader\n") == "FEXLoader\n");
  CHECK(ltrim_string("FEXLoader\r") == "FEXLoader\r");
  CHECK(ltrim_string("FEXLoader\f") == "FEXLoader\f");
  CHECK(ltrim_string("FEXLoader\t") == "FEXLoader\t");
  CHECK(ltrim_string("FEXLoader\v") == "FEXLoader\v");
  CHECK(ltrim_string("FEXLoader ") == "FEXLoader ");

  CHECK(ltrim_string("\nFEXLoader") == "FEXLoader");
  CHECK(ltrim_string("\rFEXLoader") == "FEXLoader");
  CHECK(ltrim_string("\fFEXLoader") == "FEXLoader");
  CHECK(ltrim_string("\tFEXLoader") == "FEXLoader");
  CHECK(ltrim_string("\vFEXLoader") == "FEXLoader");
  CHECK(ltrim_string(" FEXLoader") == "FEXLoader");

  CHECK(ltrim_string("\nFEXLoader\n") == "FEXLoader\n");
  CHECK(ltrim_string("\rFEXLoader\r") == "FEXLoader\r");
  CHECK(ltrim_string("\fFEXLoader\f") == "FEXLoader\f");
  CHECK(ltrim_string("\tFEXLoader\t") == "FEXLoader\t");
  CHECK(ltrim_string("\vFEXLoader\v") == "FEXLoader\v");
  CHECK(ltrim_string(" FEXLoader ") == "FEXLoader ");
}

TEST_CASE("rtrim") {
  CHECK(rtrim_string("") == "");
  CHECK(rtrim_string("FEXLoader") == "FEXLoader");

  CHECK(rtrim_string("FEXLoader\n") == "FEXLoader");
  CHECK(rtrim_string("FEXLoader\r") == "FEXLoader");
  CHECK(rtrim_string("FEXLoader\f") == "FEXLoader");
  CHECK(rtrim_string("FEXLoader\t") == "FEXLoader");
  CHECK(rtrim_string("FEXLoader\v") == "FEXLoader");
  CHECK(rtrim_string("FEXLoader ") == "FEXLoader");

  CHECK(rtrim_string("\nFEXLoader") == "\nFEXLoader");
  CHECK(rtrim_string("\rFEXLoader") == "\rFEXLoader");
  CHECK(rtrim_string("\fFEXLoader") == "\fFEXLoader");
  CHECK(rtrim_string("\tFEXLoader") == "\tFEXLoader");
  CHECK(rtrim_string("\vFEXLoader") == "\vFEXLoader");
  CHECK(rtrim_string(" FEXLoader") == " FEXLoader");

  CHECK(rtrim_string("\nFEXLoader\n") == "\nFEXLoader");
  CHECK(rtrim_string("\rFEXLoader\r") == "\rFEXLoader");
  CHECK(rtrim_string("\fFEXLoader\f") == "\fFEXLoader");
  CHECK(rtrim_string("\tFEXLoader\t") == "\tFEXLoader");
  CHECK(rtrim_string("\vFEXLoader\v") == "\vFEXLoader");
  CHECK(rtrim_string(" FEXLoader ") == " FEXLoader");
}

TEST_CASE("trim") {
  CHECK(trim_string("") == "");
  CHECK(trim_string("FEXLoader") == "FEXLoader");

  CHECK(trim_string("FEXLoader\n") == "FEXLoader");
  CHECK(trim_string("FEXLoader\r") == "FEXLoader");
  CHECK(trim_string("FEXLoader\f") == "FEXLoader");
  CHECK(trim_string("FEXLoader\t") == "FEXLoader");
  CHECK(trim_string("FEXLoader\v") == "FEXLoader");
  CHECK(trim_string("FEXLoader ") == "FEXLoader");

  CHECK(trim_string("\nFEXLoader") == "FEXLoader");
  CHECK(trim_string("\rFEXLoader") == "FEXLoader");
  CHECK(trim_string("\fFEXLoader") == "FEXLoader");
  CHECK(trim_string("\tFEXLoader") == "FEXLoader");
  CHECK(trim_string("\vFEXLoader") == "FEXLoader");
  CHECK(trim_string(" FEXLoader") == "FEXLoader");

  CHECK(trim_string("\nFEXLoader\n") == "FEXLoader");
  CHECK(trim_string("\rFEXLoader\r") == "FEXLoader");
  CHECK(trim_string("\fFEXLoader\f") == "FEXLoader");
  CHECK(trim_string("\tFEXLoader\t") == "FEXLoader");
  CHECK(trim_string("\vFEXLoader\v") == "FEXLoader");
  CHECK(trim_string(" FEXLoader ") == "FEXLoader");
}
