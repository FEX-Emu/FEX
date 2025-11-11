// SPDX-License-Identifier: MIT
#pragma once
#include <atomic>
#include <cstdint>

#if !defined(_WIN32)
#include <linux/futex.h> /* Definition of FUTEX_* constants */
#include <sys/syscall.h> /* Definition of SYS_* constants */
#include <unistd.h>
#else
#include <synchapi.h>
#endif

#include <FEXCore/Utils/LogManager.h>

#include "Utils/SpinWaitLock.h"

namespace FEXCore::Utils::WritePriorityMutex {

// A custom mutex that prioritizes exclusive locks.
// In highly contested scenarios, this can help minimize overall contention time.
//
// Features:
//  - Up to 32767 pending exclusive locks ("writers")
//  - Up to 32767 pending shared_locks ("readers")
//  - Low-overhead waiting via WFE with a fallback to futex on timeout
//  - Direct writer->reader hand-off and vice-versa to further reduce overhead
//
// Trade-offs:
//  - No guaranteed order of wake-ups besides prioritizing writers
//  - No support for recursive locking
//  - We can't use FUTEX_LOCK_PI to enable priority inheritance
class Mutex final {
public:
  Mutex() = default;

  // Move-only type
  Mutex(const Mutex&) = delete;
  Mutex& operator=(const Mutex&) = delete;
  Mutex(Mutex&& rhs) = delete;
  Mutex& operator=(Mutex&&) = delete;

  void lock() {
    // Try a non-blocking lock first.
    if (try_lock()) {
      return;
    }

    // Try a quick WFE write-lock.
    if (Attempt_WFE_WriteLock()) {
      return;
    }

    // Still couldn't get it. Start waiting.
    auto AtomicFutex = std::atomic_ref<uint32_t>(Futex);

    uint32_t Expected {};
    uint32_t Desired {};
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
    Expected = AtomicFutex.load(std::memory_order_relaxed);
    do {
      // Increment the number of write waiters.
      Desired = Expected + WRITE_WAITER_INCREMENT;

      LOGMAN_THROW_A_FMT((Desired & WRITE_WAITER_COUNT_MASK) != 0, "Overflow in write-waiters!");
    } while (AtomicFutex.compare_exchange_strong(Expected, Desired, std::memory_order_acq_rel, std::memory_order_acquire) == false);
#else
    Expected = AtomicFutex.fetch_add(WRITE_WAITER_INCREMENT);
    Desired = Expected + WRITE_WAITER_INCREMENT;
#endif

    // Thread added to waiter list.
    Expected = Desired;

    while (true) {
      bool Sleep = false;

      do {
        if ((Expected & WRITE_OWNED_BIT) == 0 && (Expected & READ_OWNER_COUNT_MASK) == 0) {
          // If not write-owned, and no read-owners, try to acquire.
          LOGMAN_THROW_A_FMT((Expected & WRITE_WAITER_COUNT_MASK) != 0, "Underflow in write-waiters!");

          // Add write-owned bit.
          Desired = Expected | WRITE_OWNED_BIT;

          // Remove ourselves from the wait list.
          Desired -= WRITE_WAITER_INCREMENT;

          Sleep = false;
        } else {
          // Already write-owned or read-locked. Go to sleep.
          Desired = Expected;
          Sleep = true;
          break;
        }

      } while (AtomicFutex.compare_exchange_strong(Expected, Desired, std::memory_order_acq_rel, std::memory_order_acquire) == false);

      if (!Sleep) {
        // Acquired early.
        LOGMAN_THROW_A_FMT((Desired & WRITE_OWNED_BIT) == WRITE_OWNED_BIT, "Somehow acquired a write-lock without it being set!");
        return;
      }
      FutexWaitForWriteAvailable(Desired);

      Expected = AtomicFutex.load(std::memory_order_relaxed);
    }
  }

