/**
 * This file defines interfaces of a dummy library used to test various
 * features of the thunk generator.
 */
#pragma once

#include <cstdint>
#include <limits>

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
uint32_t GetReorderingTypeMember(const ReorderingType*, int index);
void ModifyReorderingTypeMembers(ReorderingType* data);
uint32_t QueryOffsetOf(ReorderingType*, int index);

// Uses assume_compatible_data_layout to skip repacking
uint32_t GetReorderingTypeMemberWithoutRepacking(const ReorderingType*, int index);

/// Interfaces used to test assisted struct repacking

// We enable custom repacking on the "data" member, with repacking code that
// sets the first bit of "custom_repack_invoked" to 1 on entry.
struct CustomRepackedType {
    ReorderingType* data;
    int custom_repack_invoked;
};

// Should return true if the custom repacker set "custom_repack_invoked" to true
int RanCustomRepack(CustomRepackedType*);

/// Interface used to check that function arguments with different integer size
/// get forwarded correctly

#if !defined(GUEST_THUNK_LIBRARY)
enum DivType : uint8_t {};
#else
enum DivType : uint32_t {};
#endif
int FunctionWithDivergentSignature(DivType, DivType, DivType, DivType);


/// Interfaces used to test Vulkan-like APIs

// Equivalent of VkStructureType
enum class StructType {
    Struct1,
    Struct2,
};

// Equivalent of VkBaseInStructure
struct TestBaseStruct {
    TestBaseStruct* Next;
    StructType Type;
};

// Equivalent of e.g. VkImageCreateInfo
struct TestStruct1 {
    const void* Next;
    StructType Type; // StructType::Struct1
    uint8_t Data2;
    uint8_t pad0[3];
    int Data1;
};

struct TestStruct2 {
    const void* Next;
    StructType Type;  // StructType::Struct2
    int Data1;
};

int ReadData1(TestStruct1*, int depth);

}
