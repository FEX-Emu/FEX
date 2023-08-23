#include "api.h"

extern "C" {

uint32_t GetDoubledValue(uint32_t input) {
  return 2 * input;
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

} // extern "C"
