// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Core/Context.h>
#include <FEXCore/Config/Config.h>
#include <FEXCore/Utils/ArchHelpers/Arm64.h>

namespace FEX::Windows {
class TSOHandlerConfig final {
public:
  TSOHandlerConfig(FEXCore::Context::Context& CTX) {
    if (ParanoidTSO()) {
      UnalignedHandlerType = FEXCore::ArchHelpers::Arm64::UnalignedHandlerType::Paranoid;
    } else if (HalfBarrierTSOEnabled()) {
      UnalignedHandlerType = FEXCore::ArchHelpers::Arm64::UnalignedHandlerType::HalfBarrier;
    } else {
      UnalignedHandlerType = FEXCore::ArchHelpers::Arm64::UnalignedHandlerType::NonAtomic;
    }

    if (TSOEnabled()) {
      BOOL Enable = TRUE;
      NTSTATUS Status = NtSetInformationProcess(NtCurrentProcess(), ProcessFexHardwareTso, &Enable, sizeof(Enable));
      if (Status == STATUS_SUCCESS) {
        CTX.SetHardwareTSOSupport(true);
      }
    }

    uint64_t Flags = (StrictInProcessSplitLocks() ? FEX_UNALIGN_ATOMIC_STRICT_SPLIT_LOCKS : 0) |
                     (ParanoidTSO() ? 0 : FEX_UNALIGN_ATOMIC_BACKPATCH) | FEX_UNALIGN_ATOMIC_EMULATE;

    if (NtSetInformationProcess(NtCurrentProcess(), ProcessFexUnalignAtomic, &Flags, sizeof(Flags)) == STATUS_SUCCESS) {
      LogMan::Msg::IFmt("FEX: Kernel unaligned atomics enabled!");
    }
  }

  FEXCore::ArchHelpers::Arm64::UnalignedHandlerType GetUnalignedHandlerType() const {
    return UnalignedHandlerType;
  }

private:
  FEX_CONFIG_OPT(TSOEnabled, TSOENABLED);
  FEX_CONFIG_OPT(ParanoidTSO, PARANOIDTSO);
  FEX_CONFIG_OPT(HalfBarrierTSOEnabled, HALFBARRIERTSOENABLED);
  FEX_CONFIG_OPT(StrictInProcessSplitLocks, STRICTINPROCESSSPLITLOCKS);

  FEXCore::ArchHelpers::Arm64::UnalignedHandlerType UnalignedHandlerType {FEXCore::ArchHelpers::Arm64::UnalignedHandlerType::HalfBarrier};
};
} // namespace FEX::Windows
