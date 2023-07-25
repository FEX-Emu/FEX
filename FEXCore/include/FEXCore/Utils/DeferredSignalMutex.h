// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Debug/InternalThreadState.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>
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
  class ForkableUniqueMutex final : public std::mutex {
    public:
      void StealAndDropActiveLocks() {
        LogMan::Msg::AFmt("{} is unsupported on WIN32 builds!", __func__);
      }
  };

  class ForkableSharedMutex final : public std::shared_mutex {
    public:
      void StealAndDropActiveLocks() {
        LogMan::Msg::AFmt("{} is unsupported on WIN32 builds!", __func__);
      }
  };
#endif

  class DeferredSignalRefCountGuard final {
    public:
      explicit DeferredSignalRefCountGuard(FEXCore::Core::InternalThreadState *Thread) : Thread(Thread) {
        // Needs to be atomic so that operations can't end up getting reordered around this.
        Thread->CurrentFrame->State.DeferredSignalRefCount.Increment(1);
      }

      // Move-only type
      DeferredSignalRefCountGuard(const DeferredSignalRefCountGuard&) = delete;
      DeferredSignalRefCountGuard& operator=(DeferredSignalRefCountGuard&) = delete;
      DeferredSignalRefCountGuard(DeferredSignalRefCountGuard&& rhs) : Thread(rhs.Thread) {
        rhs.Thread = nullptr;
      }

      ~DeferredSignalRefCountGuard() {
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
      }
    private:
      FEXCore::Core::InternalThreadState *Thread;
  };

#ifndef _WIN32
  // TODO: Duplicated, unify with ScopedSignalMask
  class ScopedSignalMasker final {
    public:
      explicit ScopedSignalMasker(uint64_t Mask) : OriginalMask(0) {
        // Mask all signals, storing the original incoming mask
        ::syscall(SYS_rt_sigprocmask, SIG_SETMASK, &Mask, &*OriginalMask, sizeof(*OriginalMask));
      }

      // Move-only type
      ScopedSignalMasker(const ScopedSignalMasker&) = delete;
      ScopedSignalMasker& operator=(ScopedSignalMasker&) = delete;
      ScopedSignalMasker(ScopedSignalMasker&& rhs) : OriginalMask(rhs.OriginalMask) {
        rhs.OriginalMask.reset();
      }

      ~ScopedSignalMasker() {
        if (OriginalMask) {
          ::syscall(SYS_rt_sigprocmask, SIG_SETMASK, &OriginalMask, nullptr, sizeof(*OriginalMask));
        }
      }
    private:
      std::optional<uint64_t> OriginalMask{};
  };
#endif

  /**
   * @brief Produces a wrapper object around a scoped lock of the given mutex
   * while bumping the Thread's deferred signal refcount while the mutex is
   * locked.
   */
  template<template<typename> class LockType = std::unique_lock, typename MutexType>
  [[nodiscard]] static auto GuardSignalDeferringSection(MutexType& mutex, FEXCore::Core::InternalThreadState *Thread, uint64_t Mask = ~0ULL) {
    // Refcount is incremented first, and then the lock is acquired.
    struct {
      std::optional<DeferredSignalRefCountGuard> refcount;
      LockType<MutexType> lock;
    } scope_guard = { DeferredSignalRefCountGuard { Thread }, LockType<MutexType> { mutex } };
    return scope_guard;
  }

  // Like GuardSignalDeferringSection but falls back to masking signals when Thread is nullptr
  template<template<typename> class LockType = std::unique_lock, typename MutexType>
  [[nodiscard]] static auto GuardSignalDeferringSectionWithFallback(MutexType& mutex, FEXCore::Core::InternalThreadState *Thread, uint64_t Mask = ~0ULL) {
    struct {
      std::optional<DeferredSignalRefCountGuard> refcount;
#ifndef _WIN32
      std::optional<ScopedSignalMasker> mask;
#endif
      LockType<MutexType> lock;
    } scope_guard;
    if (Thread) {
      scope_guard.refcount.emplace(Thread);
    } else {
#ifndef _WIN32
      scope_guard.mask.emplace(Mask);
#endif
    }
    scope_guard.lock = LockType<MutexType> { mutex };
    return scope_guard;
  }
}
