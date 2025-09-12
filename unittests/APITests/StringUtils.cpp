#include <catch2/catch_test_macros.hpp>
#include <FEXCore/Utils/StringUtils.h>

using namespace FEXCore::StringUtils;

TEST_CASE("ltrim") {
  CHECK(LeftTrim("") == "");
  CHECK(LeftTrim("FEXInterpreter") == "FEXInterpreter");

  CHECK(LeftTrim("FEXInterpreter\n") == "FEXInterpreter\n");
  CHECK(LeftTrim("FEXInterpreter\r") == "FEXInterpreter\r");
  CHECK(LeftTrim("FEXInterpreter\f") == "FEXInterpreter\f");
  CHECK(LeftTrim("FEXInterpreter\t") == "FEXInterpreter\t");
  CHECK(LeftTrim("FEXInterpreter\v") == "FEXInterpreter\v");
  CHECK(LeftTrim("FEXInterpreter ") == "FEXInterpreter ");

  CHECK(LeftTrim("\nFEXInterpreter") == "FEXInterpreter");
  CHECK(LeftTrim("\rFEXInterpreter") == "FEXInterpreter");
  CHECK(LeftTrim("\fFEXInterpreter") == "FEXInterpreter");
  CHECK(LeftTrim("\tFEXInterpreter") == "FEXInterpreter");
  CHECK(LeftTrim("\vFEXInterpreter") == "FEXInterpreter");
  CHECK(LeftTrim(" FEXInterpreter") == "FEXInterpreter");

  CHECK(LeftTrim("\nFEXInterpreter\n") == "FEXInterpreter\n");
  CHECK(LeftTrim("\rFEXInterpreter\r") == "FEXInterpreter\r");
  CHECK(LeftTrim("\fFEXInterpreter\f") == "FEXInterpreter\f");
  CHECK(LeftTrim("\tFEXInterpreter\t") == "FEXInterpreter\t");
  CHECK(LeftTrim("\vFEXInterpreter\v") == "FEXInterpreter\v");
  CHECK(LeftTrim(" FEXInterpreter ") == "FEXInterpreter ");
}

TEST_CASE("rtrim") {
  CHECK(RightTrim("") == "");
  CHECK(RightTrim("FEXInterpreter") == "FEXInterpreter");

  CHECK(RightTrim("FEXInterpreter\n") == "FEXInterpreter");
  CHECK(RightTrim("FEXInterpreter\r") == "FEXInterpreter");
  CHECK(RightTrim("FEXInterpreter\f") == "FEXInterpreter");
  CHECK(RightTrim("FEXInterpreter\t") == "FEXInterpreter");
  CHECK(RightTrim("FEXInterpreter\v") == "FEXInterpreter");
  CHECK(RightTrim("FEXInterpreter ") == "FEXInterpreter");

  CHECK(RightTrim("\nFEXInterpreter") == "\nFEXInterpreter");
  CHECK(RightTrim("\rFEXInterpreter") == "\rFEXInterpreter");
  CHECK(RightTrim("\fFEXInterpreter") == "\fFEXInterpreter");
  CHECK(RightTrim("\tFEXInterpreter") == "\tFEXInterpreter");
  CHECK(RightTrim("\vFEXInterpreter") == "\vFEXInterpreter");
  CHECK(RightTrim(" FEXInterpreter") == " FEXInterpreter");

  CHECK(RightTrim("\nFEXInterpreter\n") == "\nFEXInterpreter");
  CHECK(RightTrim("\rFEXInterpreter\r") == "\rFEXInterpreter");
  CHECK(RightTrim("\fFEXInterpreter\f") == "\fFEXInterpreter");
  CHECK(RightTrim("\tFEXInterpreter\t") == "\tFEXInterpreter");
  CHECK(RightTrim("\vFEXInterpreter\v") == "\vFEXInterpreter");
  CHECK(RightTrim(" FEXInterpreter ") == " FEXInterpreter");
}

TEST_CASE("trim") {
  CHECK(Trim("") == "");
  CHECK(Trim("FEXInterpreter") == "FEXInterpreter");

  CHECK(Trim("FEXInterpreter\n") == "FEXInterpreter");
  CHECK(Trim("FEXInterpreter\r") == "FEXInterpreter");
  CHECK(Trim("FEXInterpreter\f") == "FEXInterpreter");
  CHECK(Trim("FEXInterpreter\t") == "FEXInterpreter");
  CHECK(Trim("FEXInterpreter\v") == "FEXInterpreter");
  CHECK(Trim("FEXInterpreter ") == "FEXInterpreter");

  CHECK(Trim("\nFEXInterpreter") == "FEXInterpreter");
  CHECK(Trim("\rFEXInterpreter") == "FEXInterpreter");
  CHECK(Trim("\fFEXInterpreter") == "FEXInterpreter");
  CHECK(Trim("\tFEXInterpreter") == "FEXInterpreter");
  CHECK(Trim("\vFEXInterpreter") == "FEXInterpreter");
  CHECK(Trim(" FEXInterpreter") == "FEXInterpreter");

  CHECK(Trim("\nFEXInterpreter\n") == "FEXInterpreter");
  CHECK(Trim("\rFEXInterpreter\r") == "FEXInterpreter");
  CHECK(Trim("\fFEXInterpreter\f") == "FEXInterpreter");
  CHECK(Trim("\tFEXInterpreter\t") == "FEXInterpreter");
  CHECK(Trim("\vFEXInterpreter\v") == "FEXInterpreter");
  CHECK(Trim(" FEXInterpreter ") == "FEXInterpreter");
}
