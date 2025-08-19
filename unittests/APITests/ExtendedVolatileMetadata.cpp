// SPDX-License-Identifier: MIT
#include <catch2/catch_all.hpp>
#include "Common/VolatileMetadata.h"

TEST_CASE("Basic - Empty") {
  const auto String = "";
  const auto Result = FEX::VolatileMetadata::ParseExtendedVolatileMetadata(String);
  REQUIRE(Result.empty());
}

TEST_CASE("Basic - Empty - modules") {
  const auto String = ":::::";
  const auto Result = FEX::VolatileMetadata::ParseExtendedVolatileMetadata(String);
  REQUIRE(Result.empty());
}

TEST_CASE("Basic - Single") {
  const auto String = "hl2_linux";
  const auto Result = FEX::VolatileMetadata::ParseExtendedVolatileMetadata(String);
  REQUIRE(Result.size() == 1);

  REQUIRE(Result.contains("hl2_linux"));
  CHECK(Result.at("hl2_linux").ModuleTSODisabled == true);
  CHECK(Result.at("hl2_linux").VolatileInstructions.empty());
  CHECK(Result.at("hl2_linux").VolatileValidRanges.Empty());
}

TEST_CASE("Basic - Multiple") {
  const auto String = "hl2_linux:DeckJob";
  const auto Result = FEX::VolatileMetadata::ParseExtendedVolatileMetadata(String);
  REQUIRE(Result.size() == 2);

  REQUIRE(Result.contains("hl2_linux"));
  CHECK(Result.at("hl2_linux").ModuleTSODisabled == true);
  CHECK(Result.at("hl2_linux").VolatileInstructions.empty());
  CHECK(Result.at("hl2_linux").VolatileValidRanges.Empty());

  REQUIRE(Result.contains("DeckJob"));
  CHECK(Result.at("DeckJob").ModuleTSODisabled == true);
  CHECK(Result.at("DeckJob").VolatileInstructions.empty());
  CHECK(Result.at("DeckJob").VolatileValidRanges.Empty());
}

TEST_CASE("Basic - Single plus empty") {
  const auto String = "hl2_linux:::::";
  const auto Result = FEX::VolatileMetadata::ParseExtendedVolatileMetadata(String);
  REQUIRE(Result.size() == 1);
  REQUIRE(Result.contains("hl2_linux"));
  CHECK(Result.at("hl2_linux").ModuleTSODisabled == true);
  CHECK(Result.at("hl2_linux").VolatileInstructions.empty());
  CHECK(Result.at("hl2_linux").VolatileValidRanges.Empty());
}

static inline bool ContainsRange(std::pair<uint64_t, uint64_t> Range, const std::vector<std::pair<uint64_t, uint64_t>>& ValidRanges) {
  return std::ranges::find(ValidRanges, Range) != ValidRanges.end();
}

TEST_CASE("Basic - Single - offset") {
  const auto String = "hl2_linux;0x0-0x1000";
  const auto Result = FEX::VolatileMetadata::ParseExtendedVolatileMetadata(String);
  REQUIRE(Result.size() == 1);

  REQUIRE(Result.contains("hl2_linux"));
  CHECK(Result.at("hl2_linux").ModuleTSODisabled == false);
  CHECK(Result.at("hl2_linux").VolatileInstructions.empty());
  CHECK(Result.at("hl2_linux").VolatileValidRanges.Empty() == false);

  const std::vector<std::pair<uint64_t, uint64_t>> ValidRanges = {
    {0, 0x1000},
  };

  for (auto it : Result.at("hl2_linux").VolatileValidRanges) {
    CHECK(ContainsRange(std::make_pair(it.Offset, it.End), ValidRanges));
  }
}

TEST_CASE("Basic - Single - offset x2") {
  const auto String = "hl2_linux;0x0-0x1000,0x2000-0x3000";
  const auto Result = FEX::VolatileMetadata::ParseExtendedVolatileMetadata(String);
  REQUIRE(Result.size() == 1);
  REQUIRE(Result.contains("hl2_linux"));
  CHECK(Result.at("hl2_linux").ModuleTSODisabled == false);
  CHECK(Result.at("hl2_linux").VolatileInstructions.empty());
  CHECK(Result.at("hl2_linux").VolatileValidRanges.Empty() == false);

  const std::vector<std::pair<uint64_t, uint64_t>> ValidRanges = {
    {0, 0x1000},
    {0x2000, 0x3000},
  };

  for (auto it : Result.at("hl2_linux").VolatileValidRanges) {
    CHECK(ContainsRange(std::make_pair(it.Offset, it.End), ValidRanges));
  }
}

TEST_CASE("Basic - Single - offset plus instruction") {
  const auto String = "hl2_linux;0x0-0x1000;0x1,0x2,0x3";
  const auto Result = FEX::VolatileMetadata::ParseExtendedVolatileMetadata(String);
  REQUIRE(Result.size() == 1);
  REQUIRE(Result.contains("hl2_linux"));
  CHECK(Result.at("hl2_linux").ModuleTSODisabled == false);
  CHECK(Result.at("hl2_linux").VolatileInstructions.empty() == false);
  CHECK(Result.at("hl2_linux").VolatileValidRanges.Empty() == false);

  const std::vector<std::pair<uint64_t, uint64_t>> ValidRanges = {
    {0, 0x1000},
  };

  const std::vector<uint64_t> ValidInsts = {
    1,
    2,
    3,
  };

  for (auto it : Result.at("hl2_linux").VolatileValidRanges) {
    CHECK(ContainsRange(std::make_pair(it.Offset, it.End), ValidRanges));
  }

  for (auto it : Result.at("hl2_linux").VolatileInstructions) {
    CHECK_THAT(ValidInsts, Catch::Matchers::Contains(it));
  }
}

TEST_CASE("Basic - Double - offset") {
  const auto String = "hl2_linux;0x0-0x1000:DeckJob;0x2000-0x3000";
  const auto Result = FEX::VolatileMetadata::ParseExtendedVolatileMetadata(String);
  REQUIRE(Result.size() == 2);

  {
    REQUIRE(Result.contains("hl2_linux"));
    CHECK(Result.at("hl2_linux").ModuleTSODisabled == false);
    CHECK(Result.at("hl2_linux").VolatileInstructions.empty());
    CHECK(Result.at("hl2_linux").VolatileValidRanges.Empty() == false);

    const std::vector<std::pair<uint64_t, uint64_t>> ValidRanges = {
      {0, 0x1000},
    };

    for (auto it : Result.at("hl2_linux").VolatileValidRanges) {
      CHECK(ContainsRange(std::make_pair(it.Offset, it.End), ValidRanges));
    }
  }

  {
    REQUIRE(Result.contains("DeckJob"));
    CHECK(Result.at("DeckJob").ModuleTSODisabled == false);
    CHECK(Result.at("DeckJob").VolatileInstructions.empty());
    CHECK(Result.at("DeckJob").VolatileValidRanges.Empty() == false);

    const std::vector<std::pair<uint64_t, uint64_t>> ValidRanges = {
      {0x2000, 0x3000},
    };

    for (auto it : Result.at("DeckJob").VolatileValidRanges) {
      CHECK(ContainsRange(std::make_pair(it.Offset, it.End), ValidRanges));
    }
  }
}
