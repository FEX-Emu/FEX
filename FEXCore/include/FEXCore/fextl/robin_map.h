// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/fextl/allocator.h>

#include <tsl/robin_map.h>

namespace fextl {
  template<class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>, class Allocator = fextl::FEXAlloc<std::pair<const Key, T>>>
  using robin_map = tsl::robin_map<Key, T, Hash, KeyEqual, Allocator>;
}
