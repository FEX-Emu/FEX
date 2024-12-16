// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|ThreadManager
desc: Frontend thread management
$end_info$
*/

#pragma once

#include "LinuxSyscalls/Types.h"
#include "LinuxSyscalls/Seccomp/SeccompEmulator.h"

#include <FEXCore/Core/Context.h>
#include <FEXCore/fextl/vector.h>
#include <FEXCore/Utils/SignalScopeGuards.h>

#include <cstdint>
#include <linux/seccomp.h>

namespace FEX::HLE {
class SyscallHandler;
class SignalDelegatorBase;

// A latch that can be inspected to see if there is a waiter currently active.
// This allows us to remove the race condition between a thread trying to go asleep and something else telling it to go to sleep or wake up.
//
// Only one thread can ever wait on a latch, while another thread signals it.
class InspectableLatch final {
public:
  bool Wait(struct timespec* Timeout = nullptr) {
    while (true) {
      uint32_t Expected = HAS_NO_WAITER;
      const uint32_t Desired = HAS_WAITER;

      if (Mutex.compare_exchange_strong(Expected, Desired)) {
        // We have latched, now futex.
        constexpr int Op = FUTEX_WAIT | FUTEX_PRIVATE_FLAG;
        // WAIT will keep sleeping on the futex word while it is `val`
        int Result = ::syscall(SYS_futex, &Mutex, Op,
                               Desired, // val
                               Timeout, // Timeout/val2
                               nullptr, // Addr2
                               0);      // val3

        if (Timeout && Result == -1 && errno == ETIMEDOUT) {
          return false;
        }
      } else if (Expected == HAS_SIGNALED) {
        // Reset the latch once signaled
        Mutex.store(HAS_NO_WAITER);
        return true;
      }
    }
  }

  template<class Rep, class Period>
  bool WaitFor(const std::chrono::duration<Rep, Period>& time) {
    struct timespec Timeout {};
    auto SecondsDuration = std::chrono::duration_cast<std::chrono::seconds>(time);
    Timeout.tv_sec = SecondsDuration.count();
    Timeout.tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(time - SecondsDuration).count();
    return Wait(&Timeout);
  }

  void NotifyOne() {
    DoNotify(1);
  }

  bool HasWaiter() const {
    return Mutex.load() == HAS_WAITER;
  }

private:
  std::atomic<uint32_t> Mutex {};
  constexpr static uint32_t HAS_NO_WAITER = 0;
  constexpr static uint32_t HAS_WAITER = 1;
  constexpr static uint32_t HAS_SIGNALED = 2;

  void DoNotify(int Waiters) {
    uint32_t Expected = HAS_WAITER;
    const uint32_t Desired = HAS_SIGNALED;

    // If the mutex is in a waiting state and we have CAS exchanged it to HAS_SIGNALED, then execute a futex syscall.
    // otherwise just leave since nothing was waiting.
    if (Mutex.compare_exchange_strong(Expected, Desired)) {
      constexpr int Op = FUTEX_WAKE | FUTEX_PRIVATE_FLAG;

      ::syscall(SYS_futex, &Mutex, Op,
                Waiters, // val - Number of waiters to wake
                0,       // val2
                &Mutex,  // Addr2 - Mutex to do the operation on
                0);      // val3
    }
  }
};

enum class SignalEvent : uint32_t {
  Nothing, // If the guest uses our signal we need to know it was errant on our end
  Pause,
  Stop,
  Return,
  ReturnRT,
};

struct ThreadStateObject : public FEXCore::Allocator::FEXAllocOperators {
  struct DeferredSignalState {
    siginfo_t Info;
    int Signal;
  };

  FEXCore::Core::InternalThreadState* Thread;

  struct {
    uint32_t parent_tid;
    uint32_t PID;
    std::atomic<uint32_t> TID;
    int32_t* set_child_tid {0};
    int32_t* clear_child_tid {0};
    uint64_t robust_list_head {0};
  } ThreadInfo {};

  struct {
    SignalDelegatorBase* Delegator {};

    void* AltStackPtr {};
    stack_t GuestAltStack {
      .ss_sp = nullptr,
      .ss_flags = SS_DISABLE, // By default the guest alt stack is disabled
      .ss_size = 0,
    };
    // This is the thread's current signal mask
    FEX::HLE::GuestSAMask CurrentSignalMask {};
    // The mask prior to a suspend
    FEX::HLE::GuestSAMask PreviousSuspendMask {};

    uint64_t PendingSignals {};

    // Queue of thread local signal frames that have been deferred.
    // Async signals aren't guaranteed to be delivered in any particular order, but FEX treats them as FILO.
    fextl::vector<DeferredSignalState> DeferredSignalFrames;
  } SignalInfo {};

  // Seccomp thread specific data.
  uint32_t SeccompMode {SECCOMP_MODE_DISABLED};
  fextl::vector<FEX::HLE::SeccompEmulator::FilterInformation*> Filters {};

  // personality emulation.
  uint32_t persona {};

  FEXCore::Core::NonMovableUniquePtr<FEXCore::Threads::Thread> ExecutionThread;

  // Thread signaling information
  std::atomic<SignalEvent> SignalReason {SignalEvent::Nothing};

  // Thread pause handling
  InspectableLatch ThreadSleeping;

