#include <catch2/catch.hpp>
#include <chrono>
#include <csetjmp>
#include <FEXCore/Utils/InterruptableConditionVariable.h>
#include <FEXHeaderUtils/Syscalls.h>

#include <thread>
#include <signal.h>

// Test that ensures the Reentrant mutex will timeout without signaling
TEST_CASE("SimpleWait") {
  auto Dur = std::chrono::seconds(1);
  FEXCore::InterruptableConditionVariable Mutex{};

  auto Now = std::chrono::high_resolution_clock::now();
  bool Signaled = Mutex.WaitFor(Dur);
  auto End = std::chrono::high_resolution_clock::now();

  // We weren't signaled
  REQUIRE(Signaled == false);

  // We waited at least the full duration
  REQUIRE((End - Now) >= Dur);
}

void WaitThread(FEXCore::InterruptableConditionVariable *Mutex, bool *Signaled) {
  auto Dur = std::chrono::seconds(5);
  *Signaled = Mutex->WaitFor(Dur);
}

// Test that ensure the Reentrant mutex will signal without timing out
TEST_CASE("SignaledWait") {
  bool Signaled{};
  FEXCore::InterruptableConditionVariable Mutex{};

  std::thread t(WaitThread, &Mutex, &Signaled);

  auto Now = std::chrono::high_resolution_clock::now();
  Mutex.NotifyAll();
  auto End = std::chrono::high_resolution_clock::now();

  t.join();

  auto Dur = std::chrono::seconds(5);
  // Expected to signal
  REQUIRE(Signaled);
  // Ensure we didn't timeout
  REQUIRE((End - Now) < Dur);
}

static jmp_buf LongJump{};
static int32_t NumberOfJumps{};
FEXCore::InterruptableConditionVariable WaitMutex{};

void SignalHandler(int Signal) {
  ++NumberOfJumps;
  longjmp(LongJump, 1);
}

void WaitThreadLongJump(
  FEXCore::InterruptableConditionVariable *Mutex,
  FEXCore::InterruptableConditionVariable *ThreadReadyMutex,
  bool *Signaled,
  int32_t *TID) {

  // Store the TID
  *TID = FHU::Syscalls::gettid();

  // Setup a long jump signal handler
  struct sigaction sa{};
  sa.sa_flags = SA_RESTART | SA_NODEFER;
  sigemptyset(&sa.sa_mask);
  sa.sa_handler = SignalHandler;
  sigaction(SIGUSR1, &sa, nullptr);

  // long jump here
  int Value = setjmp(LongJump);

  if (Value == 0) {
    // Only notify that we are ready once
    ThreadReadyMutex->NotifyAll();
  }

  // Notify the loop that we are ready for signaling again
  WaitMutex.NotifyAll();

  // Time out after two seconds
  auto Dur = std::chrono::seconds(2);
  *Signaled = Mutex->WaitFor(Dur);
}

// Test that ensures the Reentrant mutex survives over a long jump
// Without signaling the mutex
TEST_CASE("SignaledWaitLongJumpNoSignal") {
  int32_t TID{};
  bool Signaled{};
  FEXCore::InterruptableConditionVariable Mutex{};
  FEXCore::InterruptableConditionVariable ThreadReadyMutex{};

  NumberOfJumps = 0;
  std::thread t(WaitThreadLongJump, &Mutex, &ThreadReadyMutex, &Signaled, &TID);

  // Wait for our thread to become ready
  ThreadReadyMutex.Wait();

  int32_t NumberOfJumpsToDo = 5;
  for (int32_t i = 0; i < NumberOfJumpsToDo; ++i) {
    // Wait for the thread to signal that it is ready to receive signal
    WaitMutex.WaitFor(std::chrono::milliseconds(500));
    // Send the signal to the thread
    FHU::Syscalls::tgkill(::getpid(), TID, SIGUSR1);
  }

  // Wait for thread join
  t.join();

  // We never signaled, so we should never receive signal
  REQUIRE(Signaled == false);
  // Ensure we long jumped the correct number of times
  REQUIRE(NumberOfJumps == NumberOfJumpsToDo);
}

// Test that ensures the Reentrant mutex survives over a long jump
// With signaling the mutex
TEST_CASE("SignaledWaitLongJumpSignal") {
  int32_t TID{};
  bool Signaled{};
  FEXCore::InterruptableConditionVariable Mutex{};
  FEXCore::InterruptableConditionVariable ThreadReadyMutex{};

  NumberOfJumps = 0;
  std::thread t(WaitThreadLongJump, &Mutex, &ThreadReadyMutex, &Signaled, &TID);

  // Wait for our thread to become ready
  ThreadReadyMutex.Wait();

  int32_t NumberOfJumpsToDo = 5;
  for (int32_t i = 0; i < NumberOfJumpsToDo; ++i) {
    // Wait for the thread to signal that it is ready to receive signal
    WaitMutex.WaitFor(std::chrono::milliseconds(500));

    // Send the signal to the thread
    FHU::Syscalls::tgkill(::getpid(), TID, SIGUSR1);
  }

  // Notify the thread's mutex now
  Mutex.NotifyAll();

  // Wait for thread join
  t.join();

  // We signaled so we should have received it now
  REQUIRE(Signaled);
  // Ensure we long jumped the correct number of times
  REQUIRE(NumberOfJumps == NumberOfJumpsToDo);
}
