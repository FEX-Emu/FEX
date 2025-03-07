// SPDX-License-Identifier: MIT
#include "Utils/SpinWaitLock.h"
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <thread>

constexpr auto SleepAmount = std::chrono::milliseconds(250);

TEST_CASE("FutexSpin-Timed-8bit") {
  uint8_t Test {};

  auto now = std::chrono::high_resolution_clock::now();
  FEXCore::Utils::SpinWaitLock::Wait(&Test, 1, SleepAmount);
  auto end = std::chrono::high_resolution_clock::now();
  auto diff = end - now;

  // The futex spinwait needs to have slept for at /least/ the amount specified. It will always run slightly late.
  REQUIRE(std::chrono::duration_cast<std::chrono::nanoseconds>(diff) >= std::chrono::duration_cast<std::chrono::nanoseconds>(SleepAmount));
}

TEST_CASE("FutexSpin-Sleep-8bit") {
  constexpr auto SleepAmount = std::chrono::seconds(1);

  uint8_t Test {};
  std::atomic<uint8_t> ActualSpinLoop {};
  std::chrono::nanoseconds SleptAmount;

  std::thread t([&Test, &SleptAmount, &ActualSpinLoop]() {
    auto now = std::chrono::high_resolution_clock::now();
    ActualSpinLoop.store(1);
    FEXCore::Utils::SpinWaitLock::Wait(&Test, 1);
    auto end = std::chrono::high_resolution_clock::now();
    SleptAmount = end - now;
  });

  // Wait until the second thread lets us know to stop waiting sleeping.
  while (ActualSpinLoop.load() == 0)
    ;

  // sleep this thread for the sleep amount.
  std::this_thread::sleep_for(SleepAmount);

  // Set the futex
  FEXCore::Utils::SpinWaitLock::lock(&Test);

  // Wait for the thread to get done.
  t.join();

  // The futex spinwait needs to have slept for at /least/ the amount specified. It will always run slightly late.
  REQUIRE(SleptAmount >= std::chrono::duration_cast<std::chrono::nanoseconds>(SleepAmount));
}

TEST_CASE("FutexSpin-Timed-16bit") {
  uint16_t Test {};

  auto now = std::chrono::high_resolution_clock::now();
  FEXCore::Utils::SpinWaitLock::Wait(&Test, 1, SleepAmount);
  auto end = std::chrono::high_resolution_clock::now();
  auto diff = end - now;

  // The futex spinwait needs to have slept for at /least/ the amount specified. It will always run slightly late.
  REQUIRE(std::chrono::duration_cast<std::chrono::nanoseconds>(diff) >= std::chrono::duration_cast<std::chrono::nanoseconds>(SleepAmount));
}

TEST_CASE("FutexSpin-Timed-32bit") {
  uint32_t Test {};

  auto now = std::chrono::high_resolution_clock::now();
  FEXCore::Utils::SpinWaitLock::Wait(&Test, 1, SleepAmount);
  auto end = std::chrono::high_resolution_clock::now();
  auto diff = end - now;

  // The futex spinwait needs to have slept for at /least/ the amount specified. It will always run slightly late.
  REQUIRE(std::chrono::duration_cast<std::chrono::nanoseconds>(diff) >= std::chrono::duration_cast<std::chrono::nanoseconds>(SleepAmount));
}

TEST_CASE("FutexSpin-Timed-64bit") {
  uint64_t Test {};

  auto now = std::chrono::high_resolution_clock::now();
  FEXCore::Utils::SpinWaitLock::Wait(&Test, 1, SleepAmount);
  auto end = std::chrono::high_resolution_clock::now();
  auto diff = end - now;

  // The futex spinwait needs to have slept for at /least/ the amount specified. It will always run slightly late.
  REQUIRE(std::chrono::duration_cast<std::chrono::nanoseconds>(diff) >= std::chrono::duration_cast<std::chrono::nanoseconds>(SleepAmount));
}
