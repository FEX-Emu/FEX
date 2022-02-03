#pragma once
#include <FEXCore/Utils/LogManager.h>

#include <atomic>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <shared_mutex>
#include <unistd.h>

namespace FEXCore::Utils {
  // Shared locks have priority here
  // If a unique lock is pending but there are still shared locks then the shared locks always win
  class RefCountMutex final {
    public:
      void lock() {
        auto TryUniqueLock = [this]() -> std::pair<uint32_t, bool> {
          auto LocalFutex = Futex.load();

          if (LocalFutex) {
            // Refcount must be zero if we are to attempt getting a unique lock
          }
          else {
            // Try locking now in userspace
            while (LocalFutex == 0) {
              auto Desired = LocalFutex;
              Desired = UNIQUE_LOCK_VALUE;
              if (Futex.compare_exchange_strong(LocalFutex, Desired)) {
                // We have successfully unique locked
                //fprintf(stderr, "Unique Lock pulled: 0x%x\n", Desired);
                return std::make_pair(Desired, true);
              }
              else {
                if (LocalFutex == 0) {
                  // If another thread pulled the unique lock or the ref count incremented
                  // Then we need to wait, loop will end
                }
              }
            }
          }

          return std::make_pair(LocalFutex, false);
        };

        auto UniqueResult = TryUniqueLock();
        if (UniqueResult.second) {
          // Managed to get the unique lock
          return;
        }

        int Op = FUTEX_WAIT | FUTEX_PRIVATE_FLAG;

        do {
          ::syscall(SYS_futex,
            &Futex,
            Op,
            UniqueResult.first, // Value
            nullptr, // Timeout
            nullptr, // Addr
            0);

          UniqueResult = TryUniqueLock();
          // If Res == 0 then check the unique lock to see if unique is no longer owned
          if (UniqueResult.second) {
            // Unique lock succeeded
            return;
          }
        } while (true);
      }

      bool try_lock() {
        auto TryUniqueLock = [this]() -> std::pair<uint32_t, bool> {
          auto LocalFutex = Futex.load();

          if (LocalFutex) {
            // Refcount must be zero if we are to attempt getting a unique lock
          }
          else {
            // Try locking now in userspace
            while (LocalFutex == 0) {
              auto Desired = LocalFutex;
              Desired = UNIQUE_LOCK_VALUE;
              if (Futex.compare_exchange_strong(LocalFutex, Desired)) {
                // We have successfully unique locked
                //fprintf(stderr, "Unique Lock pulled: 0x%x\n", Desired);
                return std::make_pair(Desired, true);
              }
              else {
                if (LocalFutex == 0) {
                  // If another thread pulled the unique lock or the ref count incremented
                  // Then we need to wait, loop will end
                }
              }
            }
          }

          return std::make_pair(LocalFutex, false);
        };

        auto UniqueResult = TryUniqueLock();
        return UniqueResult.second;
      }

      void unlock() {
        LOGMAN_THROW_A_FMT(Futex.load() == UNIQUE_LOCK_VALUE, "Tried unlocking not locked mutex?");

        auto TryUniqueUnlock = [this]() -> std::pair<uint32_t, bool> {
          auto LocalFutex = Futex.load();

          if (LocalFutex != UNIQUE_LOCK_VALUE) {
            // Refcount must be zero if we are to attempt getting a unique lock
          }
          else {
            // Try locking now in userspace
            while (LocalFutex == UNIQUE_LOCK_VALUE) {
              auto Desired = LocalFutex;
              Desired = 0;
              if (Futex.compare_exchange_strong(LocalFutex, Desired)) {
                // We have successfully unique locked
                //fprintf(stderr, "Unique Unlock pulled: 0x%x\n", Desired);
                return std::make_pair(Desired, true);
              }
              else {
                if (LocalFutex == UNIQUE_LOCK_VALUE) {
                  // If another thread pulled the unique lock or the ref count incremented
                  // Then we need to wait, loop will end
                }
              }
            }
          }

          return std::make_pair(LocalFutex, false);
        };

        [[maybe_unused]] auto UniqueResult = TryUniqueUnlock();
        LOGMAN_THROW_A_FMT(UniqueResult.second, "Couldn't unlock mutex memory?");

        // We've now unlocked, use the futex to wake up any shared waiters
        int Op = FUTEX_WAKE | FUTEX_PRIVATE_FLAG;
        ::syscall(SYS_futex,
          &Futex,
          Op,
          INT_MAX, // Could be any number of shared waiters
          nullptr, // timeout
          nullptr, // addr
          0);
      }

      bool try_lock_shared() {
        auto TryRefIncrement = [this]() -> bool {
          auto LocalFutex = Futex.load();

          if (LocalFutex == UNIQUE_LOCK_VALUE) {
            // Unique lock held
          }
          else {
            // Try to increment the counter if unique lock isn't held
            while (LocalFutex != UNIQUE_LOCK_VALUE) {
              auto Desired = LocalFutex;
              Desired++;

              // Try to increment the ref count
              if (Futex.compare_exchange_strong(LocalFutex, Desired)) {
                // We have successfully incremented the ref counting mutex in userspace
                //fprintf(stderr, "Shared refcount incremented pulled: 0x%x\n", Desired);
                return true;
              }
              else {
                if (LocalFutex == UNIQUE_LOCK_VALUE) {
                  // Unique lock was held
                  // Nothing to do, loop will end
                }

                // Try again. Can happen in a race to increment the ref count
              }
            }
          }

          return false;
        };

        return TryRefIncrement();
      }

