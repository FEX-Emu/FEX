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

ReorderingType MakeReorderingType(uint32_t a, uint32_t b) {
  return ReorderingType { .a = a, .b = b };
}

uint32_t GetReorderingTypeMember(ReorderingType* data, int index) {
  if (index == 0) {
    return data->a;
  } else {
    return data->b;
  }
}

void ModifyReorderingTypeMembers(ReorderingType* data) {
  data->a += 1;
  data->b += 2;
}

int RanCustomRepack(CustomRepackedType* data) {
  return data->custom_repack_invoked;
}

} // extern "C"
