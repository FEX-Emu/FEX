#pragma once
#include <cstddef>
#include <sys/types.h>
#include <vector>

namespace FEX::Debugger::Util {
  template<typename T>
  class DataRingBuffer {
    public:
    DataRingBuffer(size_t Size)
      : RingSize {Size}
      , BufferSize {RingSize * SIZE_MULTIPLE} {
      Data.resize(BufferSize);
    }

    T const* operator()() const {
      return &Data.at(ReadOffset);
    }

    T back() const {
      return Data.at(WriteOffset - 1);
    }

    bool empty() const {
      return WriteOffset == 0;
    }

    size_t size() const {
      return WriteOffset - ReadOffset;
    }

    void push_back(T Val) {
      if (WriteOffset + 1 >= BufferSize) {
        // We reached the end of the buffer size. Time to wrap around
        // Copy the ring buffer expected size to the start
        memcpy(&Data.at(0), &Data.at(ReadOffset), sizeof(T) * RingSize);
        WriteOffset = RingSize;
        ReadOffset = 0;
      }
      Data.at(WriteOffset) = Val;
      ++WriteOffset;
      if (WriteOffset - ReadOffset > RingSize) {
        ++ReadOffset;
      }
    }

  private:
    constexpr static ssize_t SIZE_MULTIPLE = 3;
    size_t RingSize;
    size_t WriteOffset{};
    size_t ReadOffset{};
    size_t BufferSize;
    std::vector<T> Data;
  };
}
