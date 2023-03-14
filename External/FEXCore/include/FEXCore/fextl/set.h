#pragma once
#include <FEXCore/fextl/allocator.h>

#include <set>

namespace fextl {
  template<class Key, class Compare = std::less<Key>, class Allocator = fextl::FEXAlloc<Key>>
  using set = std::set<Key, Compare, Allocator>;

  template<class Key, class Compare = std::less<Key>, class Allocator = fextl::FEXAlloc<Key>>
  using multiset = std::multiset<Key, Compare, Allocator>;
}
