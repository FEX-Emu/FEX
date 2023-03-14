#pragma once

#include <atomic>
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/list.h>

#include <chrono>
#include <cstddef>
#include <mutex>
#include <sys/mman.h>

namespace FEXCore::Utils {
  /**
   * @brief An intrusive thread pool allocator
   *
   * Requires coordination between the allocator and its clients to efficiently share memory allocations between threads.
   *
   * The `Client` in this case referring to the location in code allocating a `MemoryBuffer` from the allocator.
   *   - The client must `Claim` a buffer to allocate it
   *   - In claiming a buffer, the allocator is passed a `BufferOwnedFlag` that is updated by both the allocator and client.
   *   - When the client is done with the buffer it must `Disown` or `Unclaim` the buffer.
   *     - `Disown` the buffer when it is expected to be used again soon.
   *       - This is relatively cheap.
   *     - `Unclaim` when the buffer won't be used again for an extended period.
   *       - This is expensive and requires a mutex shared between threads
   *     - `FixedSizePooledAllocation` helper class provided to help with this.
   *
   * Once the client has disowned a buffer then the allocator is free to reclaim the buffer when another thread is trying to `Claim` a new buffer.
   * The buffer getting claimed from a disowned client must have had its last use greater than the defined `DURATION` before it has a chance to get
   * reclaimed by the Allocator.
   *
   * During buffer reclaiming is also when unclaimed buffers get freed. This means active threads are able to clean up idle thread's unused memory.
   */
  class IntrusivePooledAllocator {
    public:
      template<typename T>
      struct AllocationInfo {
        T Ptr;
        size_t Size;
      };

      struct MemoryBuffer;
      /**
       * @brief Container for tracking the buffers
       *
       * We're using fextl::list explicitly because its iterators aren't invalidated when the list is adjusted.
       * if we had list types that we can atomically erase and append elements then unclaiming could be made cheaper.
       */
      using ContainerType = fextl::list<MemoryBuffer*>;
      /**
       * @brief steady_clock to ensure long running applications don't hit any timeskip problems.
       */
      using ClockType = std::chrono::steady_clock;
      /**
       * @brief Atomic flag state for letting the client know if it owns the buffer
       */
      enum class ClientFlags : uint32_t {
        FLAG_FREE = 0,
        FLAG_OWNED = 1,
        FLAG_DISOWNED = 3,
      };

      using BufferOwnedFlag = std::atomic<ClientFlags>;

      struct MemoryBuffer {
        void* Ptr;
        size_t Size;
        std::atomic<std::chrono::time_point<ClockType>> LastUsed;
        BufferOwnedFlag *CurrentClientOwnedFlag{};
      };
      // Ensure that the atomic objects of MemoryBuffer are lock free
      static_assert(decltype(MemoryBuffer::LastUsed){}.is_always_lock_free, "Oops, needs to be lock free");
      static_assert(std::remove_pointer<decltype(MemoryBuffer::CurrentClientOwnedFlag)>::type{}.is_always_lock_free, "Oops, needs to be lock free");

      /**
       * @brief Lets the client easily check if they own the buffer or not
       *
       * @param CurrentClientFlag Client owned flag
       *
       * @return Is the client buffer owned at the point of checking
       */
      static bool IsClientBufferOwned(BufferOwnedFlag &CurrentClientFlag) {
        return CurrentClientFlag.load() == ClientFlags::FLAG_OWNED;
      }

      /**
       * @brief Lets the client easily check if the buffer was freed
       *
       * @param CurrentClientFlag Client owned flag
       *
       * @return Is the client buffer owned at the point of checking
       */
      static bool IsClientBufferFree(BufferOwnedFlag &CurrentClientFlag) {
        return CurrentClientFlag.load() == ClientFlags::FLAG_FREE;
      }