  void lock_shared() {
    // Try an uncontended lock first.
    if (try_lock_shared()) {
      return;
    }

    // Try a quick WFE read-lock.
    if (Attempt_WFE_ReadLock()) {
      return;
    }

    auto AtomicFutex = std::atomic_ref<uint32_t>(Futex);

    uint32_t Expected = AtomicFutex.load(std::memory_order_relaxed);
    uint32_t Desired {};

    while (true) {

      bool Sleep = false;
      do {
        if ((Expected & WRITE_OWNED_BIT) == 0 && (Expected & WRITE_WAITER_COUNT_MASK) == 0) {
          // If no write-owner and no write-waiting, try and acquire.

          Desired = Expected + READ_OWNER_INCREMENT;
          LOGMAN_THROW_A_FMT((Desired & READ_OWNER_COUNT_MASK) != 0, "Overflow in read-owners!");
          Sleep = false;
        } else {
          // Waiting for lock to become available. Add to waiters.
          Desired = Expected | READ_WAITER_BIT;
          Sleep = true;
        }
      } while (AtomicFutex.compare_exchange_strong(Expected, Desired, std::memory_order_acq_rel, std::memory_order_acquire) == false);

      if (!Sleep) {
        // Acquired early.
        LOGMAN_THROW_A_FMT((Desired & WRITE_OWNED_BIT) != WRITE_OWNED_BIT, "Somehow read-locked and got a write lock!");
        return;
      }

      FutexWaitForReadAvailable(Desired);

      Expected = AtomicFutex.load(std::memory_order_relaxed);
    }
  }

  void unlock() {
    auto AtomicFutex = std::atomic_ref<uint32_t>(Futex);

    uint32_t Expected = AtomicFutex.load(std::memory_order_relaxed);
    uint32_t Desired {};
    do {
      LOGMAN_THROW_A_FMT((Expected & WRITE_OWNED_BIT) == WRITE_OWNED_BIT, "Trying to write-unlock something not write-locked!");
      // Remove the exclusive lock bit.
      Desired = Expected & ~WRITE_OWNED_BIT;

      // If no more writers, then make sure to clear the read-waiters bit as well.
      if ((Desired & WRITE_WAITER_COUNT_MASK) == 0) {
        Desired &= ~READ_WAITER_BIT;
      }
    } while (AtomicFutex.compare_exchange_strong(Expected, Desired, std::memory_order_acq_rel, std::memory_order_acquire) == false);

    // If success, then `Expected` has old value. Containing `READ_WAITER_BIT` which was just masked off, and also `WRITE_WAITER_COUNT_MASK`.
    if ((Expected & WRITE_WAITER_COUNT_MASK)) {
      // Handle write-write handoff.
      FutexWakeWriter();
    } else if ((Expected & READ_WAITER_BIT)) {
      // Handle write-reader handoff.
      FutexWakeReaders();
    }
  }

  void unlock_shared() {
    auto AtomicFutex = std::atomic_ref<uint32_t>(Futex);

    uint32_t Desired {};
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
    uint32_t Expected = AtomicFutex.load(std::memory_order_relaxed);
    do {
      LOGMAN_THROW_A_FMT((Expected & WRITE_OWNED_BIT) != WRITE_OWNED_BIT, "Trying to read-unlock something write-locked!");
      LOGMAN_THROW_A_FMT((Expected & READ_OWNER_COUNT_MASK) != 0, "Trying to read-unlock something not read-locked!");

      // Decrement the shared counter.
      Desired = Expected - READ_OWNER_INCREMENT;
    } while (AtomicFutex.compare_exchange_strong(Expected, Desired, std::memory_order_acq_rel, std::memory_order_acquire) == false);
#else
    Desired = AtomicFutex.fetch_sub(READ_OWNER_INCREMENT) - READ_OWNER_INCREMENT;
#endif

    // Handle read->write handoff if there are any waiting writers, and no readers left.
    if ((Desired & WRITE_WAITER_COUNT_MASK) && (Desired & READ_OWNER_COUNT_MASK) == 0) {
      FutexWakeWriter();
    }
  }

  bool try_lock() {
    auto AtomicFutex = std::atomic_ref<uint32_t>(Futex);

    uint32_t Expected = 0;

    // Try and grab the owned bit.
    uint32_t Desired = WRITE_OWNED_BIT;

    // try to CAS immediately.
    return AtomicFutex.compare_exchange_strong(Expected, Desired, std::memory_order_acq_rel, std::memory_order_acquire);
  }

