#pragma once

#include <FEXCore/Debug/InternalThreadState.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <shared_mutex>
#include <signal.h>
#ifndef _WIN32
#include <sys/syscall.h>
#endif
#include <unistd.h>

namespace FEXCore {
#ifndef _WIN32
  // Replacement for std::mutexes to deal with unlocking issues in the face of Linux fork() semantics.
  //
  // A fork() only clones the parent's calling thread. Other threads are silently dropped, which permanently leaves any mutexes owned by them locked.
  // To address this issue, ForkableUniqueMutex and ForkableSharedMutex provide a way to forcefully remove any dangling locks and reset the mutexes to their default state.
  class ForkableUniqueMutex final {
    public:
      ForkableUniqueMutex()
        : Mutex (PTHREAD_MUTEX_INITIALIZER) {
      }

      // Move-only type
      ForkableUniqueMutex(const ForkableUniqueMutex&) = delete;
      ForkableUniqueMutex& operator=(const ForkableUniqueMutex&) = delete;
      ForkableUniqueMutex(ForkableUniqueMutex &&rhs) = default;
      ForkableUniqueMutex& operator=(ForkableUniqueMutex &&) = default;

      void lock() {
        [[maybe_unused]] const auto Result = pthread_mutex_lock(&Mutex);
        LOGMAN_THROW_A_FMT(Result == 0, "{} failed to lock with {}", __func__, Result);
      }
      void unlock() {
        [[maybe_unused]] const auto Result = pthread_mutex_unlock(&Mutex);
        LOGMAN_THROW_A_FMT(Result == 0, "{} failed to unlock with {}", __func__, Result);
      }
      // Initialize the internal pthread object to its default initializer state.
      // Should only ever be used in the child process when a Linux fork() has occured.
      void StealAndDropActiveLocks() {
        Mutex = PTHREAD_MUTEX_INITIALIZER;
      }
    private:
      pthread_mutex_t Mutex;
  };

  class ForkableSharedMutex final {
    public:
      ForkableSharedMutex()
        : Mutex (PTHREAD_RWLOCK_INITIALIZER) {
      }

      // Move-only type
      ForkableSharedMutex(const ForkableSharedMutex&) = delete;
      ForkableSharedMutex& operator=(const ForkableSharedMutex&) = delete;
      ForkableSharedMutex(ForkableSharedMutex &&rhs) = default;
      ForkableSharedMutex& operator=(ForkableSharedMutex &&) = default;

      void lock() {
        [[maybe_unused]] const auto Result = pthread_rwlock_wrlock(&Mutex);
        LOGMAN_THROW_A_FMT(Result == 0, "{} failed to lock with {}", __func__, Result);
      }
      void unlock() {
        [[maybe_unused]] const auto Result = pthread_rwlock_unlock(&Mutex);
        LOGMAN_THROW_A_FMT(Result == 0, "{} failed to unlock with {}", __func__, Result);
      }
      void lock_shared() {
        [[maybe_unused]] const auto Result = pthread_rwlock_rdlock(&Mutex);
        LOGMAN_THROW_A_FMT(Result == 0, "{} failed to lock with {}", __func__, Result);
      }

      void unlock_shared() {
        unlock();
      }

      bool try_lock() {
        const auto Result = pthread_rwlock_trywrlock(&Mutex);
        return Result == 0;
      }

      bool try_lock_shared() {
        const auto Result = pthread_rwlock_tryrdlock(&Mutex);
        return Result == 0;
      }
      // Initialize the internal pthread object to its default initializer state.
      // Should only ever be used in the child process when a Linux fork() has occured.
      void StealAndDropActiveLocks() {
        Mutex = PTHREAD_RWLOCK_INITIALIZER;
      }
    private:
      pthread_rwlock_t Mutex;
  };
#else
  // Windows doesn't support forking, so these can be standard mutexes.
  class ForkableUniqueMutex final {
    public:
      ForkableUniqueMutex() = default;

      // Non-moveable
      ForkableUniqueMutex(const ForkableUniqueMutex&) = delete;
      ForkableUniqueMutex& operator=(const ForkableUniqueMutex&) = delete;
      ForkableUniqueMutex(ForkableUniqueMutex &&rhs) = delete;
      ForkableUniqueMutex& operator=(ForkableUniqueMutex &&) = delete;

      void lock() {
        Mutex.lock();
      }
      void unlock() {
        Mutex.unlock();
      }
      // Initialize the internal pthread object to its default initializer state.
      // Should only ever be used in the child process when a Linux fork() has occured.
      void StealAndDropActiveLocks() {
        LogMan::Msg::AFmt("{} is unsupported on WIN32 builds!", __func__);
      }
    private:
      std::mutex Mutex;
  };

  class ForkableSharedMutex final {
    public:
      ForkableSharedMutex() = default;

