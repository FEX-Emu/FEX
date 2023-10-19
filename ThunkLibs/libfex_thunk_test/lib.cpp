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

} // extern "C"
