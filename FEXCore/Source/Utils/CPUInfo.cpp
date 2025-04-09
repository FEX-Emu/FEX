// SPDX-License-Identifier: MIT
#include <FEXHeaderUtils/Filesystem.h>

#include <fmt/compile.h>
#include <fmt/format.h>

#include <cstddef>
#include <cstdint>
#ifdef _WIN32
#include <thread>
#else
#include <linux/limits.h>
#endif

namespace FEXCore::CPUInfo {
#ifndef _WIN32
uint32_t CalculateNumberOfCPUs() {
  constexpr auto parse_string = FMT_COMPILE("/sys/devices/system/cpu/cpu{}");
  constexpr auto max_parse_size = ::fmt::formatted_size(parse_string, UINT32_MAX);
  char Tmp[max_parse_size];
  size_t CPUs = 1;

  for (;; ++CPUs) {
    auto Size = fmt::format_to_n(Tmp, max_parse_size, parse_string, CPUs);
    Tmp[Size.size] = 0;
    if (!FHU::Filesystem::Exists(Tmp)) {
      break;
    }
  }

  return CPUs;
}
#else
uint32_t CalculateNumberOfCPUs() {
  // May not return correct number of cores if some are parked.
  return std::thread::hardware_concurrency();
}
#endif
} // namespace FEXCore::CPUInfo
