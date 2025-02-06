// SPDX-License-Identifier: MIT
#pragma once
#include "Common/Config.h"

namespace FEX {
static inline FEX::Config::PortableInformation ReadPortabilityInformation() {
  const FEX::Config::PortableInformation BadResult {false, {}};
  const char* PortableConfig = getenv("FEX_PORTABLE");
  if (!PortableConfig || strtol(PortableConfig, nullptr, 0) == 0) {
    return BadResult;
  }

  return {true, getenv("LOCALAPPDATA")};
}
} // namespace FEX
