// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/Core/HostFeatures.h>
#include "aarch64/cpu-aarch64.h"

namespace FEX {
FEXCore::HostFeatures FetchHostFeatures(vixl::CPUFeatures Features, bool SupportsCacheMaintenanceOps, uint64_t CTR, uint64_t MIDR);
FEXCore::HostFeatures FetchHostFeatures();
}
