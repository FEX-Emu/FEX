// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Config/Config.h>
#include <FEXCore/Utils/ArchHelpers/Arm64.h>

namespace FEX::Windows {
class TSOHandlerConfig final {
public:
  TSOHandlerConfig() {
    if (ParanoidTSO()) {
      UnalignedHandlerType = FEXCore::ArchHelpers::Arm64::UnalignedHandlerType::Paranoid;
    } else if (HalfBarrierTSOEnabled()) {
      UnalignedHandlerType = FEXCore::ArchHelpers::Arm64::UnalignedHandlerType::HalfBarrier;
    } else {
      UnalignedHandlerType = FEXCore::ArchHelpers::Arm64::UnalignedHandlerType::NonAtomic;
    }
  }

  FEXCore::ArchHelpers::Arm64::UnalignedHandlerType GetUnalignedHandlerType() const {
    return UnalignedHandlerType;
  }

private:
  FEX_CONFIG_OPT(ParanoidTSO, PARANOIDTSO);
  FEX_CONFIG_OPT(HalfBarrierTSOEnabled, HALFBARRIERTSOENABLED);

  FEXCore::ArchHelpers::Arm64::UnalignedHandlerType UnalignedHandlerType {FEXCore::ArchHelpers::Arm64::UnalignedHandlerType::HalfBarrier};
};
} // namespace FEX::Windows
