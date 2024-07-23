// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: Copyright © 2005-2020 Rich Felker, et al.

#include "libm.h"

double __math_divzero(uint32_t sign) {
  return fp_barrier(sign ? -1.0 : 1.0) / 0.0;
}
