// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/fextl/allocator.h>

#include <ankerl/unordered_dense.h>

namespace fextl {
template<class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>, class Allocator = fextl::FEXAlloc<std::pair<Key, T>>>
using robin_map = ankerl::unordered_dense::map<Key, T, Hash, KeyEqual, Allocator>;
}
