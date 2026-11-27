#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <string>
#include <string_view>

// Measure of VmData usage from FEX overhead. This can be thought of
// as a sibling to the test "rlimit_data_pthread."
#if defined(__x86_64__)
constexpr size_t HIGH_WATER_MARK = 4ULL * 1024 * 1024;
#elif defined(__i386__)
constexpr size_t HIGH_WATER_MARK = 8ULL * 1024 * 1024;
#endif

TEST_CASE("VmData charge at startup") {
  std::ifstream fs {"/proc/self/status", std::fstream::binary};
  std::string Line;

  constexpr std::string_view Prefix = "VmData:";
  size_t VmData = 0;
  while (std::getline(fs, Line)) {
    if (Line.starts_with(Prefix)) {
      VmData = std::stoull(Line.substr(Prefix.size()));
      break;
    }
  }

  REQUIRE(VmData != 0);
  CHECK(VmData <= HIGH_WATER_MARK);
}
