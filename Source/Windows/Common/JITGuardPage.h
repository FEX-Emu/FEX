// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/Utils/LongJump.h>

namespace FEX::Windows::JITGuardPage {
static constexpr bool IsAddressInJITGuardPage(uintptr_t GuardPage, uintptr_t Address) {
  return GuardPage != 0 && Address >= GuardPage && Address < GuardPage + FEXCore::Utils::FEX_PAGE_SIZE;
}

static_assert(!IsAddressInJITGuardPage(0, 0));
static_assert(IsAddressInJITGuardPage(0x1000, 0x1000));
static_assert(IsAddressInJITGuardPage(0x1000, 0x1fff));
static_assert(!IsAddressInJITGuardPage(0x1000, 0x2000));

static inline bool HandleJITGuardPage(FEXCore::Core::InternalThreadState* Thread, void* Address, uint64_t* GPRs, __uint128_t* FPRs, uint64_t* PC) {
  if (IsAddressInJITGuardPage(Thread->JITGuardPage, reinterpret_cast<uintptr_t>(Address))) {
    FEXCore::UncheckedLongJump::ManuallyLoadJumpBuf(Thread->RestartJump, Thread->JITGuardOverflowArgument, GPRs, FPRs, PC);
    return true;
  }

  return false;
}

} // namespace FEX::Windows::JITGuardPage
