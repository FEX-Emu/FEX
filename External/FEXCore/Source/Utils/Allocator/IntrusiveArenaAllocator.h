#pragma once

#include "FlexBitSet.h"
#include "HostAllocator.h"

#include <FEXCore/Utils/MathUtils.h>
#include <FEXHeaderUtils/TypeDefines.h>

#include <bitset>
#include <cmath>
#include <cstddef>
#ifdef TERMUX_BUILD
#ifdef __has_include
#if __has_include(<memory_resource>)
#error Termux <experimental/memory_resource> workaround can be removed
#endif
#endif
#include <experimental/memory_resource>
#include <experimental/list>
namespace fex_pmr = std::experimental::pmr;
#else
#include <memory_resource>
namespace fex_pmr = std::pmr;
#endif
#include <sys/user.h>

#include <mutex>
#include <unistd.h>
#include <vector>

namespace Alloc {
  class ForwardOnlyIntrusiveArenaAllocator final : public fex_pmr::memory_resource {
  public:
    ForwardOnlyIntrusiveArenaAllocator(void* Ptr, size_t _Size)
      : Begin {reinterpret_cast<uintptr_t>(Ptr)}
      , Size {_Size} {
      LastAllocation = sizeof(ForwardOnlyIntrusiveArenaAllocator);
    }

    ~ForwardOnlyIntrusiveArenaAllocator() = default;

    template<class U, class... Args>
    U *new_construct(Args&&... args) {
      void *Ptr = do_allocate(sizeof(U), std::alignment_of<U>::value);
      return new (Ptr) U(args...);
    }

    template<class U, class... Args>
    U *new_construct(U *Class, Args&&... args) {
      void *Ptr = do_allocate(sizeof(U), std::alignment_of<U>::value);
      return new (Ptr) U(args...);
    }

    size_t AmountAllocated() const { return LastAllocation; }

  private:
    void *do_allocate(std::size_t bytes, std::size_t alignment) override {
      size_t PreviousAligned = FEXCore::AlignUp(LastAllocation, alignment);
      size_t NewOffset = PreviousAligned + bytes;

      if (NewOffset > Size) {
        return nullptr;
      }

      LastAllocation = NewOffset;

      return reinterpret_cast<void*>(Begin + PreviousAligned);
    }

    void do_deallocate(void*, std::size_t, std::size_t) override {
      // Do nothing
    }

    bool do_is_equal(const fex_pmr::memory_resource& other) const noexcept override {
      // Only if the allocator pointers are the same are they equal
      if (this == &other) {
        return true;
      }
      // We don't share state with another allocator so we can't share anything
      return false;
    }

    uintptr_t Begin;
    size_t Size;
    size_t LastAllocation{};
  };

  class IntrusiveArenaAllocator final : public fex_pmr::memory_resource {
  public:
    IntrusiveArenaAllocator(void* Ptr, size_t _Size)
      : Begin {reinterpret_cast<uintptr_t>(Ptr)}
      , Size {_Size}
      , PageSize { getpagesize() }
      , PageShift { static_cast<int32_t>(log2(PageSize)) } {
      uint64_t NumberOfPages = _Size >> PageShift;
      uint64_t UsedBits = FEXCore::AlignUp(sizeof(IntrusiveArenaAllocator) +
        Size / PageSize / 8, PageSize);
      for (size_t i = 0; i < UsedBits; ++i) {
        UsedPages.Set(i);
      }

      FreePages = NumberOfPages - UsedBits;
    }

    template<class U, class... Args>
    U *new_construct(Args&&... args) {
      void *Ptr = do_allocate(sizeof(U), std::alignment_of<U>::value);
      return new (Ptr) U(args...);
    }

    template<class U, class... Args>
    U *new_construct(U *Class, Args&&... args) {
      void *Ptr = do_allocate(sizeof(U), std::alignment_of<U>::value);
      return new (Ptr) U(args...);
    }

    uintptr_t GetSlabBase() const { return Begin; }
    uint64_t GetSlabSize() const { return Size; }
    uint64_t GetFreePages() const { return FreePages; }

  private:
    void *do_allocate(std::size_t bytes, std::size_t alignment) override {
      std::scoped_lock<std::mutex> lk{AllocationMutex};

      size_t NumberPages = FEXCore::AlignUp(bytes, PageSize) >> PageShift;

      uintptr_t AllocatedOffset{};

      try_again:
      for (uintptr_t CurrentPage = LastAllocatedPageOffset; CurrentPage <= (Size - NumberPages);) {
        size_t Remaining = NumberPages;

        while (Remaining) {
          if (UsedPages[CurrentPage + Remaining - 1]) {
            // Has an intersecting range
            break;
          }
          --Remaining;
        }

        if (Remaining) {
          // Didn't find an allocation range
          CurrentPage += Remaining;
        }
        else {
          // We have a range to allocate
          AllocatedOffset = CurrentPage;
          break;
        }
      }

      if (!AllocatedOffset && LastAllocatedPageOffset != 0) {
        // Try again but starting from the beginning
        LastAllocatedPageOffset = 0;
        // Using goto so we don't have recursive mutex shenanigans
        goto try_again;
      }

      // Allocated offset must be valid or zero at this point
      if (AllocatedOffset) {
        // Map the range as no longer available
        for (size_t i = 0; i < NumberPages; ++i) {
          UsedPages.Set(AllocatedOffset + i);
        }

        LastAllocatedPageOffset = AllocatedOffset + NumberPages;

        // Now convert this base page to a pointer and return it
        return reinterpret_cast<void*>(Begin + AllocatedOffset * PageSize);
      }

      return nullptr;
    }

    void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override {
      std::scoped_lock<std::mutex> lk{AllocationMutex};

      uintptr_t PageOffset = (reinterpret_cast<uintptr_t>(p) - Begin) >> PageShift;
      size_t NumPages = FEXCore::AlignUp(bytes, PageSize) >> PageShift;

      // Walk the allocation list and deallocate
      uint64_t FreedPages{};
      for (size_t i = 0; i < NumPages; ++i) {
        FreedPages += UsedPages.TestAndClear(PageOffset + i) ? 1 : 0;
      }
      FreePages += FreedPages;
    }

    bool do_is_equal(const fex_pmr::memory_resource& other) const noexcept override {
      // Only if the allocator pointers are the same are they equal
      if (this == &other) {
        return true;
      }
      // We don't share state with another allocator so we can't share anything
      return false;
    }

    uintptr_t Begin;
    size_t Size;
    const int32_t PageSize {};
    const int32_t PageShift {};

    uint64_t FreePages{};
    size_t LastAllocatedPageOffset{};
    std::mutex AllocationMutex{};
    // For up to 64GB regions this will require up to 2MB tracking
    // Needs to be the last element
    FEXCore::FlexBitSet<uint64_t> UsedPages;
  };
}
