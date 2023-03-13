#include <FEXHeaderUtils/Filesystem.h>

#include <fmt/format.h>

#include <cstddef>
#include <cstdint>
#include <linux/limits.h>

namespace FEXCore::CPUInfo {
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
}
