#include <catch2/catch_test_macros.hpp>
#include <FEXCore/Utils/StringUtils.h>

using namespace FEXCore::StringUtils;

TEST_CASE("ltrim") {
  CHECK(LeftTrim("") == "");
  CHECK(LeftTrim("FEXLoader") == "FEXLoader");

  CHECK(LeftTrim("FEXLoader\n") == "FEXLoader\n");
  CHECK(LeftTrim("FEXLoader\r") == "FEXLoader\r");
  CHECK(LeftTrim("FEXLoader\f") == "FEXLoader\f");
  CHECK(LeftTrim("FEXLoader\t") == "FEXLoader\t");
  CHECK(LeftTrim("FEXLoader\v") == "FEXLoader\v");
  CHECK(LeftTrim("FEXLoader ") == "FEXLoader ");

  CHECK(LeftTrim("\nFEXLoader") == "FEXLoader");
  CHECK(LeftTrim("\rFEXLoader") == "FEXLoader");
  CHECK(LeftTrim("\fFEXLoader") == "FEXLoader");
  CHECK(LeftTrim("\tFEXLoader") == "FEXLoader");
  CHECK(LeftTrim("\vFEXLoader") == "FEXLoader");
  CHECK(LeftTrim(" FEXLoader") == "FEXLoader");

  CHECK(LeftTrim("\nFEXLoader\n") == "FEXLoader\n");
  CHECK(LeftTrim("\rFEXLoader\r") == "FEXLoader\r");
  CHECK(LeftTrim("\fFEXLoader\f") == "FEXLoader\f");
  CHECK(LeftTrim("\tFEXLoader\t") == "FEXLoader\t");
  CHECK(LeftTrim("\vFEXLoader\v") == "FEXLoader\v");
  CHECK(LeftTrim(" FEXLoader ") == "FEXLoader ");
}

TEST_CASE("rtrim") {
  CHECK(RightTrim("") == "");
  CHECK(RightTrim("FEXLoader") == "FEXLoader");

  CHECK(RightTrim("FEXLoader\n") == "FEXLoader");
  CHECK(RightTrim("FEXLoader\r") == "FEXLoader");
  CHECK(RightTrim("FEXLoader\f") == "FEXLoader");
  CHECK(RightTrim("FEXLoader\t") == "FEXLoader");
  CHECK(RightTrim("FEXLoader\v") == "FEXLoader");
  CHECK(RightTrim("FEXLoader ") == "FEXLoader");

  CHECK(RightTrim("\nFEXLoader") == "\nFEXLoader");
  CHECK(RightTrim("\rFEXLoader") == "\rFEXLoader");
  CHECK(RightTrim("\fFEXLoader") == "\fFEXLoader");
  CHECK(RightTrim("\tFEXLoader") == "\tFEXLoader");
  CHECK(RightTrim("\vFEXLoader") == "\vFEXLoader");
  CHECK(RightTrim(" FEXLoader") == " FEXLoader");

  CHECK(RightTrim("\nFEXLoader\n") == "\nFEXLoader");
  CHECK(RightTrim("\rFEXLoader\r") == "\rFEXLoader");
  CHECK(RightTrim("\fFEXLoader\f") == "\fFEXLoader");
  CHECK(RightTrim("\tFEXLoader\t") == "\tFEXLoader");
  CHECK(RightTrim("\vFEXLoader\v") == "\vFEXLoader");
  CHECK(RightTrim(" FEXLoader ") == " FEXLoader");
}

TEST_CASE("trim") {
  CHECK(Trim("") == "");
  CHECK(Trim("FEXLoader") == "FEXLoader");

  CHECK(Trim("FEXLoader\n") == "FEXLoader");
  CHECK(Trim("FEXLoader\r") == "FEXLoader");
  CHECK(Trim("FEXLoader\f") == "FEXLoader");
  CHECK(Trim("FEXLoader\t") == "FEXLoader");
  CHECK(Trim("FEXLoader\v") == "FEXLoader");
  CHECK(Trim("FEXLoader ") == "FEXLoader");

  CHECK(Trim("\nFEXLoader") == "FEXLoader");
  CHECK(Trim("\rFEXLoader") == "FEXLoader");
  CHECK(Trim("\fFEXLoader") == "FEXLoader");
  CHECK(Trim("\tFEXLoader") == "FEXLoader");
  CHECK(Trim("\vFEXLoader") == "FEXLoader");
  CHECK(Trim(" FEXLoader") == "FEXLoader");

  CHECK(Trim("\nFEXLoader\n") == "FEXLoader");
  CHECK(Trim("\rFEXLoader\r") == "FEXLoader");
  CHECK(Trim("\fFEXLoader\f") == "FEXLoader");
  CHECK(Trim("\tFEXLoader\t") == "FEXLoader");
  CHECK(Trim("\vFEXLoader\v") == "FEXLoader");
  CHECK(Trim(" FEXLoader ") == "FEXLoader");
}