      // Non-moveable
      ForkableSharedMutex(const ForkableSharedMutex&) = delete;
      ForkableSharedMutex& operator=(const ForkableSharedMutex&) = delete;
      ForkableSharedMutex(ForkableSharedMutex &&rhs) = delete;
      ForkableSharedMutex& operator=(ForkableSharedMutex &&) = delete;

      void lock() {
        Mutex.lock();
      }
      void unlock() {
        Mutex.unlock();
      }
      void lock_shared() {
        Mutex.lock_shared();
      }

      void unlock_shared() {
        Mutex.unlock_shared();
      }

      bool try_lock() {
        return Mutex.try_lock();
      }

      bool try_lock_shared() {
        return Mutex.try_lock_shared();
      }
      // Initialize the internal pthread object to its default initializer state.
      // Should only ever be used in the child process when a Linux fork() has occured.
      void StealAndDropActiveLocks() {
        LogMan::Msg::AFmt("{} is unsupported on WIN32 builds!", __func__);
      }
    private:
      std::shared_mutex Mutex;
  };
#endif

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

  // Forkable variant
  using ScopedDeferredSignalWithForkableMutex      = ScopedDeferredSignalWithMutexBase<
    FEXCore::ForkableUniqueMutex,
    &FEXCore::ForkableUniqueMutex::lock,
    &FEXCore::ForkableUniqueMutex::unlock>;
  using ScopedDeferredSignalWithForkableSharedLock = ScopedDeferredSignalWithMutexBase<
    FEXCore::ForkableSharedMutex,
    &FEXCore::ForkableSharedMutex::lock_shared,
    &FEXCore::ForkableSharedMutex::unlock_shared>;
  using ScopedDeferredSignalWithForkableUniqueLock = ScopedDeferredSignalWithMutexBase<
    FEXCore::ForkableSharedMutex,
    &FEXCore::ForkableSharedMutex::lock,
    &FEXCore::ForkableSharedMutex::unlock>;

  class ScopedSignalMasker final {
    public:
      ScopedSignalMasker() = default;

      void Mask(uint64_t Mask) {
#ifndef _WIN32
        // Mask all signals, storing the original incoming mask
        ::syscall(SYS_rt_sigprocmask, SIG_SETMASK, &Mask, &OriginalMask, sizeof(OriginalMask));
#endif
      }

      // Move-only type
      ScopedSignalMasker(const ScopedSignalMasker&) = delete;
      ScopedSignalMasker& operator=(ScopedSignalMasker&) = delete;
      ScopedSignalMasker(ScopedSignalMasker &&rhs) = default;
      ScopedSignalMasker& operator=(ScopedSignalMasker &&) = default;

      void Unmask() {
#ifndef _WIN32
        ::syscall(SYS_rt_sigprocmask, SIG_SETMASK, &OriginalMask, nullptr, sizeof(OriginalMask));
#endif
      }
    private:
#ifndef _WIN32
      uint64_t OriginalMask{};
#endif
  };

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
          Masker.Mask(Mask);
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
            Masker.Unmask();
          }
        }
      }
    private:
      MutexType *Mutex;
      ScopedSignalMasker Masker;
      FEXCore::Core::InternalThreadState *Thread;
  };

  using ScopedPotentialDeferredSignalWithMutex      = ScopedPotentialDeferredSignalWithMutexBase<std::mutex, &std::mutex::lock, &std::mutex::unlock>;
  using ScopedPotentialDeferredSignalWithSharedLock = ScopedPotentialDeferredSignalWithMutexBase<std::shared_mutex, &std::shared_mutex::lock_shared, &std::shared_mutex::unlock_shared>;
  using ScopedPotentialDeferredSignalWithUniqueLock = ScopedPotentialDeferredSignalWithMutexBase<std::shared_mutex, &std::shared_mutex::lock, &std::shared_mutex::unlock>;

  // Forkable variant
  using ScopedPotentialDeferredSignalWithForkableMutex      = ScopedPotentialDeferredSignalWithMutexBase<
    FEXCore::ForkableUniqueMutex,
    &FEXCore::ForkableUniqueMutex::lock,
    &FEXCore::ForkableUniqueMutex::unlock>;
  using ScopedPotentialDeferredSignalWithForkableSharedLock = ScopedPotentialDeferredSignalWithMutexBase<
    FEXCore::ForkableSharedMutex,
    &FEXCore::ForkableSharedMutex::lock_shared,
    &FEXCore::ForkableSharedMutex::unlock_shared>;
  using ScopedPotentialDeferredSignalWithForkableUniqueLock = ScopedPotentialDeferredSignalWithMutexBase<
    FEXCore::ForkableSharedMutex,
    &FEXCore::ForkableSharedMutex::lock,
    &FEXCore::ForkableSharedMutex::unlock>;
}
