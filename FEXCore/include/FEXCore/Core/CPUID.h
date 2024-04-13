// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>

namespace FEXCore::CPUID {
struct FunctionResults {
  uint32_t eax, ebx, ecx, edx;
};

struct XCRResults {
  uint32_t eax, edx;
};
} // namespace FEXCore::CPUID
