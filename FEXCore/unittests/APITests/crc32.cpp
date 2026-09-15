// SPDX-License-Identifier: MIT
#include <catch2/catch_all.hpp>

#include "Utils/crc32.h"

TEST_CASE("Simple") {
  uint32_t data = 0x41424344U;
  // Arm and x86 use different polynomials by default.
#if defined(ARCHITECTURE_arm64)
  CHECK(FEXCore::Utils::crc32(&data, sizeof(data)) == 0xa53ea072);
#elif defined(ARCHITECTURE_x86_64)
  CHECK(FEXCore::Utils::crc32(&data, sizeof(data)) == 0x968d46eb);
#endif
}
