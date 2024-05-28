#include <catch2/catch_test_macros.hpp>

#include <filesystem>

TEST_CASE("proc-self symlink") {
  // Saw with the Darwinia Linux game port.
  // It sanity checks that `/proc/self/exe` is a symlink and also that it points to a regular file.
  // This uses newfsstatat or statx behind the scenes which FEX didn't handle this edge-case correctly.

  // Create a path with /proc/self/exe
  std::filesystem::path path {"/proc/self/exe"};

  // Check the status of the file with status first.
  std::error_code ec;
  auto status = std::filesystem::status(path, ec);

  // No error
  REQUIRE(!ec);
  CHECK(status.type() == std::filesystem::file_type::regular);

  // Now check the status with symlink_status.
  status = std::filesystem::symlink_status(path, ec);

  // No error
  REQUIRE(!ec);
  CHECK(status.type() == std::filesystem::file_type::symlink);

  // The game would then continue to read std::filesystem::read_symlink.
}
