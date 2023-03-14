#pragma once
#include <fextl/allocator.h>

#include <unordered_set>

namespace fextl {
  template<class Key, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>, class Allocator = fextl::FEXAlloc<Key>>
  using unordered_set = std::unordered_set<Key, Hash, KeyEqual, Allocator>;

  template<class Key, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>, class Allocator = fextl::FEXAlloc<Key>>
  using unordered_multiset = std::unordered_multiset<Key, Hash, KeyEqual, Allocator>;
}
