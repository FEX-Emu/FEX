#include <FEXHeaderUtils/Filesystem.h>

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
    char Tmp[PATH_MAX];
    size_t CPUs = 1;

    for (;; ++CPUs) {
      auto Size = fmt::format_to_n(Tmp, sizeof(Tmp), "/sys/devices/system/cpu/cpu{}", CPUs);
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
}
