#include <FEXCore/Utils/MathUtils.h>
#include <catch2/catch.hpp>

TEST_CASE("ILog2") {
  auto i = GENERATE(range(0, 64));
  REQUIRE(FEXCore::ilog2(1ull << i) == i);
}
