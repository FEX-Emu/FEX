// SPDX-License-Identifier: MIT
#include <catch2/catch_all.hpp>

#include "Utils/crc32.h"

TEST_CASE("Simple") {
  uint32_t data = 0x41424344U;
  CHECK(FEXCore::Utils::crc32(&data, sizeof(data)) == 0xa53ea072);
}