      /**
       * @brief Allocates and claims a buffer that is tracked from the thread pool
       *
       * @param Size
       * @param CurrentClientFlag
       *
       * Once a buffer is claimed, the pool allocator can not reclaim this buffer until it is "Disowned"
       *
       * @return iterator to the internal tracking container
       */
      ContainerType::iterator ClaimBuffer(size_t Size, BufferOwnedFlag *CurrentClientFlag) {
        std::unique_lock lk {AllocationMutex};
        auto Buffer = ClaimBufferImpl(Size);
        (*Buffer)->CurrentClientOwnedFlag = CurrentClientFlag;
        CurrentClientFlag->store(ClientFlags::FLAG_OWNED);
        return Buffer;
      }

      /**
       * @brief Immediately release the buffer back to the allocator
       *
       * @param Buffer - The iterator that was previously given with ClaimBuffer
       *
       * Once this is called on a buffer then the pool allocator has full ownership of the buffer
       */
      void UnclaimBuffer(ContainerType::iterator Buffer) {
        std::unique_lock lk {AllocationMutex};
        (*Buffer)->CurrentClientOwnedFlag->store(ClientFlags::FLAG_FREE);
        UnclaimBufferImpl(Buffer);
      }

      /**
       * @brief Set internal flags of buffer claiming that the buffer is relinquished ownership
       *
       * @param Buffer - The iterator that was previously given with ClaimBuffer
       *
       * Once the buffer is disowned, the allocator can take back ownership of the buffer at any time
       *
       * Use ReownOrClaimBuffer if you want to attempt reusing a buffer being held on to.
       */
      void DisownBuffer(ContainerType::iterator Buffer) {
        // Client still owns the buffer but isn't using it
        // Allows us to claim it back if necessary
        (*Buffer)->LastUsed.store(ClockType::now(), std::memory_order_relaxed);
        (*Buffer)->CurrentClientOwnedFlag->store(ClientFlags::FLAG_DISOWNED);
      }

      /**
       * @brief Try to reown a buffer that we have previous disowned, failing that, claim a new buffer
       *
       * @param Buffer - The buffer we previously disowned
       * @param Size - The size of the buffer
       * @param CurrentClientFlag - The client tracked flag
       *
       * Once a DisownBuffer has been called, it is unsafe to use the buffer until it has been reowned
       * Always Reown a buffer after disowning it before use!
       *
       * @return Either the original buffer passed in if we managed to reclaim, or a new buffer if we couldn't
       */
      ContainerType::iterator ReownOrClaimBuffer(ContainerType::iterator Buffer, size_t Size, BufferOwnedFlag *CurrentClientFlag) {
        ClientFlags Expected = ClientFlags::FLAG_DISOWNED;
        if (CurrentClientFlag->compare_exchange_strong(Expected, ClientFlags::FLAG_OWNED)) {
          // If we managed to change the flag from DISOWNED to OWNED then we have successfully reclaimed
          // Finish setting up state
          (*Buffer)->LastUsed.store(ClockType::now(), std::memory_order_relaxed);
          return Buffer;
        }

        // Couldn't reclaim, just get a new buffer
        return ClaimBuffer(Size, CurrentClientFlag);
      }

      virtual ~IntrusivePooledAllocator() = default;

      // XXX: Is this a good amount?
      /**
       * @brief Duration before the allocator will reclaim buffers that the client claimed AND disowned
       *
       * Pool allocator will not attempt to reclaim client owned buffers, would be unsafe to do so.
       */
      constexpr static std::chrono::duration DURATION {std::chrono::seconds(5)};

    protected:
      IntrusivePooledAllocator() = default;

