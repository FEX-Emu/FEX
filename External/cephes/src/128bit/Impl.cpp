#include "cephes_128bit.h"

extern "C" {
// cephes_128bit functions
float128_t cephes_f128_atan2l(float128_t y, float128_t x);
float128_t cephes_f128_cosl(float128_t x);
float128_t cephes_f128_exp2l(float128_t x);
float128_t cephes_f128_log2l(float128_t x);
float128_t cephes_f128_sinl(float128_t x);
float128_t cephes_f128_tanl(float128_t x);
}

namespace FEXCore::cephes_128bit {
  float128_t atan2l(float128_t y, float128_t x) {
    return cephes_f128_atan2l(y, x);
  }
  float128_t cosl(float128_t x) {
    return cephes_f128_cosl(x);
  }
  float128_t exp2l(float128_t x) {
    return cephes_f128_exp2l(x);
  }
  float128_t log2l(float128_t x) {
    return cephes_f128_log2l(x);
  }
  float128_t sinl(float128_t x) {
    return cephes_f128_sinl(x);
  }
  float128_t tanl(float128_t x) {
    return cephes_f128_tanl(x);
  }
}
