// SPDX-License-Identifier: MIT
#pragma once

#include <windef.h>
#include <winternl.h>

namespace FEXCore::Context {
  class Context;
}

/**
 * @brief Maps CPUID results to Windows CPU info structures
 */
namespace FEX::Windows {
class CPUFeatures {
public:
  CPUFeatures(FEXCore::Context::Context &CTX);

  /**
   * @brief If the given PF_* feature is supported  
   */
  bool IsFeaturePresent(uint32_t Feature);

  /**
   * @brief Fills in `Info` according to the detected CPU features
   */
  void UpdateInformation(SYSTEM_CPU_INFORMATION *Info);

private:
  SYSTEM_CPU_INFORMATION CpuInfo{};
};
}
