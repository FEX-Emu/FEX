#include "api.h"

extern "C" {

uint32_t GetDoubledValue(uint32_t input) {
  return 2 * input;
}

struct OpaqueType {
  uint32_t data;
};

OpaqueType* MakeOpaqueType(uint32_t data) {
  return new OpaqueType { data };
}

uint32_t ReadOpaqueTypeData(OpaqueType* value) {
  return value->data;
}

void DestroyOpaqueType(OpaqueType* value) {
  delete value;
}

UnionType MakeUnionType(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
  return UnionType { .c = { a, b, c, d } };
}

uint32_t GetUnionTypeA(UnionType* value) {
  return value->a;
}

} // extern "C"
