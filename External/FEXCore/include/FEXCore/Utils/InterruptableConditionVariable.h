#pragma once

#include <atomic>
#include <chrono>
#include <climits>
#include <cstdint>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace FEXCore {
  /**
   * @brief A condition variable that is robust against use of longjmp in signal handlers.
   *
   * This is opposed to common `std::condition_variable` implementations:
   * Longjmp'ing in a signal handler while interrupting a pending `wait_for()`
   * call can leave the condition variable in an invalid state that breaks later
   * uses of that object and may cause hangs as a consequence.
   */
  class InterruptableConditionVariable final {
    public:
      bool Wait(struct timespec *Timeout = nullptr) {
        while (true) {
          uint32_t Expected = SIGNALED;
          uint32_t Desired = UNSIGNALED;

          // If the mutex was already signaled then we can early exit
          if (Mutex.compare_exchange_strong(Expected, Desired)) {
            return true;
          }

          constexpr int Op = FUTEX_WAIT | FUTEX_PRIVATE_FLAG;
          // WAIT will keep sleeping on the futex word while it is `val`
          int Result = ::syscall(SYS_futex,
            &Mutex,
            Op,
            Desired, // val
            Timeout, // Timeout/val2
            nullptr, // Addr2
            0); // val3

          if (Timeout && Result == -1 && errno == ETIMEDOUT) {
            return false;
          }
        }
      }

      template<class Rep, class Period>
      bool WaitFor(std::chrono::duration<Rep, Period> const& time) {
        struct timespec Timeout{};
        auto SecondsDuration = std::chrono::duration_cast<std::chrono::seconds>(time);
        Timeout.tv_sec = SecondsDuration.count();
        Timeout.tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(time - SecondsDuration).count();
        return Wait(&Timeout);
      }

      void NotifyOne() {
        DoNotify(1);
      }

      void NotifyAll() {
        // Maximum number of waiters
        DoNotify(INT_MAX);
      }

    private:
      std::atomic<uint32_t> Mutex{};
      constexpr static uint32_t SIGNALED = 1;
      constexpr static uint32_t UNSIGNALED = 0;

      void DoNotify(int Waiters) {
        uint32_t Expected = UNSIGNALED;
        uint32_t Desired = SIGNALED;

        // If the mutex was in an unsignaled state then signal
        if (Mutex.compare_exchange_strong(Expected, Desired)) {
          constexpr int Op = FUTEX_WAKE | FUTEX_PRIVATE_FLAG;

          ::syscall(SYS_futex,
            &Mutex,
            Op,
            Waiters, // val - Number of waiters to wake
            0,       // val2
            &Mutex,  // Addr2 - Mutex to do the operation on
            0);    // val3
        }
      }
  };
}