  // GDB signal information
  struct GdbInfoStruct {
    int Signal {};
    uint64_t SignalPC {};
    uint64_t GPRs[32];
    uint64_t PState {};
  };
  std::optional<GdbInfoStruct> GdbInfo;

  int StatusCode {};
};

class ThreadManager final {
public:

  ThreadManager(FEXCore::Context::Context* CTX, FEX::HLE::SyscallHandler* SyscallHandler, FEX::HLE::SignalDelegator* SignalDelegation)
    : CTX {CTX}
    , SyscallHandler {SyscallHandler}
    , SignalDelegation {SignalDelegation} {}

  ~ThreadManager();

  ///< Returns the ThreadStateObject from a CpuStateFrame object.
  static inline FEX::HLE::ThreadStateObject* GetStateObjectFromCPUState(FEXCore::Core::CpuStateFrame* Frame) {
    return static_cast<FEX::HLE::ThreadStateObject*>(Frame->Thread->FrontendPtr);
  }

  static inline FEX::HLE::ThreadStateObject* GetStateObjectFromFEXCoreThread(FEXCore::Core::InternalThreadState* Thread) {
    return static_cast<FEX::HLE::ThreadStateObject*>(Thread->FrontendPtr);
  }

  FEX::HLE::ThreadStateObject* CreateThread(uint64_t InitialRIP, uint64_t StackPointer, const FEXCore::Core::CPUState* NewThreadState = nullptr,
                                            uint64_t ParentTID = 0, FEX::HLE::ThreadStateObject* InheritThread = nullptr);
  void TrackThread(FEX::HLE::ThreadStateObject* Thread);

  void DestroyThread(FEX::HLE::ThreadStateObject* Thread, bool NeedsTLSUninstall = false);
  void StopThread(FEX::HLE::ThreadStateObject* Thread);
  void UnpauseThread(FEX::HLE::ThreadStateObject* Thread);

  void Pause();
  void Run();
  void Step();
  void Stop(bool IgnoreCurrentThread = false);

  void WaitForIdle();
  void WaitForIdleWithTimeout();
  void WaitForThreadsToRun();

  void SleepThread(FEXCore::Context::Context* CTX, FEX::HLE::ThreadStateObject* ThreadObject);
  void SleepThread(FEXCore::Context::Context* CTX, FEXCore::Core::CpuStateFrame* Frame) {
    auto ThreadObject = FEX::HLE::ThreadManager::GetStateObjectFromCPUState(Frame);
    SleepThread(CTX, ThreadObject);
  }

  void UnlockAfterFork(FEXCore::Core::InternalThreadState* Thread, bool Child);

  void IncrementIdleRefCount() {
    ++IdleWaitRefCount;
  }

  void InvalidateGuestCodeRange(FEXCore::Core::InternalThreadState* CallingThread, uint64_t Start, uint64_t Length) {
    std::lock_guard lk(ThreadCreationMutex);

    // Potential deferred since Thread might not be valid.
    // Thread object isn't valid very early in frontend's initialization.
    // To be more optimal the frontend should provide this code with a valid Thread object earlier.
    auto CodeInvalidationlk = GuardSignalDeferringSectionWithFallback(CTX->GetCodeInvalidationMutex(), CallingThread);

    for (auto& Thread : Threads) {
      CTX->InvalidateGuestCodeRange(Thread->Thread, Start, Length);
    }
  }

  void InvalidateGuestCodeRange(FEXCore::Core::InternalThreadState* CallingThread, uint64_t Start, uint64_t Length,
                                FEXCore::Context::CodeRangeInvalidationFn callback) {
    std::lock_guard lk(ThreadCreationMutex);

    // Potential deferred since Thread might not be valid.
    // Thread object isn't valid very early in frontend's initialization.
    // To be more optimal the frontend should provide this code with a valid Thread object earlier.
    auto CodeInvalidationlk = GuardSignalDeferringSectionWithFallback(CTX->GetCodeInvalidationMutex(), CallingThread);

    for (auto& Thread : Threads) {
      CTX->InvalidateGuestCodeRange(Thread->Thread, Start, Length, callback);
    }
  }

  FEXCore::ForkableUniqueMutex* GetThreadsCreationMutex() {
    return &ThreadCreationMutex;
  }

  const fextl::vector<FEX::HLE::ThreadStateObject*>* GetThreads() const {
    return &Threads;
  }

  size_t GetThreadCount() {
    std::lock_guard lk(ThreadCreationMutex);
    return Threads.size();
  }

private:
  FEXCore::Context::Context* CTX;
  FEX::HLE::SyscallHandler* SyscallHandler;
  FEX::HLE::SignalDelegator* SignalDelegation;

  FEXCore::ForkableUniqueMutex ThreadCreationMutex;
  fextl::vector<FEX::HLE::ThreadStateObject*> Threads;

  // Thread idling support.
  bool Running {};
  std::mutex IdleWaitMutex;
  std::condition_variable IdleWaitCV;
  std::atomic<uint32_t> IdleWaitRefCount {};

  void HandleThreadDeletion(FEX::HLE::ThreadStateObject* Thread, bool NeedsTLSUninstall = false);
  void NotifyPause();
};

} // namespace FEX::HLE