      ContainerType::iterator ClaimBufferImpl(size_t Size) {
        auto BuffersEnd = UnclaimedBuffers.end();
        ContainerType::iterator BestFit = BuffersEnd;
        ContainerType::iterator UnsizedFit = BuffersEnd;

        auto Now = ClockType::now();
        // Move any expired ClaimedBuffers to UnclaimedBuffers
        {
          // Spin the non-owned buffers and see if we can take ones past the period
          for (auto it = ClaimedBuffers.begin(); it != ClaimedBuffers.end();) {
            // 1) Can't take anything that the client has still claimed
            // 2) Needs to still be last used beyond our time threshold
            // 3) Only take the oldest buffer
            if ((*it)->CurrentClientOwnedFlag->load() == ClientFlags::FLAG_DISOWNED) {
              auto UsedTime = (*it)->LastUsed.load(std::memory_order_relaxed);
              if ((Now - UsedTime) >= DURATION) {
                ClientFlags Expected = ClientFlags::FLAG_DISOWNED;
                if ((*it)->CurrentClientOwnedFlag->compare_exchange_strong(Expected, ClientFlags::FLAG_FREE)) {
                  // We managed to take away ownership
                  // Put it back in the regular pool and come back to it
                  (*it)->CurrentClientOwnedFlag = nullptr;
                  UnclaimedBuffers.emplace_back(*it);
                  it = ClaimedBuffers.erase(it);
                  continue;
                }
              }
            }

            ++it;
          }
        }

        // Find an unclaimed buffer that is >= Size and Free up to one unclaimed buffer that has expired
        {
          // Walk all the allocations and find a buffer that fits
          for (auto it = UnclaimedBuffers.begin(); it != BuffersEnd; ++it) {
            if ((*it)->Size == Size) {
              BestFit = it;
              break;
            }

            if ((*it)->Size > Size) {
              UnsizedFit = it;
            }
          }

          // If we didn't have an exact fit then use an unsized fit
          if (BestFit == BuffersEnd) {
            BestFit = UnsizedFit;
          }

          // Free up to one unclaimed buffer that has expired
          {
            std::chrono::time_point<ClockType> LRUTime{};
            ContainerType::iterator LastUsed = BuffersEnd;

            // Walk all the allocations and find a buffer to erase
            for (auto it = UnclaimedBuffers.begin(); it != UnclaimedBuffers.end(); ++it) {
              // Ensure that the LRU value is past our duration threshold and isn't the one we are claiming
              // Also only select a single memory region
              if (it != BestFit) {
                auto UsedTime = (*it)->LastUsed.load(std::memory_order_relaxed);
                if ((Now - UsedTime) >= DURATION &&
                    UsedTime > LRUTime) {
                  LastUsed = it;
                  LRUTime = UsedTime;
                }
              }
            }

            // If we found a buffer then free it
            if (LastUsed != BuffersEnd) {
              Free((*LastUsed)->Ptr, (*LastUsed)->Size);
              delete *LastUsed;
              UnclaimedBuffers.erase(LastUsed);
            }
          }

          if (BestFit != UnclaimedBuffers.end()) {
            MemoryBuffer *Buffer = *BestFit;
            UnclaimedBuffers.erase(BestFit);
            return ClaimedBuffers.emplace(ClaimedBuffers.end(), Buffer);
          }
        }

        // Need to allocate a new buffer, couldn't fit
        auto Data = Alloc(Size);
        return ClaimedBuffers.emplace(ClaimedBuffers.end(), new MemoryBuffer{Data, Size, ClockType::now()});
      }

      void UnclaimBufferImpl(ContainerType::iterator Buffer) {
        (*Buffer)->CurrentClientOwnedFlag = nullptr;
        UnclaimedBuffers.emplace_back(*Buffer);
        ClaimedBuffers.erase(Buffer);
      }

      void FreeAllBuffers() {
        for (auto it : UnclaimedBuffers) {
          Free(it->Ptr, it->Size);
          delete it;
        }

        for (auto it : ClaimedBuffers) {
          Free(it->Ptr, it->Size);
          delete it;
        }

        UnclaimedBuffers.clear();
        ClaimedBuffers.clear();
      }

      /**
       * @brief List of buffers that this pool allocator itself owns
       */
      ContainerType UnclaimedBuffers;

