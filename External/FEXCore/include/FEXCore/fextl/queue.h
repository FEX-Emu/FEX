#pragma once
#include <FEXCore/fextl/allocator.h>
#include <FEXCore/fextl/deque.h>

#include <queue>

namespace fextl {
  template<class T, class Container = fextl::deque<T>>
  using queue = std::queue<T, Container>;
}
