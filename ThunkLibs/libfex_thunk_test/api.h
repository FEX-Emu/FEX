/**
 * This file defines interfaces of a dummy library used to test various
 * features of the thunk generator.
 */
#pragma once

#include <cstdint>

extern "C" {

uint32_t GetDoubledValue(uint32_t);


/// Interfaces used to test automatic struct repacking

// A simple struct with data layout that differs between guest and host.
// The thunk generator should emit code that swaps the member data into
// correct position.
struct ReorderingType {
#if defined(__aarch64__) || defined(_M_ARM64) // TODO: Use proper host check
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
