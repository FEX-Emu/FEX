#include <catch2/catch.hpp>
#include <filesystem>
#include <FEXHeaderUtils/Filesystem.h>

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

    "./foo1/../",
    "foo4/.///bar/../",
    "foo5/././",

    "foo6/",
    "foo7/test",
    "foo8/test/",
    "foo9/./../test/",

    "/../..",
    "...",
    "/...",
    "foo10/...",
    "/..",
    "/foo11/../../bar"
  );

  REQUIRE(std::string_view(FHU::Filesystem::LexicallyNormal(Path)) == std::string_view(std::filesystem::path(Path).lexically_normal().string()));
}

TEST_CASE("LexicallyNormalDifferences", "[!shouldfail]") {
  auto Path = GENERATE("",
    // std::fs here keeps the `/` after `foo2/`
    // FEX algorithm doesn't keep behaviour here.
    "foo2/./bar/..",   // std::fs -> "foo2/"
    "foo2/.///bar/.." // std::fs -> "foo3/"
  );

  REQUIRE(std::string_view(FHU::Filesystem::LexicallyNormal(Path)) == std::string_view(std::filesystem::path(Path).lexically_normal().string()));
}

TEST_CASE("ParentPath") {
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
    "foo/.///bar/../",
    "foo/././"
    "...",
    "/...",
    "foo/...",
    "/..",
    "/foo/../../bar"
  );

  REQUIRE(std::string_view(FHU::Filesystem::ParentPath(Path)) == std::string_view(std::filesystem::path(Path).parent_path().string()));
}
