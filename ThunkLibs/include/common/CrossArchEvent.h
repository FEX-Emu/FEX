#pragma once

#include <cassert>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <condition_variable>
#include <mutex>
#include <list>
#include <type_traits>
#if defined(__clang__)
// If we're on clang then we can use C11 atomics directly in C++.
// If we are compiling with GCC then we need to use C++ Atomics with a wrapper.
#include <stdatomic.h>
#else
#include <atomic>
#define _Atomic(X) std::atomic<X>
static_assert(sizeof(std::atomic<uint32_t>) == 4, "Atomic needs to be size basic element size");
inline constexpr std::memory_order memory_order_acquire = std::memory_order_acquire;
#endif
#include <unistd.h>

// This is a cross-architecture event object.
// This is guaranteed to be four bytes long and work on x86-64, x86, and AArch64.
// Similar to a std::condition_variable. This can block multiple threads trying to consume work.
//
// `WaitForWorkFunc` is similar to `std::condition_variable::wait`
// `NotifyWorkFunc` is similar to `std::condition_variable::notify_all`

struct CrossArchEvent final {
  _Atomic(uint32_t) Futex = 0;
  static void WaitForWorkFunc(CrossArchEvent *Event) {
    // Wait for Futex value to become 1
    while (true) {
      // First step compare it with 1 already and see if we can early out
      uint32_t One = 1;
      if (atomic_compare_exchange_strong(&Event->Futex, &One, 0)) {
        return;
      }

      int Op = FUTEX_WAIT | FUTEX_PRIVATE_FLAG;
      [[maybe_unused]] int Res = syscall(SYS_futex,
          &Event->Futex,
          Op,
          nullptr, // Timeout
          nullptr, // Addr
          0);
    }
  }

  static void NotifyWorkFunc(CrossArchEvent *Event) {
    uint32_t Zero = 0;
    if (atomic_compare_exchange_strong(&Event->Futex, &Zero, 1)) {
      int Op = FUTEX_WAKE | FUTEX_PRIVATE_FLAG;
      syscall(SYS_futex,
        &Event->Futex,
        Op,
        nullptr, // Timeout
        nullptr, // Addr
        0);
    }
  }
};

static_assert(sizeof(CrossArchEvent) == 4, "Needs to be the size of a futex");
static_assert(std::is_standard_layout_v<CrossArchEvent>, "Needs to be standard layout");

#if !defined(NDEBUG)
// In debug builds, add some extra book-keeping and overhead to ensure `CrossArchMutex` is only
// ever locked and then unlocked on the same thread.
#define MUTEX_DEBUG
#endif

// This is a mutex that can be used across architectures.
// This is guaranteed to be four bytes long and work on x86-64, x86, and AArch64.
// Similar to a std::mutex, without a `try_lock`.
// This class only provides basic `Lock` and `Unlock` semantics.
// It /MUST/ remain as a basic standard layout type so it can be passed between architectures in thunks.
struct CrossArchMutex final {
  _Atomic(uint32_t) Futex = 0;

  static void Lock(CrossArchMutex *Mutex) {
    // Unique locking the futex means setting the mutex word from 0 to 1 atomically.
    // First try, do a userspace CAS to try and set it.
    // Failing that, do a kernel space futex to wait for the word to be set.
    // We then check in userspace again.

    // Lock value is 1 or tid depending on mode
#ifdef MUTEX_DEBUG
    const uint32_t LockValue = ::gettid();
#else
    constexpr uint32_t LockValue = 1;
#endif
    // Wait for Futex value to become 1
    while (true) {
      uint32_t FutexValue = 0;
      // First step compare it with LockValue already and see if we can early out
      if (atomic_compare_exchange_strong(&Mutex->Futex, &FutexValue, LockValue)) {
        return;
      }

      // Now wait for the futex word to wake us.
      int Op = FUTEX_WAIT | FUTEX_PRIVATE_FLAG;
      [[maybe_unused]] int Res = syscall(SYS_futex,
          &Mutex->Futex,
          Op,
          nullptr, // Timeout
          nullptr, // Addr
          0);
    }
  }

