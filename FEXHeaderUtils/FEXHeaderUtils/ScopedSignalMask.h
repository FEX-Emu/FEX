#pragma once

#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/LogManager.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <shared_mutex>
#include <signal.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <FEXCore/Core/SignalDelegator.h>

namespace FHU {

  struct ScopedSignalHostDefer {
    ScopedSignalHostDefer() { FEXCore::SignalDelegator::DeferThreadHostSignals(); }
    ~ScopedSignalHostDefer() { FEXCore::SignalDelegator::DeliverThreadHostDeferredSignals(); }
  };

  struct ScopedSignalAutoHostDefer {
    ScopedSignalAutoHostDefer() { FEXCore::SignalDelegator::EnterAutoHostDefer(); }
    ~ScopedSignalAutoHostDefer() { FEXCore::SignalDelegator::LeaveAutoHostDefer(); }
  };

  struct ScopedSignalMask {
    std::atomic<bool> HasAcquired;
    ScopedSignalMask() { HasAcquired.store(FEXCore::SignalDelegator::AcquireHostDeferredSignals(), std::memory_order_relaxed); }
    ~ScopedSignalMask() { if (HasAcquired.load(std::memory_order_relaxed)) { FEXCore::SignalDelegator::ReleaseHostDeferredSignals(); } }

    ScopedSignalMask(const ScopedSignalMask&) = delete;
    ScopedSignalMask& operator=(ScopedSignalMask&) = delete;
    ScopedSignalMask(ScopedSignalMask &&rhs): HasAcquired(rhs.HasAcquired.exchange(false, std::memory_order_relaxed)) { }
  };

  /**
   * @brief A drop-in replacement for std::lock_guard that masks POSIX signals while the mutex is locked
   *
   * Use this class to prevent reentrancy issues of C++ mutexes with certain signal handlers.
   * Common examples of such issues are:
   * - C++ mutexes not unlocking due to a signal handler longjmping out of a scope owning the mutex
   * - The signal handler itself using a mutex that would be re-locked if the handler gets invoked
   *   again before unlocking
   *
   * Ownership of this object may be moved, but it is NOT SAFE to move across threads.
   *
   * Constructor order:
   * 1) Mask signals
   * 2) Lock Mutex
   *
   * Destructor Order:
   * 1) Unlock Mutex
   * 2) Unmask signals
   */
  template<typename T>
  struct ScopedSignalMaskWith: ScopedSignalMask, T { using T::T; };

  using ScopedSignalMaskWithLockGuard = ScopedSignalMaskWith<std::lock_guard<std::mutex>>;

  using ScopedSignalMaskWithUniqueLockGuard = ScopedSignalMaskWith<std::lock_guard<std::shared_mutex>>;
  using ScopedSignalMaskWithUniqueLock = ScopedSignalMaskWith<std::unique_lock<std::shared_mutex>>;
  using ScopedSignalMaskWithSharedLock = ScopedSignalMaskWith<std::unique_lock<std::shared_mutex>>;
}