#pragma once
#include <FEXCore/fextl/allocator.h>

#include <vector>

namespace fextl {
  template<class T, class Allocator = fextl::FEXAlloc<T>>
  using vector = std::vector<T, Allocator>;
}