  static void Unlock(CrossArchMutex *Mutex) {
    // In a sanely written lock, we will own the lock and this is now going to unlock it.
#ifdef MUTEX_DEBUG
    const uint32_t LockValue = ::gettid();
    uint32_t FutexValue = atomic_load(&Mutex->Futex);
    if (LockValue != FutexValue) {
      assert(false && "Tried unlocking futex not owned by thread");
      __builtin_unreachable();
    }
#else
    constexpr uint32_t LockValue = 1;
#endif

    constexpr uint32_t UnlockValue = 0;
    while (true) {
      // Trying an unlock mutex by CAS LockValue(1/TID) -> 0
      uint32_t FutexValue = LockValue;
      if (atomic_compare_exchange_strong(&Mutex->Futex, &FutexValue, UnlockValue)) {
        // Wakeup anything that was waiting to lock
        int Op = FUTEX_WAKE | FUTEX_PRIVATE_FLAG;
        syscall(SYS_futex,
          &Mutex->Futex,
          Op,
          nullptr, // Timeout
          nullptr, // Addr
          0);
        return;
      }
      else {
        fprintf(stderr, "Failed to unlock mutex owned by thread? Expecting 0x%x, got 0x%x\n", LockValue, FutexValue);
        __builtin_unreachable();

      }
    }
  }
};

static_assert(sizeof(CrossArchMutex) == 4, "Needs to be the size of a futex");
static_assert(std::is_standard_layout_v<CrossArchMutex>, "Needs to be standard layout");

// WARNING!
// This class isn't able to be passed across the architecture boundary!
// WARNING!
// This is a std::unique_lock that uses the cross-architecture Mutex object.
// This object can be used on both sides of the boundary, but it can not be passed /across/ the boundary.
// This object itself can't be used across the architecture boundary, but the backing Mutex object can be.
class NonCrossArchMutexUniqueLock final {
  public:
  NonCrossArchMutexUniqueLock(CrossArchMutex* Mutex)
    : Mutex {Mutex} {
    CrossArchMutex::Lock(Mutex);
  }

  NonCrossArchMutexUniqueLock(CrossArchMutex &Mutex)
    : Mutex {&Mutex} {
    CrossArchMutex::Lock(&Mutex);
  }

  ~NonCrossArchMutexUniqueLock() {
    CrossArchMutex::Unlock(Mutex);
  }

  private:
  CrossArchMutex* Mutex;
};

// This is a cross-architecture queue object that operates as a ring-buffer FIFO and contains work items.
// This provides no threading guarantees so accessing members must be blocked by an external mutex.
// The FIFO uses a simple ring-buffer mechanism with a fixed size to ensure low enough overhead while operating.
// This structure has a guarantee to use 64-bit sized members to ensure a pointer passed between 32-bit and 64-bit
// architectures match sizes. 32-bit guest thunks will just ignore the upper 32-bits of the member in the case of a pointer.
//
// While the work items are arbitrary uint64_t values, it is expected that pointers will be passed between 32-bit and 64-bit.
// But the remaining upper 32-bit could /technically/ be abused for 32-bit guests.
//
struct CrossArchQueue {
  // 32 elements roughly arbitarily chosen.
  // 4 cachelines with 64-byte cachelines seemed like a nice number with low enough overhead.
  constexpr static size_t QueueDepth = 32;

  // These two values are for tracking the read and write pointers inside of the FIFO.
  _Atomic(int32_t) ReadIndex = 0;
  _Atomic(int32_t) WriteIndex = 0;
  // This is a mutex that is required when the FIFO is completely full and the `producer` threads
  // need to sleep until the `consumer` has consumed work items from the FIFO.
  //
  // Expectation that this won't happen frequently and if it does then it might be good to increase the queue depth.
  CrossArchEvent QueueFullWait{};

  using PtrType = uint64_t;
  // This is the ring-buffer holding the work queue. Simple buffer of uint64_t objects.
  PtrType WorkQueue[QueueDepth];

  // Returns the number of elements in the FIFO.
  static uint32_t Size(CrossArchQueue *Queue) {
    auto TempReadIndex = atomic_load_explicit(&Queue->ReadIndex, memory_order_acquire);
    auto ExpectedTempWriteIndex = atomic_load_explicit(&Queue->WriteIndex, memory_order_acquire);
    if (ExpectedTempWriteIndex < TempReadIndex) {
      return QueueDepth - (TempReadIndex - ExpectedTempWriteIndex);
    }
    else {
      return ExpectedTempWriteIndex - TempReadIndex;
    }
  }

  // Returns true if the FIFO has zero work items.
  // Easy check, just check if read and write indexes are equal.
  static bool Empty(CrossArchQueue *Queue) {
    auto TempReadIndex = atomic_load_explicit(&Queue->ReadIndex, memory_order_acquire);
    auto ExpectedTempWriteIndex = atomic_load_explicit(&Queue->WriteIndex, memory_order_acquire);
    return ExpectedTempWriteIndex == TempReadIndex;
  }

  // When FIFO is full, this will wait until there is a free entry.
  static void WaitForWorkConsumed(CrossArchQueue *Queue) {
    CrossArchEvent::WaitForWorkFunc(&Queue->QueueFullWait);
  }

