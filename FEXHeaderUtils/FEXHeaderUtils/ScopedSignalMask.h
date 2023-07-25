// SPDX-License-Identifier: MIT
#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>
#ifndef _WIN32
#include <signal.h>
#include <sys/syscall.h>
#endif
#include <unistd.h>

namespace FHU {
#ifndef _WIN32
  /**
   * Masks POSIX signals for the scope the object is active in
   */
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
   * while ensuring POSIX signals are masked while the mutex is locked
   *
   * Use this to prevent reentrancy issues of C++ mutexes with certain signal handlers.
   * Common examples of such issues are:
   * - C++ mutexes not unlocking due to a signal handler calling longjmp from within a scope owning the mutex
   * - The signal handler itself using a mutex that would be re-locked if the handler gets invoked
   *   again before unlocking
   *
   * Ownership of the returned object may be moved, but it is NOT SAFE to move across threads.
   */
  template<template<typename> class LockType = std::unique_lock, typename MutexType>
  [[nodiscard]] static auto MaskSignalsAndLockMutex(MutexType& mutex, uint64_t Mask = ~0ULL) {
#ifndef _WIN32
      // Signals are masked first, and then the lock is acquired
      struct {
          ScopedSignalMasker mask;
          LockType<MutexType> lock;
      } scope_guard { ScopedSignalMasker { Mask }, LockType<MutexType> { mutex } };
      return scope_guard;
#else
      // TODO: Doesn't block signals which may or may not cause issues.
      return LockType<MutexType> { mutex };
#endif
  }
}
