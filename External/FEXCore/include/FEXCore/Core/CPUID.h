#pragma once
#include <cstdint>

namespace FEXCore::CPUID {
  struct FunctionResults {
    uint32_t eax, ebx, ecx, edx;
  };
}

