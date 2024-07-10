// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/fextl/allocator.h>

#include <memory>
#include <new>

namespace fextl {
template<class T>
struct default_delete : public std::default_delete<T> {
  void operator()(T* ptr) const {
    if (ptr) {
      std::destroy_at(ptr);
      FEXCore::Allocator::aligned_free(ptr);
    }
  }

  template<typename U>
  requires (std::is_base_of_v<U, T>)
  operator fextl::default_delete<U>() {
    return fextl::default_delete<U>();
  }
};

template<class T, class Deleter = fextl::default_delete<T>>
using unique_ptr = std::unique_ptr<T, Deleter>;

template<class T, class... Args>
requires (!std::is_array_v<T>)
fextl::unique_ptr<T> make_unique(Args&&... args) {
  auto ptr = FEXCore::Allocator::aligned_alloc(std::alignment_of_v<T>, sizeof(T));
  auto Result = ::new (ptr) T(std::forward<Args>(args)...);
  return fextl::unique_ptr<T>(Result);
}
} // namespace fextl
