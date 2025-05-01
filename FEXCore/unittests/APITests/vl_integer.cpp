// SPDX-License-Identifier: MIT
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <catch2/generators/catch_generators_random.hpp>

#include "Utils/variable_length_integer.h"

#include <limits>

TEST_CASE("vl-size") {
  // Check 8-bit minimum and maximum.
  CHECK(FEXCore::Utils::vl64::EncodedSize(-64) == 1);
  CHECK(FEXCore::Utils::vl64::EncodedSize(63) == 1);

  // Check 16-bit minimum and maximum.
  CHECK(FEXCore::Utils::vl64::EncodedSize(-8192) == 2);
  CHECK(FEXCore::Utils::vl64::EncodedSize(8191) == 2);

  // Check 32-bit minimum and maximum.
  CHECK(FEXCore::Utils::vl64::EncodedSize(std::numeric_limits<int32_t>::min()) == 5);
  CHECK(FEXCore::Utils::vl64::EncodedSize(std::numeric_limits<int32_t>::max()) == 5);

  // Check 64-bit minimum and maximum.
  CHECK(FEXCore::Utils::vl64::EncodedSize(std::numeric_limits<int64_t>::min()) == 9);
  CHECK(FEXCore::Utils::vl64::EncodedSize(std::numeric_limits<int64_t>::max()) == 9);
}

