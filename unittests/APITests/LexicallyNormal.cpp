#include <catch2/catch.hpp>
#include <filesystem>
#include <FEXHeaderUtils/Filesystem.h>

#define TestPath(Path) \
  REQUIRE(std::string_view(FHU::Filesystem::LexicallyNormal(Path)) == std::string_view(std::filesystem::path(Path).lexically_normal().string()));

TEST_CASE("LexicallyNormal") {
  TestPath("");
  TestPath("/");
  TestPath("/./");
  TestPath("//.");
  TestPath("//./");
  TestPath("//.//");

  TestPath(".");
  TestPath("..");
  TestPath(".//");
  TestPath("../../");
  TestPath("././");
  TestPath("./../");
  TestPath("./../");
  TestPath("./.././.././.");
  TestPath("./.././.././..");

  TestPath("./foo/../");
  TestPath("foo/./bar/..");
  TestPath("foo/.///bar/..");
  TestPath("foo/.///bar/../");
}
