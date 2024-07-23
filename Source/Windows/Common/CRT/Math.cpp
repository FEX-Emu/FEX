// SPDX-License-Identifier: MIT
#define _SECIMP
#define _CRTIMP
#include <cstdlib>
#include <cstdint>
#include <cmath>

long double tanl(long double X) {
  return tan(static_cast<double>(X));
}

long double sinl(long double X) {
  return sin(static_cast<double>(X));
}

long double cosl(long double X) {
  return cos(static_cast<double>(X));
}

long double exp2l(long double N) {
  return exp2(static_cast<double>(N));
}

long double log2l(long double N) {
  return log2(static_cast<double>(N));
}

long double atan2l(long double X, long double Y) {
  return atan2(static_cast<double>(X), static_cast<double>(Y));
}
