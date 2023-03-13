#pragma once
#include <FEXCore/Utils/CompilerDefs.h>

#include <cstdint>

namespace FEXCore::CPUInfo {
  /**
   * @brief Calculate the number of CPUs in the system regardless of affinity mask.
   *
   * @return The number of CPUs in the system.
   */
  FEX_DEFAULT_VISIBILITY uint32_t CalculateNumberOfCPUs();
}
