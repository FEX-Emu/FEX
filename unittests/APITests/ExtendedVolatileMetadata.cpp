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

TEST_CASE("TSO disabled ranges - offsets") {
  const auto Result = FEX::VolatileMetadata::ParseExtendedVolatileMetadata("hl2_linux;1000-1800,3000-3010,1fff0-20010");
  FEXCore::IntervalList<uint64_t> Ranges {};
  FEX::VolatileMetadata::ApplyFEXExtendedTSODisabledRanges(Result.at("hl2_linux"), Ranges, 0x7ff800000000, 0x7ff800020000);

  CHECK(Ranges.Contains({0x7ff800001000, 0x7ff800001800}));
  CHECK(Ranges.Contains({0x7ff800003000, 0x7ff800003010}));
  // A block crossing a range boundary is not covered.
  CHECK_FALSE(Ranges.Contains({0x7ff8000017f0, 0x7ff800001810}));
  CHECK_FALSE(Ranges.Contains({0x7ff800000ff0, 0x7ff800001010}));
  CHECK_FALSE(Ranges.Contains({0x7ff800002000, 0x7ff800002010}));
  // A range past the end of the module is dropped.
  CHECK_FALSE(Ranges.Intersect({0x7ff80001fff0, 0x7ff800020000}));
}

TEST_CASE("TSO disabled ranges - whole module") {
  const auto Result = FEX::VolatileMetadata::ParseExtendedVolatileMetadata("hl2_linux");
  FEXCore::IntervalList<uint64_t> Ranges {};
  FEX::VolatileMetadata::ApplyFEXExtendedTSODisabledRanges(Result.at("hl2_linux"), Ranges, 0x180000000, 0x181e65000);

  CHECK(Ranges.Contains({0x180000000, 0x181e65000}));
  CHECK_FALSE(Ranges.Contains({0x181e64ff0, 0x181e65010}));
}

TEST_CASE("TSO disabled ranges - instructions keep their volatile metadata form") {
  const auto Result = FEX::VolatileMetadata::ParseExtendedVolatileMetadata("hl2_linux;1000-2000;1010");
  fextl::set<uint64_t> Instructions {};
  FEXCore::IntervalList<uint64_t> ValidRanges {};
  FEXCore::IntervalList<uint64_t> Ranges {};
  auto Entry = Result.at("hl2_linux");
  FEX::VolatileMetadata::ApplyFEXExtendedVolatileMetadata(Entry, Instructions, ValidRanges, 0x10000, 0x20000);
  FEX::VolatileMetadata::ApplyFEXExtendedTSODisabledRanges(Entry, Ranges, 0x10000, 0x20000);

  CHECK(Instructions.contains(0x11010));
  CHECK(ValidRanges.Contains({0x11000, 0x12000}));
  CHECK(Ranges.Contains({0x11000, 0x12000}));
}
