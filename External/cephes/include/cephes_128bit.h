#pragma once

extern "C" {
#include "SoftFloat-3e/platform.h"
#include "SoftFloat-3e/softfloat.h"
}

namespace FEXCore::cephes_128bit {
  float128_t atan2l(float128_t y, float128_t x);
  float128_t cosl(float128_t x);
  float128_t exp2l(float128_t x);
  float128_t log2l(float128_t x);
  float128_t sinl(float128_t x);
  float128_t tanl(float128_t x);
}
