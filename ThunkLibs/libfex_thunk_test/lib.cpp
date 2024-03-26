#include "api.h"

#include <cstdio>
#include <cstddef>

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

uint32_t GetReorderingTypeMember(const ReorderingType* data, int index) {
  if (index == 0) {
    return data->a;
  } else {
    return data->b;
  }
}

uint32_t GetReorderingTypeMemberWithoutRepacking(const ReorderingType* data, int index) {
  return GetReorderingTypeMember(data, index);
}

void ModifyReorderingTypeMembers(ReorderingType* data) {
  data->a += 1;
  data->b += 2;
}

int RanCustomRepack(CustomRepackedType* data) {
  return data->custom_repack_invoked;
}

int FunctionWithDivergentSignature(DivType a, DivType b, DivType c, DivType d) {
  return ((uint8_t)a << 24) | ((uint8_t)b << 16) | ((uint8_t)c << 8) | (uint8_t)d;
}

int ReadData1(TestStruct1* data, int depth) {
  auto* base = (TestBaseStruct*)data;
  for (int i = 0; i != depth; ++i) {
    if (!base) {
      return -1;
    }
    base = base->Next;
  }
  if (!base) {
    return -1;
  }

  switch (base->Type) {
  case StructType::Struct1:
    return ((TestStruct1*)base)->Data1;

  case StructType::Struct2:
    return ((TestStruct2*)base)->Data1;

  default:
    return -2;
  }
}

} // extern "C"
