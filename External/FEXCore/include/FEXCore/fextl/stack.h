#pragma once
#include <FEXCore/fextl/allocator.h>
#include <FEXCore/fextl/deque.h>

#include <stack>

namespace fextl {
  template<class T, class Container = fextl::deque<T>>
  using stack = std::stack<T, Container>;
}