  // Try to append a work item to the FIFO.
  // This will return false when the FIFO is full and the work wasn't appended.
  // The only reason why the work wouldn't get appended is if the FIFO is full.
  // In the case of full FIFO, then external Mutexes need to be unlocked and then
  // `WaitForWorkConsumed` needs to be called.
  // This ensures the modification mutex is unlocked for the `consumer` threads.
  template<typename T>
  static bool AppendWorkItem(CrossArchQueue *Queue, T Event) {
    while (true) {
      bool Wrapped = false;
      auto TempReadIndex = atomic_load_explicit(&Queue->ReadIndex, memory_order_acquire);
      auto ExpectedTempWriteIndex = atomic_load_explicit(&Queue->WriteIndex, memory_order_acquire);

      if ((ExpectedTempWriteIndex + 1) == QueueDepth) {
        // Wrap around
        Wrapped = true;

        if (TempReadIndex == 0) {
          // If we have wrapped around before the reader has started consuming
          // then we need to block.
          return false;
        }
      }
      else if ((ExpectedTempWriteIndex + 1) == TempReadIndex) {
        // If we have wrapped around before the reader has started consuming
        // then we need to block.
        return false;
      }

      // Safe to store the item now
      Queue->WorkQueue[ExpectedTempWriteIndex] = reinterpret_cast<PtrType>(Event);

      if (Wrapped) {
        // Wrote to zero index, so write index is at one now
        atomic_store(&Queue->WriteIndex, 0);
      }
      else {
        atomic_fetch_add(&Queue->WriteIndex, 1);
      }

      // Stored work
      return true;
    }
  }

  // Dequeue a work item from the FIFO.
  // This will return 0/nullptr when the FIFO is empty.
  // When there is work in the queue, then the `consumer` thread should consume work
  // until this returns 0/nullptr.
  // Once the work item is dequeued then it is no longer safe to access that member inside the queue.
  // A copy (which is just a uint64_t) is returned.
  template<typename T>
  static T DequeueWorkItem(CrossArchQueue *Queue) {
    // If FIFO is empty then early return 0/nullptr.
    if (CrossArchQueue::Empty(Queue)) {
      return reinterpret_cast<T>(0);
    }

    // Load the read index.
    // Load the work item off the FIFO.
    // Increment the read index to the next FIFO item.
    auto TempReadIndex = atomic_load(&Queue->ReadIndex);
    PtrType WorkItem = Queue->WorkQueue[TempReadIndex];
    auto Previous = atomic_fetch_add(&Queue->ReadIndex, 1);

    // If we are at maximum queue depth, wrap back around to zero.
    if ((Previous + 1) == QueueDepth) {
      // Overflow, wrap around
      atomic_store(&Queue->ReadIndex, 0);
    }

    // Potentially wake up anything waiting to add more work
    // Effectively "Free" if nothing is waiting.
    CrossArchEvent::NotifyWorkFunc(&Queue->QueueFullWait);

    return reinterpret_cast<T>(WorkItem);
  }
};

static_assert(std::is_standard_layout_v<CrossArchQueue>, "Needs to be standard layout");

// This is a cross-architecture work-queue delegator.
// This is the developer interface for passing work-queue items between architecture boundaries.
// This /can/ safely be used across the architecture boundary, but it can not be moved across the architecture boundary.
//
// `consumer` use case:
// while (!Shutdown) {
//  WaitForWork(Queue);
//  // Spin on all objects until 0/nullptr. Otherwise may miss events.
//  // `GetWorkEvent` returns 0/nullptr on empty
//  while (auto WQE = GetWorkEvent(Queue)) {
//    auto WorkItem = reinterpret_cast<void*>(WQE->WorkData); // Cast uint64_t to user defined type.
//    DoWork(WorkItem); ///< Do work.
//    CrossArchEvent::NotifyWorkFunc(&WQE->WorkCompleted); ///< Notify any potential waiter that the work is consumed.
//    // It is now unsafe to touch WQE.
//  }
// }
//
// `producer` use case:
//  CrossArchWorkQueueEvent WorkEvent{};
//  WorkEvent.WorkData = reinterpret_cast<uintptr_t>(&Work);
//
//  // Add the work event to the queue.
//  CrossArchWorkQueueDelegator::AddWorkEvent(WorkQueue, &WorkEvent);
//
//  // Wait for the work to be done.
//  CrossArchEvent::WaitForWorkFunc(&WorkEvent.WorkCompleted);
//  // It is now safe to cleanup `WorkEvent`

