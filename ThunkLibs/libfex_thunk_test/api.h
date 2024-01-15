/**
 * This file defines interfaces of a dummy library used to test various
 * features of the thunk generator.
 */
#pragma once

#include <cstdint>

extern "C" {

uint32_t GetDoubledValue(uint32_t);


/// Interfaces used to test opaque_type and assume_compatible_data_layout annotations

struct OpaqueType;

OpaqueType* MakeOpaqueType(uint32_t data);
uint32_t ReadOpaqueTypeData(OpaqueType*);
void DestroyOpaqueType(OpaqueType*);

union UnionType {
    uint32_t a;
    int32_t b;
    uint8_t c[4];
};

UnionType MakeUnionType(uint8_t a, uint8_t b, uint8_t c, uint8_t d);
uint32_t GetUnionTypeA(UnionType*);


/// Interfaces used to test automatic struct repacking

// A simple struct with data layout that differs between guest and host.
// The thunk generator should emit code that swaps the member data into
// correct position.
struct ReorderingType {
#if !defined(GUEST_THUNK_LIBRARY)
    uint32_t a;
    uint32_t b;
#else
    uint32_t b;
    uint32_t a;
#endif
};

ReorderingType MakeReorderingType(uint32_t a, uint32_t b);
uint32_t GetReorderingTypeMember(ReorderingType*, int index);
void ModifyReorderingTypeMembers(ReorderingType* data);
uint32_t QueryOffsetOf(ReorderingType*, int index);

}
