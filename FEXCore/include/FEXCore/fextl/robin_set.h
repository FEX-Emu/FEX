// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/fextl/allocator.h>

#include <tsl/robin_set.h>

namespace fextl {
template<class Key, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>, class Allocator = fextl::FEXAlloc<Key>>
using robin_set = tsl::robin_set<Key, Hash, KeyEqual, Allocator>;
}
