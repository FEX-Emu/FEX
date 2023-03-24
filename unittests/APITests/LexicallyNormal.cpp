#include <catch2/catch.hpp>
#include <filesystem>
#include <FEXHeaderUtils/Filesystem.h>

#define TestPath(Path) \

TEST_CASE("LexicallyNormal") {
  auto Path = GENERATE("",
  "/",
  "/./",
  "//.",
  "//./",
  "//.//",

  ".",
  "..",
  ".//",
  "../../",
  "././",
  "./../",
  "./../",
  "./.././.././.",
  "./.././.././..",

  "./foo/../",
  "foo/./bar/..",
  "foo/.///bar/..",
  "foo/.///bar/../");

  REQUIRE(std::string_view(FHU::Filesystem::LexicallyNormal(Path)) == std::string_view(std::filesystem::path(Path).lexically_normal().string()));
}
