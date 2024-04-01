// SPDX-License-Identifier: MIT
#pragma once
#include <cstddef>

namespace FEXCore::Utils {
  // FEX assumes an operating page size of 4096
  // To work around build systems that build on a 16k/64k page size, define our page size here
  // Don't use the system provided PAGE_SIZE define because of this.
  constexpr size_t FEX_PAGE_SIZE = 4096;
  constexpr size_t FEX_PAGE_SHIFT = 12;
  constexpr size_t FEX_PAGE_MASK = ~(FEX_PAGE_SIZE - 1);
}
