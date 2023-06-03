#pragma once

#include <FEXCore/Debug/InternalThreadState.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <shared_mutex>
#include <signal.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace FEXCore {

  template<typename MutexType, void (MutexType::*lock_fn)(), void (MutexType::*unlock_fn)()>
  class ScopedDeferredSignalWithMutexBase final {
    public:

      ScopedDeferredSignalWithMutexBase(MutexType &_Mutex, FEXCore::Core::InternalThreadState *Thread)
        : Mutex {&_Mutex}
        , Thread {Thread} {
        // Needs to be atomic so that operations can't end up getting reordered around this.
        Thread->CurrentFrame->State.DeferredSignalRefCount.Increment(1);
        // Lock the mutex
        (Mutex->*lock_fn)();
      }

      // No copy or assignment possible
      ScopedDeferredSignalWithMutexBase(const ScopedDeferredSignalWithMutexBase&) = delete;
      ScopedDeferredSignalWithMutexBase& operator=(ScopedDeferredSignalWithMutexBase&) = delete;

      // Only move
      ScopedDeferredSignalWithMutexBase(ScopedDeferredSignalWithMutexBase &&rhs)
        : Mutex {rhs.Mutex}
        , Thread {rhs.Thread} {
        rhs.Mutex = nullptr;
      }

      ~ScopedDeferredSignalWithMutexBase() {
        if (Mutex != nullptr) {
          // Unlock the mutex
          (Mutex->*unlock_fn)();

#ifdef _M_X86_64
          // Needs to be atomic so that operations can't end up getting reordered around this.
          // Without this, the recount and the signal access could get reordered.
          auto Result = Thread->CurrentFrame->State.DeferredSignalRefCount.Decrement(1);

          // X86-64 must do an additional check around the store.
          if ((Result - 1) == 0) {
            // Must happen after the refcount store
            Thread->CurrentFrame->State.DeferredSignalFaultAddress->Store(0);
          }
#else
          Thread->CurrentFrame->State.DeferredSignalRefCount.Decrement(1);
          Thread->CurrentFrame->State.DeferredSignalFaultAddress->Store(0);
#endif
        }
      }
    private:
      MutexType *Mutex;
      FEXCore::Core::InternalThreadState *Thread;
  };

  using ScopedDeferredSignalWithMutex      = ScopedDeferredSignalWithMutexBase<std::mutex, &std::mutex::lock, &std::mutex::unlock>;
  using ScopedDeferredSignalWithSharedLock = ScopedDeferredSignalWithMutexBase<std::shared_mutex, &std::shared_mutex::lock_shared, &std::shared_mutex::unlock_shared>;
  using ScopedDeferredSignalWithUniqueLock = ScopedDeferredSignalWithMutexBase<std::shared_mutex, &std::shared_mutex::lock, &std::shared_mutex::unlock>;

  template<typename MutexType, void (MutexType::*lock_fn)(), void (MutexType::*unlock_fn)()>
  class ScopedPotentialDeferredSignalWithMutexBase final {
    public:

      ScopedPotentialDeferredSignalWithMutexBase(MutexType &_Mutex, FEXCore::Core::InternalThreadState *Thread, uint64_t Mask = ~0ULL)
        : Mutex {&_Mutex}
        , Thread {Thread} {
        if (Thread) {
          Thread->CurrentFrame->State.DeferredSignalRefCount.Increment(1);
        }
        else {
          // Mask all signals, storing the original incoming mask
          ::syscall(SYS_rt_sigprocmask, SIG_SETMASK, &Mask, &OriginalMask, sizeof(OriginalMask));
        }
        // Lock the mutex
        (Mutex->*lock_fn)();
      }

      // No copy or assignment possible
      ScopedPotentialDeferredSignalWithMutexBase(const ScopedPotentialDeferredSignalWithMutexBase&) = delete;
      ScopedPotentialDeferredSignalWithMutexBase& operator=(ScopedPotentialDeferredSignalWithMutexBase&) = delete;

      // Only move
      ScopedPotentialDeferredSignalWithMutexBase(ScopedPotentialDeferredSignalWithMutexBase &&rhs)
        : Mutex {rhs.Mutex}
        , Thread {rhs.Thread} {
        rhs.Mutex = nullptr;
      }

      ~ScopedPotentialDeferredSignalWithMutexBase() {
        if (Mutex != nullptr) {
          // Unlock the mutex
          (Mutex->*unlock_fn)();

          if (Thread) {
#ifdef _M_X86_64
          // Needs to be atomic so that operations can't end up getting reordered around this.
          // Without this, the refcount and the signal access could get reordered.
          auto Result = Thread->CurrentFrame->State.DeferredSignalRefCount.Decrement(1);

          // X86-64 must do an additional check around the store.
          if ((Result - 1) == 0) {
            // Must happen after the refcount store
            Thread->CurrentFrame->State.DeferredSignalFaultAddress->Store(0);
          }
#else
          Thread->CurrentFrame->State.DeferredSignalRefCount.Decrement(1);
          Thread->CurrentFrame->State.DeferredSignalFaultAddress->Store(0);
#endif
          }
          else {
            // Unmask back to the original signal mask
            ::syscall(SYS_rt_sigprocmask, SIG_SETMASK, &OriginalMask, nullptr, sizeof(OriginalMask));
          }
        }
      }
    private:
      MutexType *Mutex;
      uint64_t OriginalMask{};
      FEXCore::Core::InternalThreadState *Thread;
  };

  using ScopedPotentialDeferredSignalWithMutex      = ScopedPotentialDeferredSignalWithMutexBase<std::mutex, &std::mutex::lock, &std::mutex::unlock>;
  using ScopedPotentialDeferredSignalWithSharedLock = ScopedPotentialDeferredSignalWithMutexBase<std::shared_mutex, &std::shared_mutex::lock_shared, &std::shared_mutex::unlock_shared>;
  using ScopedPotentialDeferredSignalWithUniqueLock = ScopedPotentialDeferredSignalWithMutexBase<std::shared_mutex, &std::shared_mutex::lock, &std::shared_mutex::unlock>;
}
