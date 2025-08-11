// SPDX-License-Identifier: MIT
#pragma once

#include <winternl.h>

namespace FEX::Windows {
  bool ValidateHandleAccess(HANDLE Handle, ACCESS_MASK Access) {
    OBJECT_BASIC_INFORMATION Info;

    if (NtQueryObject(Handle, ObjectBasicInformation, &Info, sizeof(Info), nullptr)) {
	return false;
    }

    return (Info.GrantedAccess & Access) == Access;
  }
}
