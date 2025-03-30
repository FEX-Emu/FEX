#include "cephes_128bit.h"

#if !LOW_PRECISION
extern "C" {
// cephes_128bit functions
long double atan2l(long double y, long double x);
long double cosl(long double x);
long double exp2l(long double x);
long double log2l(long double x);
long double sinl(long double x);
long double tanl(long double x);
}
#else
#include <cmath>
#endif

namespace FEXCore::cephes_128bit {
  long double atan2l(long double y, long double x) {
    return ::atan2l(y, x);
  }
  long double cosl(long double x) {
    return ::cosl(x);
  }
  long double exp2l(long double x) {
    return ::exp2l(x);
  }
  long double log2l(long double x) {
    return ::log2l(x);
  }
  long double sinl(long double x) {
    return ::sinl(x);
  }
  long double tanl(long double x) {
    return ::tanl(x);
  }
}