TEST_CASE("vl8 - in memory - encode/decode") {
  uint8_t data[1];
  REQUIRE(FEXCore::Utils::vl64::Encode(data, 0) == 1);
  CHECK(data[0] == 0);
  auto Dec = FEXCore::Utils::vl64::Decode(data);
  CHECK(Dec.Size == 1);
  CHECK(Dec.Integer == 0);

  REQUIRE(FEXCore::Utils::vl64::Encode(data, 63) == 1);
  CHECK(data[0] == 0b0011'1111);
  Dec = FEXCore::Utils::vl64::Decode(data);
  CHECK(Dec.Size == 1);
  CHECK(Dec.Integer == 63);

  REQUIRE(FEXCore::Utils::vl64::Encode(data, -1) == 1);
  CHECK(data[0] == 0b0111'1111);
  Dec = FEXCore::Utils::vl64::Decode(data);
  CHECK(Dec.Size == 1);
  CHECK(Dec.Integer == -1);

  REQUIRE(FEXCore::Utils::vl64::Encode(data, -64) == 1);
  CHECK(data[0] == 0b0100'0000);
  Dec = FEXCore::Utils::vl64::Decode(data);
  CHECK(Dec.Size == 1);
  CHECK(Dec.Integer == -64);
}

TEST_CASE("vl16 - in memory - encode/decode") {
  uint8_t data[2];

  REQUIRE(FEXCore::Utils::vl64::Encode(data, -65) == 2);
  CHECK((uint64_t)data[0] == 0b1011'1111);
  CHECK((uint64_t)data[1] == 0b1011'1111);
  auto Dec = FEXCore::Utils::vl64::Decode(data);
  CHECK(Dec.Size == 2);
  CHECK(Dec.Integer == -65);

  REQUIRE(FEXCore::Utils::vl64::Encode(data, -66) == 2);
  CHECK((uint64_t)data[0] == 0b1011'1111);
  CHECK((uint64_t)data[1] == 0b1011'1110);
  Dec = FEXCore::Utils::vl64::Decode(data);
  CHECK(Dec.Size == 2);
  CHECK(Dec.Integer == -66);

  REQUIRE(FEXCore::Utils::vl64::Encode(data, 64) == 2);
  CHECK((uint64_t)data[0] == 0b1000'0000);
  CHECK((uint64_t)data[1] == 0b0100'0000);
  Dec = FEXCore::Utils::vl64::Decode(data);
  CHECK(Dec.Size == 2);
  CHECK(Dec.Integer == 64);

  REQUIRE(FEXCore::Utils::vl64::Encode(data, 8191) == 2);
  CHECK((uint64_t)data[0] == 0b1001'1111);
  CHECK((uint64_t)data[1] == 0b1111'1111);
  Dec = FEXCore::Utils::vl64::Decode(data);
  CHECK(Dec.Size == 2);
  CHECK(Dec.Integer == 8191);

  REQUIRE(FEXCore::Utils::vl64::Encode(data, -8192) == 2);
  CHECK((uint64_t)data[0] == 0b1010'0000);
  CHECK((uint64_t)data[1] == 0b0000'0000);
  Dec = FEXCore::Utils::vl64::Decode(data);
  CHECK(Dec.Size == 2);
  CHECK(Dec.Integer == -8192);
}

TEST_CASE("vl32 - in memory - encode/decode") {
  uint8_t data[5];
  int32_t result {};

  REQUIRE(FEXCore::Utils::vl64::Encode(data, 8192) == 5);
  CHECK(data[0] == 0b1100'0000);
  memcpy(&result, &data[1], sizeof(int32_t));
  CHECK(result == 8192);
  auto Dec = FEXCore::Utils::vl64::Decode(data);
  CHECK(Dec.Size == 5);
  CHECK(Dec.Integer == 8192);

  REQUIRE(FEXCore::Utils::vl64::Encode(data, -8193) == 5);
  CHECK(data[0] == 0b1100'0000);
  memcpy(&result, &data[1], sizeof(int32_t));
  CHECK(result == -8193);
  Dec = FEXCore::Utils::vl64::Decode(data);
  CHECK(Dec.Size == 5);
  CHECK(Dec.Integer == -8193);

  REQUIRE(FEXCore::Utils::vl64::Encode(data, std::numeric_limits<int32_t>::min()) == 5);
  CHECK(data[0] == 0b1100'0000);
  memcpy(&result, &data[1], sizeof(int32_t));
  CHECK(result == std::numeric_limits<int32_t>::min());
  Dec = FEXCore::Utils::vl64::Decode(data);
  CHECK(Dec.Size == 5);
  CHECK(Dec.Integer == std::numeric_limits<int32_t>::min());

  REQUIRE(FEXCore::Utils::vl64::Encode(data, std::numeric_limits<int32_t>::max()) == 5);
  CHECK(data[0] == 0b1100'0000);
  memcpy(&result, &data[1], sizeof(int32_t));
  CHECK(result == std::numeric_limits<int32_t>::max());
  Dec = FEXCore::Utils::vl64::Decode(data);
  CHECK(Dec.Size == 5);
  CHECK(Dec.Integer == std::numeric_limits<int32_t>::max());
}

TEST_CASE("vl64 - in memory - encode/decode") {
  uint8_t data[9];
  int64_t result {};

  REQUIRE(FEXCore::Utils::vl64::Encode(data, static_cast<int64_t>(std::numeric_limits<int32_t>::min()) - 1) == 9);
  CHECK(data[0] == 0b1110'0000);
  memcpy(&result, &data[1], sizeof(int64_t));
  CHECK(result == static_cast<int64_t>(std::numeric_limits<int32_t>::min()) - 1);
  auto Dec = FEXCore::Utils::vl64::Decode(data);
  CHECK(Dec.Size == 9);
  CHECK(Dec.Integer == static_cast<int64_t>(std::numeric_limits<int32_t>::min()) - 1);

  REQUIRE(FEXCore::Utils::vl64::Encode(data, static_cast<int64_t>(std::numeric_limits<int32_t>::max()) + 1) == 9);
  CHECK(data[0] == 0b1110'0000);
  memcpy(&result, &data[1], sizeof(int64_t));
  CHECK(result == static_cast<int64_t>(std::numeric_limits<int32_t>::max()) + 1);
  Dec = FEXCore::Utils::vl64::Decode(data);
  CHECK(Dec.Size == 9);
  CHECK(Dec.Integer == static_cast<int64_t>(std::numeric_limits<int32_t>::max()) + 1);

  REQUIRE(FEXCore::Utils::vl64::Encode(data, std::numeric_limits<int64_t>::min()) == 9);
  CHECK(data[0] == 0b1110'0000);
  memcpy(&result, &data[1], sizeof(int64_t));
  CHECK(result == std::numeric_limits<int64_t>::min());
  Dec = FEXCore::Utils::vl64::Decode(data);
  CHECK(Dec.Size == 9);
  CHECK(Dec.Integer == std::numeric_limits<int64_t>::min());

  REQUIRE(FEXCore::Utils::vl64::Encode(data, std::numeric_limits<int64_t>::max()) == 9);
  CHECK(data[0] == 0b1110'0000);
  memcpy(&result, &data[1], sizeof(int64_t));
  CHECK(result == std::numeric_limits<int64_t>::max());
  Dec = FEXCore::Utils::vl64::Decode(data);
  CHECK(Dec.Size == 9);
  CHECK(Dec.Integer == std::numeric_limits<int64_t>::max());
}

TEST_CASE("vl64pair-size") {
  // Check 8-bit minimum and maximum.
  CHECK(FEXCore::Utils::vl64pair::EncodedSize(4, 1) == 1);
  CHECK(FEXCore::Utils::vl64pair::EncodedSize(64, 8) == 1);

  // Interlaced 8-bit minimum and maximum
  CHECK(FEXCore::Utils::vl64pair::EncodedSize(4, 8) == 1);
  CHECK(FEXCore::Utils::vl64pair::EncodedSize(64, 1) == 1);

  // Check 16-bit minimum and maximum.
  CHECK(FEXCore::Utils::vl64pair::EncodedSize(-512, -32) == 2);
  CHECK(FEXCore::Utils::vl64pair::EncodedSize(504, 31) == 2);

  // Interlaced 16-bit minimum and maximum.
  CHECK(FEXCore::Utils::vl64pair::EncodedSize(-512, 31) == 2);
  CHECK(FEXCore::Utils::vl64pair::EncodedSize(504, -32) == 2);

  // Check 32-bit minimum and maximum.
  CHECK(FEXCore::Utils::vl64pair::EncodedSize(std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min()) == 9);
  CHECK(FEXCore::Utils::vl64pair::EncodedSize(std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max()) == 9);

  // Interlaced 32-bit minimum and maximum.
  CHECK(FEXCore::Utils::vl64pair::EncodedSize(std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max()) == 9);
  CHECK(FEXCore::Utils::vl64pair::EncodedSize(std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::min()) == 9);

  // Check 64-bit minimum and maximum.
  CHECK(FEXCore::Utils::vl64pair::EncodedSize(std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::min()) == 17);
  CHECK(FEXCore::Utils::vl64pair::EncodedSize(std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max()) == 17);

  // Interlaced 64-bit minimum and maximum.
  CHECK(FEXCore::Utils::vl64pair::EncodedSize(std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max()) == 17);
  CHECK(FEXCore::Utils::vl64pair::EncodedSize(std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::min()) == 17);
}

TEST_CASE("vl8pair - in memory - encode/decode") {
  uint8_t data[1];
  // Minimum, Minimum
  REQUIRE(FEXCore::Utils::vl64pair::Encode(data, (1 * 4), 1) == 1);
  CHECK(data[0] == 0);
  auto Dec = FEXCore::Utils::vl64pair::Decode(data);
  CHECK(Dec.Size == 1);
  CHECK(Dec.IntegerARMPC == (1 * 4));
  CHECK(Dec.IntegerX86RIP == 1);

  // Maximum, Maximum
  REQUIRE(FEXCore::Utils::vl64pair::Encode(data, (16 * 4), 8) == 1);
  CHECK(data[0] == 0b0111'1111);
  Dec = FEXCore::Utils::vl64pair::Decode(data);
  CHECK(Dec.Size == 1);
  CHECK(Dec.IntegerARMPC == (16 * 4));
  CHECK(Dec.IntegerX86RIP == 8);

  // Minimum, Maximum
  REQUIRE(FEXCore::Utils::vl64pair::Encode(data, (1 * 4), 8) == 1);
  CHECK(data[0] == 0b0111'0000);
  Dec = FEXCore::Utils::vl64pair::Decode(data);
  CHECK(Dec.Size == 1);
  CHECK(Dec.IntegerARMPC == (1 * 4));
  CHECK(Dec.IntegerX86RIP == 8);

  // Maximum, Minimum
  REQUIRE(FEXCore::Utils::vl64pair::Encode(data, (16 * 4), 1) == 1);
  CHECK(data[0] == 0b0000'1111);
  Dec = FEXCore::Utils::vl64pair::Decode(data);
  CHECK(Dec.Size == 1);
  CHECK(Dec.IntegerARMPC == (16 * 4));
  CHECK(Dec.IntegerX86RIP == 1);
}

TEST_CASE("vl16pair - in memory - encode/decode") {
  uint8_t data[2];

  // vl8pair Minimum - 1, Minimum - 1
  REQUIRE(FEXCore::Utils::vl64pair::Encode(data, 0, 0) == 2);
  CHECK((uint64_t)data[0] == 0b1000'0000);
  CHECK((uint64_t)data[1] == 0b0000'0000);
  auto Dec = FEXCore::Utils::vl64pair::Decode(data);
  CHECK(Dec.Size == 2);
  CHECK(Dec.IntegerARMPC == 0);
  CHECK(Dec.IntegerX86RIP == 0);

  // vl8pair Maximum + 1, Maximum + 1
  REQUIRE(FEXCore::Utils::vl64pair::Encode(data, (17 * 4), 9) == 2);
  CHECK((uint64_t)data[0] == 0b1000'1001);
  CHECK((uint64_t)data[1] == 0b0001'0001);
  Dec = FEXCore::Utils::vl64pair::Decode(data);
  CHECK(Dec.Size == 2);
  CHECK(Dec.IntegerARMPC == (17 * 4));
  CHECK(Dec.IntegerX86RIP == 9);

  // Minimum, Minimum
  REQUIRE(FEXCore::Utils::vl64pair::Encode(data, (-128 * 4), -32) == 2);
  CHECK((uint64_t)data[0] == 0b1010'0000);
  CHECK((uint64_t)data[1] == 0b1000'0000);
  Dec = FEXCore::Utils::vl64pair::Decode(data);
  CHECK(Dec.Size == 2);
  CHECK(Dec.IntegerARMPC == (-128 * 4));
  CHECK(Dec.IntegerX86RIP == -32);

  // Maximum, Maximum
  REQUIRE(FEXCore::Utils::vl64pair::Encode(data, (127 * 4), 31) == 2);
  CHECK((uint64_t)data[0] == 0b1001'1111);
  CHECK((uint64_t)data[1] == 0b0111'1111);
  Dec = FEXCore::Utils::vl64pair::Decode(data);
  CHECK(Dec.Size == 2);
  CHECK(Dec.IntegerARMPC == (127 * 4));
  CHECK(Dec.IntegerX86RIP == 31);

  // Interleaved Minimum, Maximum
  REQUIRE(FEXCore::Utils::vl64pair::Encode(data, (-128 * 4), 31) == 2);
  CHECK((uint64_t)data[0] == 0b1001'1111);
  CHECK((uint64_t)data[1] == 0b1000'0000);
  Dec = FEXCore::Utils::vl64pair::Decode(data);
  CHECK(Dec.Size == 2);
  CHECK(Dec.IntegerARMPC == (-128 * 4));
  CHECK(Dec.IntegerX86RIP == 31);

  // Interleaved Maximum, Minimum
  REQUIRE(FEXCore::Utils::vl64pair::Encode(data, (127 * 4), -32) == 2);
  CHECK((uint64_t)data[0] == 0b1010'0000);
  CHECK((uint64_t)data[1] == 0b0111'1111);
  Dec = FEXCore::Utils::vl64pair::Decode(data);
  CHECK(Dec.Size == 2);
  CHECK(Dec.IntegerARMPC == (127 * 4));
  CHECK(Dec.IntegerX86RIP == -32);
}

TEST_CASE("vl32pair - in memory - encode/decode") {
  uint8_t data[9];
  int32_t result {};

  // Minimum, Minimum
  REQUIRE(FEXCore::Utils::vl64pair::Encode(data, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min()) == 9);
  CHECK(data[0] == 0b1100'0000);
  memcpy(&result, &data[1], sizeof(int32_t));
  CHECK(result == std::numeric_limits<int32_t>::min());
  memcpy(&result, &data[1 + sizeof(int32_t)], sizeof(int32_t));
  CHECK(result == std::numeric_limits<int32_t>::min());
  auto Dec = FEXCore::Utils::vl64pair::Decode(data);
  CHECK(Dec.Size == 9);
  CHECK(Dec.IntegerARMPC == std::numeric_limits<int32_t>::min());
  CHECK(Dec.IntegerX86RIP == std::numeric_limits<int32_t>::min());

  // Maximum, Maximum
  REQUIRE(FEXCore::Utils::vl64pair::Encode(data, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max()) == 9);
  CHECK(data[0] == 0b1100'0000);
  memcpy(&result, &data[1], sizeof(int32_t));
  CHECK(result == std::numeric_limits<int32_t>::max());
  memcpy(&result, &data[1 + sizeof(int32_t)], sizeof(int32_t));
  CHECK(result == std::numeric_limits<int32_t>::max());
  Dec = FEXCore::Utils::vl64pair::Decode(data);
  CHECK(Dec.Size == 9);
  CHECK(Dec.IntegerARMPC == std::numeric_limits<int32_t>::max());
  CHECK(Dec.IntegerX86RIP == std::numeric_limits<int32_t>::max());

  // Interleaved Minimum, Maximum
  REQUIRE(FEXCore::Utils::vl64pair::Encode(data, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max()) == 9);
  CHECK(data[0] == 0b1100'0000);
  memcpy(&result, &data[1], sizeof(int32_t));
  CHECK(result == std::numeric_limits<int32_t>::min());
  memcpy(&result, &data[1 + sizeof(int32_t)], sizeof(int32_t));
  CHECK(result == std::numeric_limits<int32_t>::max());
  Dec = FEXCore::Utils::vl64pair::Decode(data);
  CHECK(Dec.Size == 9);
  CHECK(Dec.IntegerARMPC == std::numeric_limits<int32_t>::min());
  CHECK(Dec.IntegerX86RIP == std::numeric_limits<int32_t>::max());

  // Interleaved Maximum, Minimum
  REQUIRE(FEXCore::Utils::vl64pair::Encode(data, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::min()) == 9);
  CHECK(data[0] == 0b1100'0000);
  memcpy(&result, &data[1], sizeof(int32_t));
  CHECK(result == std::numeric_limits<int32_t>::max());
  memcpy(&result, &data[1 + sizeof(int32_t)], sizeof(int32_t));
  CHECK(result == std::numeric_limits<int32_t>::min());
  Dec = FEXCore::Utils::vl64pair::Decode(data);
  CHECK(Dec.Size == 9);
  CHECK(Dec.IntegerARMPC == std::numeric_limits<int32_t>::max());
  CHECK(Dec.IntegerX86RIP == std::numeric_limits<int32_t>::min());
}

TEST_CASE("vl64pair - in memory - encode/decode") {
  uint8_t data[17];
  int64_t result {};

  // Minimum, Minimum
  REQUIRE(FEXCore::Utils::vl64pair::Encode(data, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::min()) == 17);
  CHECK(data[0] == 0b1110'0000);
  memcpy(&result, &data[1], sizeof(int64_t));
  CHECK(result == std::numeric_limits<int64_t>::min());
  memcpy(&result, &data[1 + sizeof(int64_t)], sizeof(int64_t));
  CHECK(result == std::numeric_limits<int64_t>::min());
  auto Dec = FEXCore::Utils::vl64pair::Decode(data);
  CHECK(Dec.Size == 17);
  CHECK(Dec.IntegerARMPC == std::numeric_limits<int64_t>::min());
  CHECK(Dec.IntegerX86RIP == std::numeric_limits<int64_t>::min());

  // Maximum, Maximum
  REQUIRE(FEXCore::Utils::vl64pair::Encode(data, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max()) == 17);
  CHECK(data[0] == 0b1110'0000);
  memcpy(&result, &data[1], sizeof(int64_t));
  CHECK(result == std::numeric_limits<int64_t>::max());
  memcpy(&result, &data[1 + sizeof(int64_t)], sizeof(int64_t));
  CHECK(result == std::numeric_limits<int64_t>::max());
  Dec = FEXCore::Utils::vl64pair::Decode(data);
  CHECK(Dec.Size == 17);
  CHECK(Dec.IntegerARMPC == std::numeric_limits<int64_t>::max());
  CHECK(Dec.IntegerX86RIP == std::numeric_limits<int64_t>::max());

  // Interleaved Minimum, Maximum
  REQUIRE(FEXCore::Utils::vl64pair::Encode(data, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max()) == 17);
  CHECK(data[0] == 0b1110'0000);
  memcpy(&result, &data[1], sizeof(int64_t));
  CHECK(result == std::numeric_limits<int64_t>::min());
  memcpy(&result, &data[1 + sizeof(int64_t)], sizeof(int64_t));
  CHECK(result == std::numeric_limits<int64_t>::max());
  Dec = FEXCore::Utils::vl64pair::Decode(data);
  CHECK(Dec.Size == 17);
  CHECK(Dec.IntegerARMPC == std::numeric_limits<int64_t>::min());
  CHECK(Dec.IntegerX86RIP == std::numeric_limits<int64_t>::max());

  // Interleaved Maximum, Minimum
  REQUIRE(FEXCore::Utils::vl64pair::Encode(data, std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::min()) == 17);
  CHECK(data[0] == 0b1110'0000);
  memcpy(&result, &data[1], sizeof(int64_t));
  CHECK(result == std::numeric_limits<int64_t>::max());
  memcpy(&result, &data[1 + sizeof(int64_t)], sizeof(int64_t));
  CHECK(result == std::numeric_limits<int64_t>::min());
  Dec = FEXCore::Utils::vl64pair::Decode(data);
  CHECK(Dec.Size == 17);
  CHECK(Dec.IntegerARMPC == std::numeric_limits<int64_t>::max());
  CHECK(Dec.IntegerX86RIP == std::numeric_limits<int64_t>::min());
}
