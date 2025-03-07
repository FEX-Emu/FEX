// SPDX-License-Identifier: MIT
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include "Utils/Allocator/FlexBitSet.h"

TEST_CASE("FlexBitSet - Sizing") {
  // Ensure that FlexBitSet sizing is correct.

  // Size of zero shouldn't take any space.
  CHECK(FEXCore::FlexBitSet<uint8_t>::SizeInBytes(0) == 0);
  CHECK(FEXCore::FlexBitSet<uint16_t>::SizeInBytes(0) == 0);
  CHECK(FEXCore::FlexBitSet<uint32_t>::SizeInBytes(0) == 0);
  CHECK(FEXCore::FlexBitSet<uint64_t>::SizeInBytes(0) == 0);

  CHECK(FEXCore::FlexBitSet<uint8_t>::SizeInBits(0) == 0);
  CHECK(FEXCore::FlexBitSet<uint16_t>::SizeInBits(0) == 0);
  CHECK(FEXCore::FlexBitSet<uint32_t>::SizeInBits(0) == 0);
  CHECK(FEXCore::FlexBitSet<uint64_t>::SizeInBits(0) == 0);

  // Size of 1 should take one sizeof(ElementSize) size
  CHECK(FEXCore::FlexBitSet<uint8_t>::SizeInBytes(1) == sizeof(uint8_t));
  CHECK(FEXCore::FlexBitSet<uint16_t>::SizeInBytes(1) == sizeof(uint16_t));
  CHECK(FEXCore::FlexBitSet<uint32_t>::SizeInBytes(1) == sizeof(uint32_t));
  CHECK(FEXCore::FlexBitSet<uint64_t>::SizeInBytes(1) == sizeof(uint64_t));

  CHECK(FEXCore::FlexBitSet<uint8_t>::SizeInBits(1) == sizeof(uint8_t) * 8);
  CHECK(FEXCore::FlexBitSet<uint16_t>::SizeInBits(1) == sizeof(uint16_t) * 8);
  CHECK(FEXCore::FlexBitSet<uint32_t>::SizeInBits(1) == sizeof(uint32_t) * 8);
  CHECK(FEXCore::FlexBitSet<uint64_t>::SizeInBits(1) == sizeof(uint64_t) * 8);

  // Size of `sizeof(ElementSize) * 8` should take one sizeof(ElementSize) size
  CHECK(FEXCore::FlexBitSet<uint8_t>::SizeInBytes(sizeof(uint8_t) * 8) == sizeof(uint8_t));
  CHECK(FEXCore::FlexBitSet<uint16_t>::SizeInBytes(sizeof(uint16_t) * 8) == sizeof(uint16_t));
  CHECK(FEXCore::FlexBitSet<uint32_t>::SizeInBytes(sizeof(uint32_t) * 8) == sizeof(uint32_t));
  CHECK(FEXCore::FlexBitSet<uint64_t>::SizeInBytes(sizeof(uint64_t) * 8) == sizeof(uint64_t));

  CHECK(FEXCore::FlexBitSet<uint8_t>::SizeInBits(sizeof(uint8_t) * 8) == sizeof(uint8_t) * 8);
  CHECK(FEXCore::FlexBitSet<uint16_t>::SizeInBits(sizeof(uint16_t) * 8) == sizeof(uint16_t) * 8);
  CHECK(FEXCore::FlexBitSet<uint32_t>::SizeInBits(sizeof(uint32_t) * 8) == sizeof(uint32_t) * 8);
  CHECK(FEXCore::FlexBitSet<uint64_t>::SizeInBits(sizeof(uint64_t) * 8) == sizeof(uint64_t) * 8);
}