      /**
       * @brief List of buffers that are client claimed
       */
      ContainerType ClaimedBuffers;

      /**
       * @brief Mutex to ensure thread safety while shuffling buffers around and allocating
       */
      std::mutex AllocationMutex;

    private:
      /**
       * @brief Allocates the buffer
       *
       * @param Size of the object to allocate
       *
       * @return pointer
       */
      virtual void *Alloc(size_t Size) = 0;
      /**
       * @brief Frees the buffer
       *
       * @param Ptr buffer pointer
       * @param Size buffer size
       */
      virtual void Free(void* Ptr, size_t Size) = 0;
  };

  /**
   * @brief Thread pool allocator that allocates and frees objects using malloc
   */
  class PooledAllocatorMalloc final : public IntrusivePooledAllocator {
    public:
      PooledAllocatorMalloc() = default;

      virtual ~PooledAllocatorMalloc() {
        FreeAllBuffers();
      }

    private:
      void *Alloc(size_t Size) override {
        return FEXCore::Allocator::malloc(Size);
      }

      void Free(void* Ptr, size_t Size) override {
        FEXCore::Allocator::free(Ptr);
      }
  };

  /**
   * @brief Thread pool allocator that allocates and frees objects that uses mmap
   */
  class PooledAllocatorMMap final : public IntrusivePooledAllocator {
    public:
      PooledAllocatorMMap() = default;

      virtual ~PooledAllocatorMMap() {
        FreeAllBuffers();
      }

    private:
      void *Alloc(size_t Size) override {
        return FEXCore::Allocator::mmap(0, Size,
          PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      }

      void Free(void* Ptr, size_t Size) override {
        FEXCore::Allocator::munmap(Ptr, Size);
      }
  };

  /**
   * @brief Wrapper around the pool allocator for delayed pool reclaiming
   *
   * This is expected to be used in high frequency buffer temporary usage.
   * Instead of quickly unclaiming and reclaiming the buffer while the the code is hot,
   * This instead will do the cheap operation of disowning the buffer until the code path cools down enough.
   * Once the code path stops disowning the codepath more times than `PeriodFrequency` during `PeriodMS` then
   * it will immediately unclaim.
   *
   * Implications:
   *   - The object will always be claimed for at *least* `PeriodFrequency`
   *   - The object will still *always* be disowned after each temporary use
   *     - This allows the pool allocator to reclaim a buffer from a sleeping thread
   *
   * Performance characteristics:
   *  - Disowning is cheap.
   *    - Last-used timestamp update
   *    - atomic_bool clear to signify it is disowned
   *
   *  - Reowning is relatively cheap (When buffer is still owned).
   *    - atomic_bool load to check if the object is still owned
   *      - atomic<uint32_t> CAS to change the object to `OWNED` state
   *        - Resolves a race condition where the `Allocator` can be in the process of reclaiming the buffer from the client
   *      - Last-used timestamp update
   *      - atomic_bool<relaxed> set to signify owned
   *      - atomic<uint32_t> set to change object to `OWNED` state
   *    - When object isn't owned, then allocate a new buffer from the pool
   *
   *  - Unclaiming is fairly costly
   *    - Requires owning a mutex, shared between all threads using the `Allocator`
   *    - Updating two fextl::list containers to give the ownership back to the `Allocator`
   *
   *  - Claiming is very costly
   *    - Requires owning a mutex, shared between all threads using the `Allocator`
   *    - Scans two fextl::list containers to find the best fit buffer
   *    - Or allocates another buffer when that fails
   *    - Frees stale buffers opportunistically
   */
  template<typename Type, size_t PeriodMS, size_t PeriodFrequency>
  class FixedSizePooledAllocation final {
    // If the delayed object reclaimer is more than the thread pool allocator's duration then the pool allocator would always need to reclaim the
    // buffer rather than giving it back.
    static_assert(std::chrono::duration(std::chrono::milliseconds(PeriodMS)) <= IntrusivePooledAllocator::DURATION,
      "DeplayedObjectReclaimer period needs to be lower or equal to the pool allocator duration");

