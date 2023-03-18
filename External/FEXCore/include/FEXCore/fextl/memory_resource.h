#pragma once
#include <FEXCore/Utils/MathUtils.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/allocator.h>

#include <memory_resource>
#include <fmt/format.h>

namespace fextl {
  namespace pmr {
    class default_resource : public std::pmr::memory_resource {
    private:
      void* do_allocate( std::size_t bytes, std::size_t alignment ) override {
        return FEXCore::Allocator::memalign(alignment, bytes);
      }

      void do_deallocate( void* p, std::size_t bytes, std::size_t alignment ) override {
        return FEXCore::Allocator::free(p);
      }

      bool do_is_equal( const std::pmr::memory_resource& other ) const noexcept override {
        return this == &other;
      }
    };

    FEX_DEFAULT_VISIBILITY std::pmr::memory_resource* get_default_resource();

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
        fixed_size_monotonic_buffer_resource(void* Base, size_t Size)
            : Ptr {reinterpret_cast<uint64_t>(Base)}
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
            , PtrEnd {reinterpret_cast<uint64_t>(Base) + Size}
            , Size {Size}
#endif
            {}
        void* do_allocate( std::size_t bytes, std::size_t alignment ) override {
            uint64_t NewPtr = FEXCore::AlignUp((uint64_t)Ptr, alignment);
            Ptr = NewPtr + bytes;
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
            if (Ptr >= PtrEnd) {
              LogMan::Msg::AFmt("Fail: Only allocated: {} ({} this time) bytes. Tried allocating at ptr offset: {}.\n",
                Size,
                bytes,
                (uint64_t)(Ptr - (PtrEnd - Size)));
              FEX_TRAP_EXECUTION;
            }
#endif
            return reinterpret_cast<void*>(NewPtr);
        }

        void do_deallocate( void* p, std::size_t bytes, std::size_t alignment ) override {
            // noop
        }

        bool do_is_equal( const std::pmr::memory_resource& other ) const noexcept override {
            return this == &other;
        }
    private:
        uint64_t Ptr;
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
        uint64_t PtrEnd;
        size_t Size;
#endif
    };
  }
}
