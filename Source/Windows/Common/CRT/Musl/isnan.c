// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: Copyright © 2005-2020 Rich Felker, et al.

static unsigned long long __DOUBLE_BITS(double __f) {
  union {
    double __f;
    unsigned long long __i;
  } __u;
  __u.__f = __f;
  return __u.__i;
}


int __isnan(double x) {
  return (__DOUBLE_BITS(x) & -1ULL >> 1) > 0x7ffULL << 52;
}
