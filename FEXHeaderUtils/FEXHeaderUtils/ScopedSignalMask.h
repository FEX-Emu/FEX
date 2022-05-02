#pragma once

#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/LogManager.h>

#include <cstdint>
#include <mutex>
#include <shared_mutex>
#include <signal.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace FHU {
  /**
   * @brief A class that masks signals and locks a mutex until it goes out of scope. It is NOT SAFE to move across threads.
   *
   * Constructor order:
   * 1) Mask signals
   * 2) Lock Mutex
   *
   * Destructor Order:
   * 1) Unlock Mutex
   * 2) Unmask signals
   *
   * Masking signals around mutex locks is needed for signal-reentrant safety
   */
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

  using ScopedSignalMaskWithMutex = ScopedSignalMaskWithMutexBase<std::mutex, &std::mutex::lock, &std::mutex::unlock>;
  using ScopedSignalMaskWithSharedLock = ScopedSignalMaskWithMutexBase<std::shared_mutex, &std::shared_mutex::lock_shared, &std::shared_mutex::unlock_shared>;
  using ScopedSignalMaskWithUniqueLock = ScopedSignalMaskWithMutexBase<std::shared_mutex, &std::shared_mutex::lock, &std::shared_mutex::unlock>;
}
