// SPDX-License-Identifier: MIT
#pragma once

#include <winternl.h>

namespace FEX::Windows {
class ScopedHandle final {
public:
  ScopedHandle() = default;

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

  const HANDLE& operator*() const {
    return Handle;
  }

  HANDLE& operator*() {
    return Handle;
  }

  operator bool() const {
    return Handle != INVALID_HANDLE_VALUE;
  }

private:
  HANDLE Handle {INVALID_HANDLE_VALUE};
};


inline bool ValidateHandleAccess(HANDLE Handle, ACCESS_MASK Access) {
  OBJECT_BASIC_INFORMATION Info;

  if (NtQueryObject(Handle, ObjectBasicInformation, &Info, sizeof(Info), nullptr)) {
    return false;
  }

  return (Info.GrantedAccess & Access) == Access;
}

inline ScopedHandle DupHandle(HANDLE Handle, ACCESS_MASK Access) {
  HANDLE Duplicated = INVALID_HANDLE_VALUE;
  NtDuplicateObject(NtCurrentProcess(), Handle, NtCurrentProcess(), &Duplicated, Access, 0, 0);
  return ScopedHandle {Duplicated};
}
} // namespace FEX::Windows