struct CrossArchWorkQueueDelegator {
  // This is a cross-architecture queue event object.
  // This contains the raw pointer from the `CrossArchQueue` FIFO.
  // Plus this contains a cross-architecture mutex to wake up any potential `producers` that the work item is completed.
  struct CrossArchWorkQueueEvent {
    CrossArchQueue::PtrType WorkData;
    CrossArchEvent WorkCompleted{};
  };

  // Mutex to hold while modifying work queue
  CrossArchMutex EventModifyMutex{};
  // Mutex to wait on for work.
  CrossArchEvent EventNotifyMutex{};
  // List of work events in a queue.
  CrossArchQueue Events{};

  CrossArchWorkQueueDelegator() = default;

  // This is not safe to move.
  CrossArchWorkQueueDelegator(CrossArchWorkQueueDelegator&&) = delete;

  // Wait for work to become available in the FIFO.
  static void WaitForWork(CrossArchWorkQueueDelegator *Queue) {
    CrossArchEvent::WaitForWorkFunc(&Queue->EventNotifyMutex);
  }

  // Notify work is available in the FIFO queue.
  // Called automatically from `AddWorkEvent`
  static void NotifyWork(CrossArchWorkQueueDelegator *Queue) {
    CrossArchEvent::NotifyWorkFunc(&Queue->EventNotifyMutex);
  }

  // Add a work item to the FIFO queue.
  // Blocking if the FIFO is full, until a space in the FIFO is open.
  static void AddWorkEvent(CrossArchWorkQueueDelegator *Queue, CrossArchWorkQueueEvent *Event) {
    while (true) {
      {
        // Don't hold the lock if we need to wait for full work queue.
        NonCrossArchMutexUniqueLock lk(Queue->EventModifyMutex);
        if (Queue->Events.AppendWorkItem(&Queue->Events, Event)) {
          break;
        }
      }
      CrossArchQueue::WaitForWorkConsumed(&Queue->Events);
    }

    CrossArchWorkQueueDelegator::NotifyWork(Queue);
  }

  // Dequeue a work item from the FIFO queue.
  // Returning nullptr if the queue was empty.
  static CrossArchWorkQueueEvent* GetWorkEvent(CrossArchWorkQueueDelegator *Queue) {
    CrossArchWorkQueueEvent* Event{};
    {
      NonCrossArchMutexUniqueLock lk(Queue->EventModifyMutex);
      Event = CrossArchQueue::DequeueWorkItem<CrossArchWorkQueueEvent*>(&Queue->Events);
    }

    return Event;
  }
};

static_assert(std::is_standard_layout_v<CrossArchWorkQueueDelegator>, "Needs to be standard layout");

//class CrossArchEvent final {
//  private:
//  /**
//   * @brief Literally just an atomic bool that we are using for this class
//   */
//  class Flag final {
//  public:
//    bool TestAndSet(bool SetValue = true) {
//      bool Expected = !SetValue;
//      return Value.compare_exchange_strong(Expected, SetValue);
//    }
//
//    bool TestAndClear() {
//      return TestAndSet(false);
//    }
//
//    bool Get() {
//      return Value.load();
//    }
//
//  private:
//    std::atomic_bool Value {false};
//  };
//
//  public:
//    ~CrossArchEvent() {
//      NotifyAll();
//    }
//    void NotifyOne() {
//      if (FlagObject.TestAndSet()) {
//        std::lock_guard<std::mutex> lk(MutexObject);
//        CondObject.notify_one();
//      }
//    }
//
//    void NotifyAll() {
//      if (FlagObject.TestAndSet()) {
//        std::lock_guard<std::mutex> lk(MutexObject);
//        CondObject.notify_all();
//      }
//    }
//
//    void Wait() {
//      // Have we signaled before we started waiting?
//      if (FlagObject.TestAndClear())
//        return;
//
//      std::unique_lock<std::mutex> lk(MutexObject);
//      CondObject.wait(lk, [this]{ return FlagObject.TestAndClear(); });
//    }
//
//    template<class Rep, class Period>
//    bool WaitFor(std::chrono::duration<Rep, Period> const& time) {
//      // Have we signaled before we started waiting?
//      if (FlagObject.TestAndClear())
//        return true;
//
//      std::unique_lock<std::mutex> lk(MutexObject);
//      bool DidSignal = CondObject.wait_for(lk, time, [this]{ return FlagObject.TestAndClear(); });
//      return DidSignal;
//    }
//    void BusyWaitForWork() {
//      while (!FlagObject.Get());
//    }
//
//  private:
//    std::mutex MutexObject;
//    std::condition_variable CondObject;
//    Flag FlagObject;
//};


