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

  // Marks a scope as deferring host signals
  struct ScopedSignalHostDefer {
    ScopedSignalHostDefer() { FEXCore::SignalDelegator::DeferThreadHostSignals(); }
    ~ScopedSignalHostDefer() { FEXCore::SignalDelegator::DeliverThreadHostDeferredSignals(); }
  };

  // Marks a scope as deferring host signals whenever inside a nested ScopedSignalMask
  struct ScopedSignalAutoHostDefer {
    ScopedSignalAutoHostDefer() { FEXCore::SignalDelegator::EnterAutoHostDefer(); }
    ~ScopedSignalAutoHostDefer() { FEXCore::SignalDelegator::LeaveAutoHostDefer(); }
  };

  // Marks a scope as requiring host deferred signals
  struct ScopedSignalMask {
    std::atomic<bool> HasAcquired;
    ScopedSignalMask() { HasAcquired.store(FEXCore::SignalDelegator::AcquireHostDeferredSignals(), std::memory_order_relaxed); }
    ~ScopedSignalMask() { if (HasAcquired.exchange(false, std::memory_order_relaxed)) { FEXCore::SignalDelegator::ReleaseHostDeferredSignals(); } }

    ScopedSignalMask(const ScopedSignalMask&) = delete;
    ScopedSignalMask& operator=(ScopedSignalMask&) = delete;
    ScopedSignalMask(ScopedSignalMask &&rhs): HasAcquired(rhs.HasAcquired.exchange(false, std::memory_order_relaxed)) { }
  };

  // Marks as requiring host deferred signals before T's ctor, and til after T's dtor
  template<typename T>
  struct ScopedSignalMaskWith: ScopedSignalMask, T { using T::T; };

  // std::lock_guard<std::mutex> drop in replacement
  using ScopedSignalMaskWithLockGuard = ScopedSignalMaskWith<std::lock_guard<std::mutex>>;

  // std::lock_guard<std::shared_mutex> drop in replacement
  using ScopedSignalMaskWithUniqueLockGuard = ScopedSignalMaskWith<std::lock_guard<std::shared_mutex>>;
  // std::unique_lock<std::shared_mutex> drop in replacement
  using ScopedSignalMaskWithUniqueLock = ScopedSignalMaskWith<std::unique_lock<std::shared_mutex>>;
  // std::shared_lock<std::shared_mutex> drop in replacement
  using ScopedSignalMaskWithSharedLock = ScopedSignalMaskWith<std::shared_lock<std::shared_mutex>>;
}