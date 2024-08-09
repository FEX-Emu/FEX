// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: Copyright © 2005-2020 Rich Felker, et al.

#include <math.h>

double remainder(double x, double y) {
  int q;
  return remquo(x, y, &q);
}
