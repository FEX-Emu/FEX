// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/Utils/MathUtils.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/TypeDefines.h>
#include <FEXCore/fextl/allocator.h>
#include <FEXCore/fextl/list.h>

#include <memory_resource>
#include <fmt/format.h>

namespace fextl {
namespace pmr {
  class default_resource : public std::pmr::memory_resource {
  private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
      return FEXCore::Allocator::memalign(alignment, bytes);
    }

    void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override {
      return FEXCore::Allocator::aligned_free(p);
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
      return this == &other;
    }
  };

  FEX_DEFAULT_VISIBILITY std::pmr::memory_resource* get_default_resource();

  /**
   * @brief A `std::pmr::monotonic_buffer_resource` compatible class.
   *
   * Allocates internal buffers on page boundaries and names them for buffer tracking.
   */
  class named_monotonic_page_buffer_resource final : public std::pmr::memory_resource {
  public:
    explicit named_monotonic_page_buffer_resource(const char* Name)
      : Name {Name} {}

    void release() noexcept {
      for (auto& Iter : Buffers) {
        FEXCore::Allocator::VirtualFree(Iter.Buffer, Iter.BufferSize);
      }
      Buffers.clear();

      CurrentBufferRemaining = 0;
      CurrentAllocationSize = FEXCore::Utils::FEX_PAGE_SIZE;
    }

  protected:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
      LOGMAN_THROW_A_FMT(bytes != 0, "Nope");
      LOGMAN_THROW_A_FMT(alignment <= 4096, "Nope");

      // Wow, an actual use case of std::align in the wild.
      void* NewPointer = std::align(alignment, bytes, CurrentBuffer, CurrentBufferRemaining);
      if (!NewPointer) [[unlikely]] {
        AllocateNewBuffer(bytes, alignment);
        NewPointer = CurrentBuffer;
      }

      CurrentBuffer = static_cast<char*>(CurrentBuffer) + bytes;
      CurrentBufferRemaining -= bytes;

      return NewPointer;
    }

    void do_deallocate(void*, std::size_t, std::size_t) override {
      // Explicit no-op.
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
      return this == &other;
    }

  private:
    const char* Name;

    // Allocate a new buffer that can at least fit the passed in bytes with alignment.
    void AllocateNewBuffer(std::size_t bytes, std::size_t) {
      bytes = FEXCore::AlignUp(bytes, CurrentAllocationSize);
      void* Ptr = FEXCore::Allocator::VirtualAlloc(bytes);
      if (Name) {
        FEXCore::Allocator::VirtualName(Name, Ptr, bytes);
      }

      Buffers.emplace_back(BufferData {
        .Buffer = Ptr,
        .BufferSize = bytes,
      });

      CurrentBuffer = Ptr;
      CurrentBufferRemaining = bytes;

      // Multiply the allocation size by 1.5 for the next allocation
      // Avoid double math because of ugly conversions.
      CurrentAllocationSize = FEXCore::AlignUp(CurrentAllocationSize + (CurrentAllocationSize >> 1), FEXCore::Utils::FEX_PAGE_SIZE);
    }

    // Current buffer management.
    void* CurrentBuffer {};
    size_t CurrentBufferRemaining {};

    struct BufferData final {
      void* Buffer;
      size_t BufferSize;
    };

    fextl::list<BufferData> Buffers {};

    size_t CurrentAllocationSize = FEXCore::Utils::FEX_PAGE_SIZE;
  };

  /**
   * @brief This is similar to the std::pmr::monotonic_buffer_resource.
   *
   * The difference is that class doesn't have ownership of the backing memory and
   * it also doesn't have any growth factor.
   *
   * If the amount of memory allocated is overrun then this will overwrite memory unless assertions are enabled.
   *
   * Ensure that you know how much memory you're going to use before using this class.
   */
  class fixed_size_monotonic_buffer_resource final : public std::pmr::memory_resource {
  public:
    fixed_size_monotonic_buffer_resource(void* Base, [[maybe_unused]] size_t Size)
      : Ptr {reinterpret_cast<uint64_t>(Base)}
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
      , PtrEnd {reinterpret_cast<uint64_t>(Base) + Size}
      , Size {Size}
#endif
    {
    }
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
      uint64_t NewPtr = FEXCore::AlignUp((uint64_t)Ptr, alignment);
      Ptr = NewPtr + bytes;
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
      if (Ptr >= PtrEnd) {
        LogMan::Msg::AFmt("Fail: Only allocated: {} ({} this time) bytes. Tried allocating at ptr offset: {}.\n", Size, bytes,
                          (uint64_t)(Ptr - (PtrEnd - Size)));
        FEX_TRAP_EXECUTION;
      }
#endif
      return reinterpret_cast<void*>(NewPtr);
    }

    void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override {
      // noop
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
      return this == &other;
    }
  private:
    uint64_t Ptr;
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
    uint64_t PtrEnd;
    size_t Size;
#endif
  };
} // namespace pmr
} // namespace fextl
