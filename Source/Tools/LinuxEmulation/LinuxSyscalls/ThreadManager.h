// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|ThreadManager
desc: Frontend thread management
$end_info$
*/

#pragma once

#include "Common/Profiler.h"

#include "LinuxSyscalls/Types.h"
#include "LinuxSyscalls/Seccomp/SeccompEmulator.h"

#include <FEXCore/Core/Context.h>
#include <FEXCore/fextl/vector.h>
#include <FEXCore/Utils/Profiler.h>
#include <FEXCore/Utils/SignalScopeGuards.h>

#include <cstdint>
#include <linux/seccomp.h>

namespace FEX::HLE {
class SyscallHandler;
class SignalDelegator;

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
    SignalDelegator* Delegator {};

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
  std::atomic_bool ThreadSleeping {false};
  FEXCore::InterruptableConditionVariable ThreadPaused;

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

  ThreadManager(FEXCore::Context::Context* CTX, FEX::HLE::SignalDelegator* SignalDelegation)
    : CTX {CTX}
    , SignalDelegation {SignalDelegation} {}

  ~ThreadManager();

  class StatAlloc final : public FEX::Profiler::StatAllocBase {
  public:
    StatAlloc();

    void LockBeforeFork();
    void UnlockAfterFork(FEXCore::Core::InternalThreadState* Thread, bool Child);

    void CleanupForExit();

    FEXCore::Profiler::ThreadStats* AllocateSlot(uint32_t TID);
    void DeallocateSlot(FEXCore::Profiler::ThreadStats* AllocatedSlot);

  private:
    void Initialize();

    uint32_t FrontendAllocateSlots(uint32_t NewSize) override;
    FEX_CONFIG_OPT(ProfileStats, PROFILESTATS);
    FEX_CONFIG_OPT(Is64BitMode, IS64BIT_MODE);

    constexpr static int USER_PERMS = S_IRWXU | S_IRWXG | S_IRWXO;
    FEXCore::ForkableUniqueMutex StatMutex;
  };

  void CleanupForExit() {
    Stat.CleanupForExit();
  }

  StatAlloc Stat;

  ///< Returns the ThreadStateObject from a CpuStateFrame object.
  static inline FEX::HLE::ThreadStateObject* GetStateObjectFromCPUState(FEXCore::Core::CpuStateFrame* Frame) {
    return static_cast<FEX::HLE::ThreadStateObject*>(Frame->Thread->FrontendPtr);
  }

  static inline FEX::HLE::ThreadStateObject* GetStateObjectFromFEXCoreThread(FEXCore::Core::InternalThreadState* Thread) {
    return static_cast<FEX::HLE::ThreadStateObject*>(Thread->FrontendPtr);
  }

  FEX::HLE::ThreadStateObject* CreateThread(uint64_t InitialRIP, uint64_t StackPointer, const FEXCore::Core::CPUState* NewThreadState = nullptr,
                                            uint64_t ParentTID = 0, FEX::HLE::ThreadStateObject* InheritThread = nullptr);
  void TrackThread(FEX::HLE::ThreadStateObject* Thread) {
    std::lock_guard lk(ThreadCreationMutex);
    Threads.emplace_back(Thread);
  }

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

  void SleepThread(FEXCore::Context::Context* CTX, FEXCore::Core::CpuStateFrame* Frame);

  void LockBeforeFork();
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

  const fextl::vector<FEX::HLE::ThreadStateObject*>* GetThreads() const {
    return &Threads;
  }

private:
  FEXCore::Context::Context* CTX;
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
  FEX_CONFIG_OPT(ProfileStats, PROFILESTATS);
};

} // namespace FEX::HLE
