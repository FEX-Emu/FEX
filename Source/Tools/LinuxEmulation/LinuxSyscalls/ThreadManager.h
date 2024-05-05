// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|ThreadManager
desc: Frontend thread management
$end_info$
*/

#pragma once

#include <FEXCore/Core/Context.h>
#include <FEXCore/fextl/vector.h>
#include <FEXCore/Utils/SignalScopeGuards.h>

#include <cstdint>

namespace FEX::HLE {
class SyscallHandler;
class SignalDelegator;


class ThreadManager final {
public:
  ThreadManager(FEXCore::Context::Context* CTX, FEX::HLE::SignalDelegator* SignalDelegation)
    : CTX {CTX}
    , SignalDelegation {SignalDelegation} {}

  ~ThreadManager();

  FEXCore::Core::InternalThreadState*
  CreateThread(uint64_t InitialRIP, uint64_t StackPointer, FEXCore::Core::CPUState* NewThreadState = nullptr, uint64_t ParentTID = 0);
  void TrackThread(FEXCore::Core::InternalThreadState* Thread) {
    std::lock_guard lk(ThreadCreationMutex);
    Threads.emplace_back(Thread);
  }

  void DestroyThread(FEXCore::Core::InternalThreadState* Thread);
  void StopThread(FEXCore::Core::InternalThreadState* Thread);
  void RunThread(FEXCore::Core::InternalThreadState* Thread);

  void Pause();
  void Run();
  void Step();
  void Stop(bool IgnoreCurrentThread = false);

  void WaitForIdle();
  void WaitForIdleWithTimeout();
  void WaitForThreadsToRun();

  void SleepThread(FEXCore::Context::Context* CTX, FEXCore::Core::CpuStateFrame* Frame);

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
      CTX->InvalidateGuestCodeRange(Thread, Start, Length);
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
      CTX->InvalidateGuestCodeRange(Thread, Start, Length, callback);
    }
  }

  const fextl::vector<FEXCore::Core::InternalThreadState*>* GetThreads() const {
    return &Threads;
  }

private:
  FEXCore::Context::Context* CTX;
  FEX::HLE::SignalDelegator* SignalDelegation;

  FEXCore::ForkableUniqueMutex ThreadCreationMutex;
  fextl::vector<FEXCore::Core::InternalThreadState*> Threads;

  // Thread idling support.
  bool Running {};
  std::mutex IdleWaitMutex;
  std::condition_variable IdleWaitCV;
  std::atomic<uint32_t> IdleWaitRefCount {};

  void HandleThreadDeletion(FEXCore::Core::InternalThreadState* Thread);
  void NotifyPause();
};

} // namespace FEX::HLE
