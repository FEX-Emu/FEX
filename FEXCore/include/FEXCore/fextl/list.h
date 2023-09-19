// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/fextl/allocator.h>

#include <list>

namespace fextl {
  template<class T, class Allocator = fextl::FEXAlloc<T>>
  using list = std::list<T, Allocator>;
}
