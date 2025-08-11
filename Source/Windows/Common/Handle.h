// SPDX-License-Identifier: MIT
#pragma once

#include <winternl.h>

namespace FEX::Windows {
class ScopedHandle final {
public:
  explicit ScopedHandle(HANDLE Handle)
    : Handle(Handle) {}

  // Move-only type
  ScopedHandle(const ScopedHandle&) = delete;
  ScopedHandle& operator=(ScopedHandle&) = delete;
  ScopedHandle(ScopedHandle&& rhs)
    : Handle(rhs.Handle) {
    rhs.Handle = INVALID_HANDLE_VALUE;
  }

  ~ScopedHandle() {
    if (Handle != INVALID_HANDLE_VALUE) {
      NtClose(Handle);
    }
  }

  HANDLE operator*() const {
    return Handle;
  }
private:
  HANDLE Handle;
};


bool ValidateHandleAccess(HANDLE Handle, ACCESS_MASK Access) {
  OBJECT_BASIC_INFORMATION Info;

  if (NtQueryObject(Handle, ObjectBasicInformation, &Info, sizeof(Info), nullptr)) {
    return false;
  }

  return (Info.GrantedAccess & Access) == Access;
}

ScopedHandle DupHandle(HANDLE Handle, ACCESS_MASK Access) {
  HANDLE Duplicated = INVALID_HANDLE_VALUE;
  NtDuplicateObject(NtCurrentProcess(), Handle, NtCurrentProcess(), &Duplicated, Access, 0, 0);
  return ScopedHandle {Duplicated};
}
} // namespace FEX::Windows
