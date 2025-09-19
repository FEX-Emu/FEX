// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/Utils/LongJump.h>

namespace FEX::Windows::JITGuardPage {
static inline bool HandleJITGuardPage(FEXCore::Core::InternalThreadState* Thread, void* Address, uint64_t* GPRs, __uint128_t* FPRs, uint64_t* PC) {
  if (Address >= reinterpret_cast<void*>(Thread->JITGuardPage) &&
      Address < reinterpret_cast<void*>(Thread->JITGuardPage + FEXCore::Utils::FEX_PAGE_SIZE)) {
    FEXCore::UncheckedLongJump::ManuallyLoadJumpBuf(Thread->RestartJump, Thread->JITGuardOverflowArgument, GPRs, FPRs, PC);
    return true;
  }

  return false;
}

} // namespace FEX::Windows::JITGuardPage