    public:
      FixedSizePooledAllocation(IntrusivePooledAllocator &Allocator, size_t Size)
        : ThreadAllocator {Allocator}
        , Size {Size} {
      }

      /**
       * @brief Return the owned buffer or allocate another one from the `Allocator`
       *
       * The buffer returned isn't guaranteed to be the exact size of `Size` but it will be at least `Size`.
       * The contents of the memory returned isn't guaranteed to be zero initialized or not.
       * Not even guaranteed to contain the previous data from the previous reowning if the pointer is the same.
       *
       * @return object of type `Type` allocated with at least the size of `Size` from the constructor
       */
      Type ReownOrClaimBuffer() {
        if (!FEXCore::Utils::IntrusivePooledAllocator::IsClientBufferOwned(ClientOwnedFlag)) {
          Info = ThreadAllocator.ReownOrClaimBuffer(Info, Size, &ClientOwnedFlag);
        }

        // Putting a memset here is very handy for using thread sanitizer to find buffer usage races
        // Leaving this here for future excavation that will definitely occur here
        // memset((*Info)->Ptr, 0, Size);

        return reinterpret_cast<Type>((*Info)->Ptr);
      }

      /**
       * @brief Disown or unclaim the buffer, letting the `Allocator` know it can reclaim the buffer
       *
       * Once the `ReownOrClaimBuffer` function has been used, this must be called to let the `Allocator` know it is safe to reclaim a buffer.
       *
       * This will first Disown the buffer; which is cheap.
       *
       * If the frequency of use is below the threshold then immediately `UnclaimBuffer` so that `Allocator` can reuse it.
       */
      void DelayedDisownBuffer() {
        LOGMAN_THROW_A_FMT(FEXCore::Utils::IntrusivePooledAllocator::IsClientBufferOwned(ClientOwnedFlag),
          "Tried to disown buffer when client doesn't own it");

        // Always disown but not always unclaim
        // Disowning = cheap, unclaiming = expensive
        ThreadAllocator.DisownBuffer(Info);

        auto Now = std::chrono::steady_clock::now();
        if ((Now - Previous) >= std::chrono::duration(std::chrono::milliseconds(PeriodMS))) {
          if (CountPer < PeriodFrequency) {
            // Only unclaim the buffer if our buffer usage isn't excessive in the last period
            UnclaimBuffer();
          }
          CountPer = 0;
          Previous = Now;
        }
        ++CountPer;
      }

      /**
       * @brief Completely unclaim the buffer
       *
       * Useful if it is known that the buffer won't be used again for a period and can be given back
       * to the `Allocator` immediately.
       *
       * Necessary if an object is going to be freed from memory, so the `Allocator` can't update the `ClientOwnedFlag`
       *
       * Only use in that edge case! Otherwise use `DelayedDisownBuffer`
       */
      void UnclaimBuffer() {
        if (!FEXCore::Utils::IntrusivePooledAllocator::IsClientBufferFree(ClientOwnedFlag)) {
          ThreadAllocator.UnclaimBuffer(Info);
        }
      }

    private:
      // Thread allocator
      FEXCore::Utils::IntrusivePooledAllocator &ThreadAllocator;

      // Buffer size
      size_t Size;

      // Buffer ownership tracking
      FEXCore::Utils::IntrusivePooledAllocator::ContainerType::iterator Info{};
      FEXCore::Utils::IntrusivePooledAllocator::BufferOwnedFlag ClientOwnedFlag { FEXCore::Utils::IntrusivePooledAllocator::ClientFlags::FLAG_FREE };

      // Threshold counting
      uint64_t CountPer{};
      std::chrono::steady_clock::time_point Previous;
  };
}