  // Can race with other threads trying to lock shared!
  bool try_lock_shared() {
    auto AtomicFutex = std::atomic_ref<uint32_t>(Futex);
    uint32_t Expected = AtomicFutex.load(std::memory_order_relaxed);

    // Exclusively owned or has a list of waiting owners. Can't pass.
    if ((Expected & WRITE_OWNED_BIT) || (Expected & WRITE_WAITER_COUNT_MASK)) {
      return false;
    }

    // Try to add reader.
    uint32_t Desired = Expected + READ_OWNER_INCREMENT;
    LOGMAN_THROW_A_FMT((Desired & READ_OWNER_COUNT_MASK) != 0, "Overflow in read-owners!");

    // Uncontended mutex check
    return AtomicFutex.compare_exchange_strong(Expected, Desired, std::memory_order_acq_rel, std::memory_order_acquire);
  }

private:

#if !defined(_WIN32)
  void FutexWaitForWriteAvailable(uint32_t Expected) {
    ::syscall(SYS_futex, &Futex, FUTEX_PRIVATE_FLAG | FUTEX_WAIT_BITSET, Expected, nullptr, nullptr, FUTEX_BITSET_WAIT_WRITERS);
  }

  // Read-lock waiting for writers to drain out.
  void FutexWaitForReadAvailable(uint32_t Expected) {
    ::syscall(SYS_futex, &Futex, FUTEX_PRIVATE_FLAG | FUTEX_WAIT_BITSET, Expected, nullptr, nullptr, FUTEX_BITSET_WAIT_READERS);
  }

  // Read-Lock or Write-lock unlocked, wake one writer.
  // - Read->Write handoff.
  // - Write->Write handoff.
  void FutexWakeWriter() {
    ::syscall(SYS_futex, &Futex, FUTEX_PRIVATE_FLAG | FUTEX_WAKE_BITSET, 1, nullptr, nullptr, FUTEX_BITSET_WAIT_WRITERS);
  }

  // Write-lock unlocked, wake read-locks waiting.
  void FutexWakeReaders() {
    // Wake all readers.
    ::syscall(SYS_futex, &Futex, FUTEX_PRIVATE_FLAG | FUTEX_WAKE_BITSET, INT_MAX, nullptr, nullptr, FUTEX_BITSET_WAIT_READERS);
  }
#else
  // Writers wait for the full 32-bit futex.
  void FutexWaitForWriteAvailable(uint32_t Expected) {
    WaitOnAddress(&Futex, &Expected, sizeof(Futex), INFINITE);
  }

  // Readers wait for Futex bits [31:16] to be zero.
  void FutexWaitForReadAvailable(uint32_t Expected) {
    auto ReadWaiterAddress = reinterpret_cast<uint8_t*>(&Futex) + 2;
    uint16_t smol_Expected = Expected >> 16;
    WaitOnAddress(ReadWaiterAddress, &smol_Expected, sizeof(smol_Expected), INFINITE);
  }

  void FutexWakeWriter() {
    WakeByAddressSingle(&Futex);
  }

  void FutexWakeReaders() {
    auto ReadWaiterAddress = reinterpret_cast<uint8_t*>(&Futex) + 2;
    WakeByAddressAll(ReadWaiterAddress);
  }
#endif

