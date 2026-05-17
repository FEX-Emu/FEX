// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Core/Context.h>
#include <FEXCore/Config/Config.h>
#include <FEXCore/Utils/ArchHelpers/Arm64.h>

#include "Unixlib/FEXUnixlib.h"
#include <wine/unixlib.h>

namespace FEX::Windows {
class TSOHandlerConfig final {
public:
  TSOHandlerConfig(FEXCore::Context::Context& CTX) {
    if (HalfBarrierTSOEnabled()) {
      UnalignedHandlerType = FEXCore::ArchHelpers::Arm64::UnalignedHandlerType::HalfBarrier;
    } else {
      UnalignedHandlerType = FEXCore::ArchHelpers::Arm64::UnalignedHandlerType::NonAtomic;
    }

    if (TSOEnabled()) {
      FexSetHardwareTSOParams Params {.enable = 1};
      if (WINE_UNIX_CALL(fex_unix_set_hardware_tso, &Params) == STATUS_SUCCESS) {
        CTX.SetHardwareTSOSupport(true);
      }
    }

    uint64_t Flags = (StrictInProcessSplitLocks() ? FEX_UNALIGN_ATOMIC_STRICT_SPLIT_LOCKS : 0) |
                     (KernelUnalignedAtomicBackpatching() ? FEX_UNALIGN_ATOMIC_BACKPATCH : 0) | FEX_UNALIGN_ATOMIC_EMULATE;

    FexSetUnalignAtomicParams Params {.flags = Flags};
    if (WINE_UNIX_CALL(fex_unix_set_unalign_atomic, &Params) == STATUS_SUCCESS) {
      LogMan::Msg::IFmt("FEX: Kernel unaligned atomics enabled!");
    }
  }

  FEXCore::ArchHelpers::Arm64::UnalignedHandlerType GetUnalignedHandlerType() const {
    return UnalignedHandlerType;
  }

private:
  FEX_CONFIG_OPT(TSOEnabled, TSOENABLED);
  FEX_CONFIG_OPT(HalfBarrierTSOEnabled, HALFBARRIERTSOENABLED);
  FEX_CONFIG_OPT(StrictInProcessSplitLocks, STRICTINPROCESSSPLITLOCKS);
  FEX_CONFIG_OPT(KernelUnalignedAtomicBackpatching, KERNELUNALIGNEDATOMICBACKPATCHING);

  FEXCore::ArchHelpers::Arm64::UnalignedHandlerType UnalignedHandlerType {FEXCore::ArchHelpers::Arm64::UnalignedHandlerType::HalfBarrier};
};
} // namespace FEX::Windows
