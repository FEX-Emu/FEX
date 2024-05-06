// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/fextl/allocator.h>
#include <FEXCore/Utils/LogManager.h>

#include <memory>
#include <new>

namespace fextl {
template<class T>
struct default_delete : public std::default_delete<T> {
  void operator()(T* ptr) const {
    std::destroy_at(ptr);
    FEXCore::Allocator::aligned_free(ptr);
  }

  template<typename U>
  requires (std::is_base_of_v<U, T>)
  operator fextl::default_delete<U>() {
    return fextl::default_delete<U>();
  }
};

template<class T, class Deleter = fextl::default_delete<T>>
class unique_ptr {
  static_assert(!std::is_rvalue_reference_v<Deleter>, "Deleter must not be an r-value reference.");

public:
  using pointer = T*;
  using element_type = T;
  using deleter_type = Deleter;

  // Constructors
  constexpr unique_ptr() noexcept
    : pair {} {}

  constexpr unique_ptr(std::nullptr_t) noexcept
    : pair {} {}

  template<typename Del = Deleter>
  constexpr explicit unique_ptr(pointer p) noexcept
    : pair {} {
    pair.first = p;
  }

  template<typename Del = Deleter>
  requires (std::is_copy_constructible_v<Del>)
  constexpr unique_ptr(pointer p, const Deleter& d)
    : pair {p, d} {}

  template<typename Del = Deleter>
  requires (std::is_move_constructible_v<Del>)
  constexpr unique_ptr(pointer p, Deleter&& d)
    : pair {p, std::move(d)} {}

  template<class U, class E>
  requires (!std::is_array_v<U> && std::is_convertible_v<E, deleter_type> &&
            (std::is_same_v<E, deleter_type> || !std::is_lvalue_reference_v<deleter_type>))
  constexpr unique_ptr(unique_ptr<U, E>&& r) noexcept
    : pair {r.release(), std::forward<E>(r.get_deleter())} {}

  // TODO: Missing a bunch
  unique_ptr(const unique_ptr&) = delete;

  // Destructor
  constexpr ~unique_ptr() {
    reset();
  }

  // Assignments
  constexpr unique_ptr& operator=(unique_ptr&& r) noexcept {
    reset(r.release());
    pair.second = std::move(r.get_deleter());
    return *this;
  }

  template<class U, class E>
  requires (!std::is_array_v<U> && std::is_convertible_v<E, deleter_type> &&
            (std::is_same_v<E, deleter_type> || !std::is_lvalue_reference_v<deleter_type>))
  constexpr unique_ptr& operator=(unique_ptr<U, E>&& r) noexcept {
    reset(r.release());
    pair.second = std::forward<E>(r.get_deleter());
    return *this;
  }

  constexpr unique_ptr& operator=(std::nullptr_t) noexcept {
    reset();
    return *this;
  }

  unique_ptr& operator=(const unique_ptr&) = delete;

  // Modifiers
  constexpr pointer release() noexcept {
    auto backup = pair.first;
    pair.first = nullptr;
    return backup;
  }

  void reset(pointer p = pointer()) noexcept {
    if (pair.first) {
      pair.second(pair.first);
    }
    pair.first = p;
  }

  void swap(unique_ptr& other) noexcept {
    std::swap(pair, other.pair);
  }

  // Observers
  pointer get() const noexcept {
    return pair.first;
  }

  deleter_type& get_deleter() noexcept {
    return pair.second;
  }

  const deleter_type& get_deleter() const noexcept {
    return pair.second;
  }

  constexpr explicit operator bool() const noexcept {
    return pair.first != nullptr;
  }

  // Dereferencing
  constexpr typename std::add_lvalue_reference<T>::type operator*() const noexcept(noexcept(*std::declval<pointer>())) {
    LogMan::Throw::AFmt(get() != pointer(), "operator* called without unique_ptr ownership!");
    return *get();
  }

  constexpr pointer operator->() const noexcept {
    LogMan::Throw::AFmt(get() != pointer(), "operator-> called without unique_ptr ownership!");
    return get();
  }

private:
  std::pair<pointer, deleter_type> pair {};
};

static_assert(std::is_standard_layout_v<unique_ptr<uint32_t>>, "Needs to be standard layout");

// Non-member equality operators
// TODO: Missing a bunch
template< class T1, class D1, class T2, class D2 >
constexpr bool operator==(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y) {
  return x.get() == y.get();
}

template< class T, class D >
constexpr bool operator==(const unique_ptr<T, D>& x, std::nullptr_t) noexcept {
  return !x;
}

template<class T, class... Args>
requires (!std::is_array_v<T>)
fextl::unique_ptr<T> make_unique(Args&&... args) {
  auto ptr = FEXCore::Allocator::aligned_alloc(std::alignment_of_v<T>, sizeof(T));
  auto Result = ::new (ptr) T(std::forward<Args>(args)...);
  return fextl::unique_ptr<T>(Result);
}

} // namespace fextl
