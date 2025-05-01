#pragma once
#include <bit>
#include <cstddef>

namespace FHU {
// This is a fast thread-local non-blocking ring-buffer.
// Very useful for debugging purposes to see the history of something.
template<typename T, size_t Elements>
requires (std::has_single_bit(Elements) && std::is_trivially_copyable_v<T>)
class [[deprecated("Not for production use")]] NonBlockRingBuffer final {
public:
  void emplace(T Val) {
    Ring[Current] = Val;
    Current = (Current + 1) & (Elements - 1);
  }

private:
  T Ring[Elements] {};
  size_t Current {};
};
} // namespace FHU