      void lock_shared() {
        auto TryRefIncrement = [this]() -> bool {
          auto LocalFutex = Futex.load();

          if (LocalFutex == UNIQUE_LOCK_VALUE) {
            // Unique lock held
          }
          else {
            // Try to increment the counter if unique lock isn't held
            while (LocalFutex != UNIQUE_LOCK_VALUE) {
              auto Desired = LocalFutex;
              Desired++;

              // Try to increment the ref count
              if (Futex.compare_exchange_strong(LocalFutex, Desired)) {
                // We have successfully incremented the ref counting mutex in userspace
                //fprintf(stderr, "Shared refcount incremented pulled: 0x%x\n", Desired);
                return true;
              }
              else {
                if (LocalFutex == UNIQUE_LOCK_VALUE) {
                  // Unique lock was held
                  // Nothing to do, loop will end
                }

                // Try again. Can happen in a race to increment the ref count
              }
            }
          }

          return false;
        };

        if (TryRefIncrement()) {
          return;
        }

        auto WaitForUniqueAndExecute = [this](auto Func) {
          // Unique lock was held. Wait until it is no longer held using a system futex
          int Op = FUTEX_WAIT | FUTEX_PRIVATE_FLAG;

          auto Expected = UNIQUE_LOCK_VALUE;
          do {
            ::syscall(SYS_futex,
              &Futex,
              Op,
              Expected, // Value
              nullptr, // Timeout,
              nullptr, // Addr
              0);

            Expected = Futex.load();
            // If Res == 0 then check the unique lock to see if unique is no longer owned
            if (Expected != UNIQUE_LOCK_VALUE) {
              if (Func()) {
                // Ref count succeeded
                return;
              }
            }
          } while (true);
        };


        WaitForUniqueAndExecute(TryRefIncrement);
      }

      // Number of ref counts remaining once this leaves
      uint32_t unlock_shared() {
        auto TryRefDecrement = [this]() -> std::pair<uint32_t, bool> {
          auto LocalFutex = Futex.load();

          if (LocalFutex == UNIQUE_LOCK_VALUE) {
            // Unique lock held
          }
          else {
            // Try to increment the counter if unique lock isn't held
            while (LocalFutex != UNIQUE_LOCK_VALUE) {
              auto Desired = LocalFutex;
              Desired--;

              // Try to increment the ref count
              if (Futex.compare_exchange_strong(LocalFutex, Desired)) {
                // We have successfully incremented the ref counting mutex in userspace
                //fprintf(stderr, "Shared refcount Decremented pulled: 0x%x\n", Desired);
                return std::make_pair(Desired, true);
              }
              else {
                if (LocalFutex == UNIQUE_LOCK_VALUE) {
                  // Unique lock was held
                  // Nothing to do, loop will end
                }

                // Try again. Can happen in a race to increment the ref count
              }
            }
          }

          return std::make_pair(LocalFutex, false);
        };

        auto DecrementResult = TryRefDecrement();
        if (DecrementResult.second) {
          if (DecrementResult.first == 0) {
            // If we were the last shared value out then we need to do a futex to wake up any waiters
            int Op = FUTEX_WAKE | FUTEX_PRIVATE_FLAG;
            ::syscall(SYS_futex,
              &Futex,
              Op,
              1, // Wake up only one thread if one is waiting. Which would be the unique waiter
              nullptr, // timeout
              nullptr, // addr
              0);
          }
          return DecrementResult.first;
        }

        LOGMAN_MSG_A_FMT("Managed to squeeze a unique lock between shared locks?");
        return 0; // Error
      }

      uint32_t GetNumRefCounts() const {
        return Futex.load();
      }

      // Be careful with this. Only use when you know the mutex is dead
      void Reset() {
        Futex.store(0);

        int Op = FUTEX_WAKE | FUTEX_PRIVATE_FLAG;
        ::syscall(SYS_futex,
          &Futex,
          Op,
          INT_MAX, // Wake up all threads if any waiting
          nullptr, // timeout
          nullptr, // addr
          0);
      }

    private:
      constexpr static uint32_t UNIQUE_LOCK_VALUE = -4096U;
      // -1 = unique_lock
      // 0 = no shared
      // >0 = shared waiters
      std::atomic<uint32_t> Futex;
  };

  template <typename T>
  class RefCountLockShared final {
    public:
      RefCountLockShared() = delete;
      RefCountLockShared(T &_Mutex)
        : Mutex {_Mutex} {
        Mutex.lock_shared();
      }

      ~RefCountLockShared() {
        Mutex.unlock_shared();
      }

    private:
      T &Mutex;
  };

  template <typename T>
  class RefCountLockUnique final {
    public:
      RefCountLockUnique() = delete;
      RefCountLockUnique(T &_Mutex)
        : Mutex {_Mutex} {
        Mutex.lock();
      }

      ~RefCountLockUnique() {
        Mutex.unlock();
      }

    private:
      T &Mutex;
  };

}

#if 1
using AOTMutex = FEXCore::Utils::RefCountMutex;
#else
using AOTMutex = std::shared_mutex;
#endif
