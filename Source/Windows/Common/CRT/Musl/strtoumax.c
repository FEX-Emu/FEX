// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: Copyright © 2005-2011 Rich Felker, et al.
// NOTE: From an older musl release that avoids stdio usage

#include <inttypes.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>

/* Lookup table for digit values. -1==255>=36 -> invalid */
static const unsigned char digits[] = {
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  -1, -1, -1, -1, -1, -1,
  -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1,
  -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

uintmax_t strtoumax(const char* s1, char** p, int base) {
  const unsigned char* s = s1;
  size_t x1, z1;
  uintmax_t x, z = 0;
  int sign = 0;
  int shift;

  if (!p) {
    p = (char**)&s1;
  }

  /* Initial whitespace */
  for (; isspace(*s); s++)
    ;

  /* Optional sign */
  if (*s == '-') {
    sign = *s++;
  } else if (*s == '+') {
    s++;
  }

  /* Default base 8, 10, or 16 depending on prefix */
  if (base == 0) {
    if (s[0] == '0') {
      if ((s[1] | 32) == 'x') {
        base = 16;
      } else {
        base = 8;
      }
    } else {
      base = 10;
    }
  }

  if ((unsigned)base - 2 > 36 - 2 || digits[*s] >= base) {
    *p = (char*)s1;
    errno = EINVAL;
    return 0;
  }

  /* Main loops. Only use big types if we have to. */
  if (base == 10) {
    for (x1 = 0; isdigit(*s) && x1 <= SIZE_MAX / 10 - 10; s++) {
      x1 = 10 * x1 + *s - '0';
    }
    for (x = x1; isdigit(*s) && x <= UINTMAX_MAX / 10 - 10; s++) {
      x = 10 * x + *s - '0';
    }
    if (isdigit(*s)) {
      if (isdigit(s[1]) || 10 * x > UINTMAX_MAX - (*s - '0')) {
        goto overflow;
      }
      x = 10 * x + *s - '0';
    }
  } else if (!(base & base / 2)) {
    if (base == 16) {
      if (s[0] == '0' && (s[1] | 32) == 'x' && digits[s[2]] < 16) {
        s += 2;
      }
      shift = 4;
      z1 = SIZE_MAX / 16;
      z = UINTMAX_MAX / 16;
    } else if (base == 8) {
      shift = 3;
      z1 = SIZE_MAX / 8;
      z = UINTMAX_MAX / 8;
    } else if (base == 2) {
      shift = 1;
      z1 = SIZE_MAX / 2;
      z = UINTMAX_MAX / 2;
    } else if (base == 4) {
      shift = 2;
      z1 = SIZE_MAX / 4;
      z = UINTMAX_MAX / 4;
    } else /* if (base == 32) */ {
      shift = 5;
      z1 = SIZE_MAX / 32;
      z = UINTMAX_MAX / 32;
    }
    for (x1 = 0; digits[*s] < base && x1 <= z1; s++) {
      x1 = (x1 << shift) + digits[*s];
    }
    for (x = x1; digits[*s] < base && x <= z; s++) {
      x = (x << shift) + digits[*s];
    }
    if (digits[*s] < base) {
      goto overflow;
    }
  } else {
    z1 = SIZE_MAX / base - base;
    for (x1 = 0; digits[*s] < base && x1 <= z1; s++) {
      x1 = x1 * base + digits[*s];
    }
    if (digits[*s] < base) {
      z = UINTMAX_MAX / base - base;
    }
    for (x = x1; digits[*s] < base && x <= z; s++) {
      x = x * base + digits[*s];
    }
    if (digits[*s] < base) {
      if (digits[s[1]] < base || x * base > UINTMAX_MAX - digits[*s]) {
        goto overflow;
      }
      x = x * base + digits[*s];
    }
  }

  *p = (char*)s;
  return sign ? -x : x;

overflow:
  for (; digits[*s] < base; s++)
    ;
  *p = (char*)s;
  errno = ERANGE;
  return UINTMAX_MAX;
}
