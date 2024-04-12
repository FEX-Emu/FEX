// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/fextl/allocator.h>

#include <map>

namespace fextl {
template<class Key, class T, class Compare = std::less<Key>, class Allocator = fextl::FEXAlloc<std::pair<const Key, T>>>
using map = std::map<Key, T, Compare, Allocator>;

template<class Key, class T, class Compare = std::less<Key>, class Allocator = fextl::FEXAlloc<std::pair<const Key, T>>>
using multimap = std::multimap<Key, T, Compare, Allocator>;
} // namespace fextl
