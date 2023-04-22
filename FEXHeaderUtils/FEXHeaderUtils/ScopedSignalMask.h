#pragma once

#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/LogManager.h>

#include <cstdint>
#include <mutex>
#include <shared_mutex>
#ifndef _WIN32
#include <signal.h>
#include <sys/syscall.h>
#endif
#include <unistd.h>

namespace FHU {
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
#ifndef _WIN32
  template<typename MutexType, void (MutexType::*lock_fn)(), void (MutexType::*unlock_fn)()>
  class ScopedSignalMaskWithMutexBase final {
    public:

      ScopedSignalMaskWithMutexBase(MutexType &_Mutex, uint64_t Mask = ~0ULL)
        : Mutex {&_Mutex} {
        // Mask all signals, storing the original incoming mask
        ::syscall(SYS_rt_sigprocmask, SIG_SETMASK, &Mask, &OriginalMask, sizeof(OriginalMask));

        // Lock the mutex
        (Mutex->*lock_fn)();
      }

      // No copy or assignment possible
      ScopedSignalMaskWithMutexBase(const ScopedSignalMaskWithMutexBase&) = delete;
      ScopedSignalMaskWithMutexBase& operator=(ScopedSignalMaskWithMutexBase&) = delete;

      // Only move
      ScopedSignalMaskWithMutexBase(ScopedSignalMaskWithMutexBase &&rhs)
       : OriginalMask {rhs.OriginalMask}, Mutex {rhs.Mutex} {
        rhs.Mutex = nullptr;
      }

      ~ScopedSignalMaskWithMutexBase() {
        if (Mutex != nullptr) {
          // Unlock the mutex
          (Mutex->*unlock_fn)();

          // Unmask back to the original signal mask
          ::syscall(SYS_rt_sigprocmask, SIG_SETMASK, &OriginalMask, nullptr, sizeof(OriginalMask));
        }
      }
    private:
      uint64_t OriginalMask{};
      MutexType *Mutex;
  };
#else
  // TODO: Doesn't block signals which may or may not cause issues.
  template<typename MutexType, void (MutexType::*lock_fn)(), void (MutexType::*unlock_fn)()>
  class ScopedSignalMaskWithMutexBase final {
    public:

      ScopedSignalMaskWithMutexBase(MutexType &_Mutex, [[maybe_unused]] uint64_t Mask = ~0ULL)
        : Mutex {&_Mutex} {
        // Lock the mutex
        (Mutex->*lock_fn)();
      }

      // No copy or assignment possible
      ScopedSignalMaskWithMutexBase(const ScopedSignalMaskWithMutexBase&) = delete;
      ScopedSignalMaskWithMutexBase& operator=(ScopedSignalMaskWithMutexBase&) = delete;

      // Only move
      ScopedSignalMaskWithMutexBase(ScopedSignalMaskWithMutexBase &&rhs)
       : Mutex {rhs.Mutex} {
        rhs.Mutex = nullptr;
      }

      ~ScopedSignalMaskWithMutexBase() {
        if (Mutex != nullptr) {
          // Unlock the mutex
          (Mutex->*unlock_fn)();
        }
      }
    private:
      MutexType *Mutex;
  };

#endif

  using ScopedSignalMaskWithMutex = ScopedSignalMaskWithMutexBase<std::mutex, &std::mutex::lock, &std::mutex::unlock>;
  using ScopedSignalMaskWithSharedLock = ScopedSignalMaskWithMutexBase<std::shared_mutex, &std::shared_mutex::lock_shared, &std::shared_mutex::unlock_shared>;
  using ScopedSignalMaskWithUniqueLock = ScopedSignalMaskWithMutexBase<std::shared_mutex, &std::shared_mutex::lock, &std::shared_mutex::unlock>;
}
