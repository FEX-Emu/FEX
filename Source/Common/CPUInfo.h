// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/Utils/CompilerDefs.h>

#include <cstdint>

namespace FEX::CPUInfo {
/**
 * @brief Calculate the number of CPUs in the system regardless of affinity mask.
 *
 * @return The number of CPUs in the system.
 */
uint32_t CalculateNumberOfCPUs();
} // namespace FEX::CPUInfo
