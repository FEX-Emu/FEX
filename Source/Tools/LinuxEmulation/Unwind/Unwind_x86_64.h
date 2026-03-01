// SPDX-License-Identifier: MIT
#pragma once
#include "Unwind/Unwind.h"

#include <cstdint>
#include <functional>
#include <string>

namespace Unwind::x86_64 {
  Unwinder *Unwind(void *Info, void *Context);
}
