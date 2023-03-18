#pragma once

#include <cstddef>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/memory.h>

namespace FEXCore {

  // BucketList is an optimized container, it includes an inline array of Size
  // and can overflow to a linked list of further buckets
  //
  // To optimize for best performance, Size should be big enough to allocate one or two
  // buckets for the typical case
  // Picking a Size so sizeof(Bucket<...>) is a power of two is also a small win
  template<size_t _Size, typename T = uint32_t>
  struct BucketList {
    static constexpr size_t Size = _Size;

    T Items[Size];
    fextl::unique_ptr<BucketList<Size, T>> Next;

    void Clear() {
      Items[0] = T{};
      #if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
      for (size_t i = 1; i < Size; i++) {
        Items[i] = T{0xDEADBEEF};
      }
      #endif
      Next.reset();
    }

    BucketList() {
      Clear();
    }

    template<typename EnumeratorFn>
    void Iterate(EnumeratorFn Enumerator) const {
      size_t i = 0;
      auto Bucket = this;

      while (true) {
        auto Item = Bucket->Items[i];
        if (Item == T{}) {
          break;
        }

        Enumerator(Item);

        if (++i == Size) {
          LOGMAN_THROW_A_FMT(Bucket->Next != nullptr, "Interference bug");
          Bucket = Bucket->Next.get();
          i = 0;
        }
      }
    }

    template<typename EnumeratorFn>
    bool Find(EnumeratorFn Enumerator) const {
      size_t i = 0;
      auto Bucket = this;

      while (true) {
        auto Item = Bucket->Items[i];
        if (Item == T{}) {
          break;
        }

        if (Enumerator(Item))
          return true;

        if (++i == Size) {
          LOGMAN_THROW_A_FMT(Bucket->Next != nullptr, "Bucket in bad state");
          Bucket = Bucket->Next.get();
          i = 0;
        }
      }

      return false;
    }

    void Append(T Val) {
      auto that = this;

      while (that->Next) {
        that = that->Next.get();
      }

      size_t i;
      for (i = 0; i < Size; i++) {
        if (that->Items[i] == T{}) {
          that->Items[i] = Val;
          break;
        }
      }

      if (i < (Size-1)) {
        that->Items[i+1] = T{};
      } else {
        that->Next = fextl::make_unique<BucketList<Size, T>>();
      }
    }
    void Erase(T Val) {
      size_t i = 0;
      auto that = this;
      auto foundThat = this;
      size_t foundI = 0;

      while (true) {
        if (that->Items[i] == Val) {
          foundThat = that;
          foundI = i;
          break;
        }
        else if (++i == Size) {
          i = 0;
          LOGMAN_THROW_A_FMT(that->Next != nullptr, "Bucket::Erase but element not contained");
          that = that->Next.get();
        }
      }

      while (true) {
        if (that->Items[i] == T{}) {
          foundThat->Items[foundI] = that->Items[i-1];
          that->Items[i-1] = T{};
          break;
        }
        else if (++i == Size) {
          if (that->Next->Items[0] == T{}) {
            that->Next.reset();
            foundThat->Items[foundI] = that->Items[Size-1];
            that->Items[Size-1] = T{};
            break;
          }
          i = 0;
          that = that->Next.get();
        }
      }
    }
  };

} // namespace
