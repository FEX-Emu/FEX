// SPDX-License-Identifier: MIT
#pragma once
#include <functional>
#include <cstdint>

namespace Unwind {
class Unwinder {
public:
   virtual void Backtrace() = 0;
};
}
