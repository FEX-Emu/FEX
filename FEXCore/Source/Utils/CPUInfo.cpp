// SPDX-License-Identifier: MIT
#include <FEXHeaderUtils/Filesystem.h>

#include <fmt/format.h>

#include <atomic>
#include <charconv>
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
  static std::atomic<uint32_t> NumCPUs {};
  if (NumCPUs.load(std::memory_order_relaxed)) {
    return NumCPUs.load(std::memory_order_relaxed);
  }

  // Support up to 8 digits of CPU cores plus the null terminator.
  constexpr std::string_view CPUString = "/sys/devices/system/cpu/cpu";
  char Tmp[CPUString.size() + 1 + 8];
  std::ranges::copy(CPUString, Tmp);
  size_t CPUs = 1;

  for (;; ++CPUs) {
    auto Res = std::to_chars(&Tmp[CPUString.size()], &Tmp[sizeof(Tmp)], CPUs);
    // null terminate the string.
    Res.ptr[0] = '\0';

    if (!FHU::Filesystem::Exists(Tmp)) {
      NumCPUs.store(CPUs, std::memory_order_relaxed);
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