  // Reuse the SpinWaitLock WFE implementations for read/write lock acquiring with WFE.
  // Can't reuse the spin-lock directly as some bit-representations are different.
  // WFE-write-lock is less likely to occur the more read-lock threads are participating. Can still occur so good to try.
  // WFE-read-lock is actually quite likely to succeed.
  // Return: true if the lock was acquired.
  bool Attempt_WFE_WriteLock() {
#ifdef _M_ARM_64
    const auto Begin = FEXCore::Utils::SpinWaitLock::GetCycleCounter();
    auto Now = Begin;
    const auto Duration = FEXCore::Utils::SpinWaitLock::CycleCounterFrequency / CYCLECOUNT_DIVISOR;

    auto AtomicFutex = std::atomic_ref<uint32_t>(Futex);
    uint32_t Expected = AtomicFutex.load(std::memory_order_relaxed);

    while ((Now - Begin) < Duration) {
      if (Expected == 0) {
        // Try and grab the owned bit.
        uint32_t Desired = WRITE_OWNED_BIT;

        if (AtomicFutex.compare_exchange_strong(Expected, Desired, std::memory_order_acq_rel, std::memory_order_acquire)) {
          return true;
        }
      }

      // One-shot attempt to wait for mask to be zero.
      Expected = FEXCore::Utils::SpinWaitLock::OneShotWFEBitComparison(&Futex, ~0U, 0U);
      Now = FEXCore::Utils::SpinWaitLock::GetCycleCounter();
    }
#endif

    return false;
  }

  // Return: true if the lock was acquired.
  bool Attempt_WFE_ReadLock() {
#ifdef _M_ARM_64
    // Spin on a WFE for a short-amount of time, waiting for write-owned and writer-count to be zero.
    //  - Attempt to acquire read-lock at that point.
    //  - Don't add read-waiters bit on failure, return false.
    const auto Begin = FEXCore::Utils::SpinWaitLock::GetCycleCounter();
    auto Now = Begin;
    const auto Duration = FEXCore::Utils::SpinWaitLock::CycleCounterFrequency / CYCLECOUNT_DIVISOR;

    auto AtomicFutex = std::atomic_ref<uint32_t>(Futex);
    uint32_t Expected = AtomicFutex.load(std::memory_order_relaxed);
    uint32_t Desired {};

    while ((Now - Begin) < Duration) {
      if ((Expected & WRITE_OWNED_BIT) == 0 && (Expected & WRITE_WAITER_COUNT_MASK) == 0) {
        // If no write-owner and no write-waiting, try and acquire.

        Desired = Expected + READ_OWNER_INCREMENT;
        LOGMAN_THROW_A_FMT((Desired & READ_OWNER_COUNT_MASK) != 0, "Overflow in read-owners!");
        if (AtomicFutex.compare_exchange_strong(Expected, Desired, std::memory_order_acq_rel, std::memory_order_acquire)) {
          return true;
        }
      }

      // One-shot attempt to wait for mask to be zero.
      Expected = FEXCore::Utils::SpinWaitLock::OneShotWFEBitComparison(&Futex, WRITE_OWNED_BIT | WRITE_WAITER_COUNT_MASK, 0U);
      Now = FEXCore::Utils::SpinWaitLock::GetCycleCounter();
    }
#endif

    return false;
  }

  constexpr static uint32_t WRITE_OWNED_BIT = 1U << 31;
  constexpr static uint32_t READ_WAITER_BIT = 1U << 15;
  constexpr static uint32_t WRITE_WAITER_OFFSET = 16;
  constexpr static uint32_t WRITE_WAITER_INCREMENT = 1U << WRITE_WAITER_OFFSET;
  constexpr static uint32_t READ_OWNER_INCREMENT = 1;

  // Count masks
  constexpr static uint32_t WRITE_WAITER_COUNT_MASK = 0x7FFFU << WRITE_WAITER_OFFSET;
  constexpr static uint32_t READ_OWNER_COUNT_MASK = 0x7FFFU;

  // Independent futex bit-set masks.
  // Wait for readers to drain.
  constexpr static uint32_t FUTEX_BITSET_WAIT_READERS = 1U << 0;
  // Wait for writers to drain.
  constexpr static uint32_t FUTEX_BITSET_WAIT_WRITERS = 1U << 1;

  // Only spin on WFE for 0.01ms (10k ns).
  constexpr static uint64_t CYCLECOUNT_DIVISOR = 1'000'000'000ULL / 10'000U;

  // Layout:
  //    Bits[31]: Write-lock bit.
  // Bits[30:16]: Write-waiter count.
  //    Bits[15]: Read-waiter bit.
  //  Bits[14:0]: Read-owner count.
  uint32_t Futex {};
};
} // namespace FEXCore::Utils::WritePriorityMutex
