// SPDX-License-Identifier: MIT
#include <FEXCore/Utils/FileLoading.h>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("LoadFile-Doesn'tExist") {
  fextl::string MapsFile;
  auto Read = FEXCore::FileLoading::LoadFile(MapsFile, "/tmp/a/b/c/d/e/z");
  REQUIRE(MapsFile.size() == 0);
  REQUIRE(Read == false);
}

TEST_CASE("LoadFile-procfs") {
  fextl::string MapsFile;
  FEXCore::FileLoading::LoadFile(MapsFile, "/proc/self/maps");
  REQUIRE(MapsFile.size() != 0);
}

TEST_CASE("LoadFile-Buffer") {
  fextl::string MapsFile;
  MapsFile.resize(16);
  auto Read = FEXCore::FileLoading::LoadFileToBuffer("/proc/self/maps", MapsFile);
  REQUIRE(MapsFile.size() == Read);
}
