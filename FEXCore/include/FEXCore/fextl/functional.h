// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Utils/AllocatorHooks.h>

#include <functional>
#include <type_traits>
#include <utility>

namespace fextl {

/**
 * Equivalent to std::move_only_function but uses FEXCore::Allocator routines
 * for non-function pointers.
 */
template<typename F, void* (*Alloc)(size_t, size_t) = ::FEXCore::Allocator::aligned_alloc, void (*Dealloc)(void*) = ::FEXCore::Allocator::aligned_free>
class move_only_function;

template<typename R, typename... Args, void* (*Alloc)(size_t, size_t), void (*Dealloc)(void*)>
class move_only_function<R(Args...), Alloc, Dealloc> {
public:
  template<typename F>
  requires std::is_invocable_r_v<R, F, Args...>
  move_only_function(F&& f) noexcept(std::is_nothrow_move_constructible_v<F>) {
    if constexpr (std::is_convertible_v<F, R (*)(Args...)>) {
      // Argument is a function pointer, a captureless lambda, or a stateless function object.
      // std::function can store these without allocation
      internal = std::move(f);
    } else if constexpr (std::is_nothrow_constructible_v<std::function<R(Args...)>, F>) {
      // If construction is guaranteed not to throw an exception, this implies
      // the std::function implementation won't allocate memory!
      internal = std::move(f);
    } else {
      // Other arguments require allocation, which is a problem since
      // std::function doesn't allow allocator customization. Implementations
      // are generally able to avoid allocation for lambdas with a single
      // pointer capture however. We can exploit this special case by wrapping
      // the actual argument in a lambda that points an external storage
      // location.

      static_assert(!std::is_pointer_v<F>, "Pointer types must manually be dereferenced");

      // First, relocate argument to a location returned from FEX's allocators
      using Fnoref = std::remove_reference_t<F>;
      storage = Alloc(std::alignment_of_v<Fnoref>, sizeof(Fnoref));
      auto moved_lambda = new (storage) Fnoref {std::move(f)};

      // Second, wrap the relocated argument in a single-capture lambda
      auto wrapped_lambda = [moved_lambda](Args... args) {
        return (*moved_lambda)(std::forward<Args>(args)...);
      };

      // Third, assign the result to std::function, ensuring it's indeed
      // allocation-free by checking for nothrow-constructibility
      static_assert(noexcept(internal = std::move(wrapped_lambda)), "This implementation of std::function "
                                                                    "does not support implementing "
                                                                    "fextl::move_only_function");
      internal = std::move(wrapped_lambda);

      // Finally, if a destructor must be called, generate a pointer to its destructor
      if constexpr (!std::is_trivially_destructible_v<Fnoref>) {
        internal_destructor = [](move_only_function* self) {
          reinterpret_cast<Fnoref*>(self->storage)->~Fnoref();
        };
      }
    }
  }

  move_only_function() noexcept {}
  move_only_function(std::nullptr_t) noexcept {}
  move_only_function(const move_only_function&) = delete;
  move_only_function(move_only_function&& other) noexcept {
    *this = std::move(other);
  }

  move_only_function& operator=(move_only_function&& other) noexcept {
    if (!other && internal_destructor) {
      this->~move_only_function();
    }
    internal = std::exchange(other.internal, nullptr);
    internal_destructor = std::exchange(other.internal_destructor, nullptr);
    storage = std::exchange(other.storage, nullptr);
    return *this;
  }

  ~move_only_function() {
    if (internal_destructor) {
      internal_destructor(this);
    }
    Dealloc(storage);
  }

  R operator()(Args... args) const {
    return internal(std::forward<Args>(args)...);
  }

  explicit operator bool() const noexcept {
    return (bool)internal;
  }

private:
  std::function<R(Args...)> internal;
  void (*internal_destructor)(move_only_function*) = nullptr;
  void* storage = nullptr;
};
} // namespace fextl
