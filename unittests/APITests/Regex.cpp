#include "FEXCore/fextl/string.h"
#include <FEXCore/Utils/Regex.h>
#include <catch2/catch_test_macros.hpp>

using namespace FEXCore::Utils;

TEST_CASE("Singular regex") {
  CHECK(Regex("a").matches("a") == true);
  CHECK(Regex("a*").matches("aaaaaaa") == true);
  CHECK(Regex(".").matches("a") == true);
}
