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
  class ScopedSignalMaskWithMutex final {
    public:

      ScopedSignalMaskWithMutex(std::mutex &_Mutex, uint64_t Mask = ~0ULL)
        : Mutex {&_Mutex} {
        // Mask all signals, storing the original incoming mask
        ::syscall(SYS_rt_sigprocmask, SIG_SETMASK, &Mask, &OriginalMask, sizeof(OriginalMask));

        // Lock the mutex
        Mutex->lock();
      }

      // No copy or assignment possible
      ScopedSignalMaskWithMutex(const ScopedSignalMaskWithMutex&) = delete;
      ScopedSignalMaskWithMutex& operator=(ScopedSignalMaskWithMutex&) = delete;

      // Only move
      ScopedSignalMaskWithMutex(ScopedSignalMaskWithMutex &&rhs)
       : OriginalMask {rhs.OriginalMask}, Mutex {rhs.Mutex} {
        rhs.Mutex = nullptr;
      }

      ~ScopedSignalMaskWithMutex() {
        if (Mutex != nullptr) {
          // Unlock the mutex
          Mutex->unlock();

          // Unmask back to the original signal mask
          ::syscall(SYS_rt_sigprocmask, SIG_SETMASK, &OriginalMask, nullptr, sizeof(OriginalMask));
        }
      }
    private:
      uint64_t OriginalMask{};
      std::mutex *Mutex;
  };

  /**
   * @brief A class that masks signals and shared locks a shared mutex until it goes out of scope. It is NOT SAFE to move across threads.
   *
   * Constructor order:
   * 1) Mask signals
   * 2) Lock Mutex
   *
   * Destructor Order:
   * 1) Unlock Mutex
   * 2) Unmask signals
   *
   * Masking signals around mutex locks is needed for signal-rentrant safety
   */
  class ScopedSignalMaskWithSharedLock final {
    public:
      ScopedSignalMaskWithSharedLock(std::shared_mutex &_Mutex, uint64_t Mask = ~0ULL)
        : Mutex {&_Mutex} {
        // Mask all signals, storing the original incoming mask
        ::syscall(SYS_rt_sigprocmask, SIG_SETMASK, &Mask, &OriginalMask, sizeof(OriginalMask));

        // Lock the mutex
        Mutex->lock_shared();
      }

      // No copy or assignment possible
      ScopedSignalMaskWithSharedLock(const ScopedSignalMaskWithSharedLock&) = delete;
      ScopedSignalMaskWithSharedLock& operator=(ScopedSignalMaskWithSharedLock&) = delete;

      // Only move
      ScopedSignalMaskWithSharedLock(ScopedSignalMaskWithSharedLock &&rhs)
       : OriginalMask {rhs.OriginalMask}, Mutex {rhs.Mutex} {
        rhs.Mutex = nullptr;
      }

      ~ScopedSignalMaskWithSharedLock() {
        if (Mutex) {
          // Unlock the mutex
          Mutex->unlock_shared();
          
          // Unmask back to the original signal mask
          ::syscall(SYS_rt_sigprocmask, SIG_SETMASK, &OriginalMask, nullptr, sizeof(OriginalMask));
        }
      }
    private:
      uint64_t OriginalMask{};
      std::shared_mutex *Mutex;
  };


  /**
   * @brief A class that masks signals and unique locks a shared mutex until it goes out of scope. It is NOT SAFE to move across threads.
   *
   * Constructor order:
   * 1) Mask signals
   * 2) Lock Mutex
   *
   * Destructor Order:
   * 1) Unlock Mutex
   * 2) Unmask signals
   *
   * Masking signals around mutex locks is needed for signal-rentrant safety
   */
  class ScopedSignalMaskWithUniqueLock final {
    public:
      ScopedSignalMaskWithUniqueLock(std::shared_mutex &_Mutex, uint64_t Mask = ~0ULL)
        : Mutex {&_Mutex} {
        // Mask all signals, storing the original incoming mask
        ::syscall(SYS_rt_sigprocmask, SIG_SETMASK, &Mask, &OriginalMask, sizeof(OriginalMask));

        // Lock the mutex
        Mutex->lock();
      }

      // No copy or assignment possible
      ScopedSignalMaskWithUniqueLock(const ScopedSignalMaskWithSharedLock&) = delete;
      ScopedSignalMaskWithUniqueLock& operator=(ScopedSignalMaskWithSharedLock&) = delete;

      ScopedSignalMaskWithUniqueLock(ScopedSignalMaskWithUniqueLock &&rhs)
        : OriginalMask {rhs.OriginalMask}, Mutex {rhs.Mutex} {
        rhs.Mutex = nullptr;
      }

      ~ScopedSignalMaskWithUniqueLock() {
        if (Mutex != nullptr)
        {
          // Unlock the mutex
          Mutex->unlock();
          
          // Unmask back to the original signal mask
          ::syscall(SYS_rt_sigprocmask, SIG_SETMASK, &OriginalMask, nullptr, sizeof(OriginalMask));
        }
      }
    private:
      uint64_t OriginalMask{};
      std::shared_mutex *Mutex;
  };
}