// SPDX-License-Identifier: MIT
#pragma once
#include <winternl.h>

namespace FEX::Windows::Allocator {
void SetupHooks(bool IsWine);
}
