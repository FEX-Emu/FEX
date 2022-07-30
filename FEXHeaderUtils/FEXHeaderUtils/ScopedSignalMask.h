#pragma once

#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/LogManager.h>

#include <cstdint>
#include <mutex>
#include <shared_mutex>
#include <signal.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <FEXCore/Core/SignalDelegator.h>

namespace FHU {

  struct ScopedSignalMask {
    ScopedSignalMask() { FEXCore::SignalDelegator::DeferThreadHostSignals(); }
    ~ScopedSignalMask() { FEXCore::SignalDelegator::DeliverThreadHostDeferredSignals(); }
  };

  struct ScopedSignalCheck {
    #if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
      ScopedSignalCheck() { FEXCore::SignalDelegator::AcquireHostDeferredSignals(); }
      ~ScopedSignalCheck() { FEXCore::SignalDelegator::ReleaseHostDeferredSignals(); }
    #endif
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
  struct ScopedSignalCheckWith: ScopedSignalCheck, T { using T::T; };

  using ScopedSignalCheckWithLockGuard = ScopedSignalCheckWith<std::lock_guard<std::mutex>>;

  using ScopedSignalCheckWithUniqueLockGuard = ScopedSignalCheckWith<std::lock_guard<std::shared_mutex>>;
  using ScopedSignalCheckWithUniqueLock = ScopedSignalCheckWith<std::unique_lock<std::shared_mutex>>;
  using ScopedSignalCheckWithSharedLock = ScopedSignalCheckWith<std::unique_lock<std::shared_mutex>>;
}