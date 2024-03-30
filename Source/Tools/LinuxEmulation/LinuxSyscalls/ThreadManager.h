// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|common
desc: Glue logic, STRACE magic
$end_info$
*/
#pragma once

#include <FEXCore/Core/Context.h>
#include <FEXCore/fextl/vector.h>
#include <FEXCore/Utils/SignalScopeGuards.h>

#include <cstdint>
#include <mutex>

namespace FEXCore {
  namespace Context {
    class Context;
  }
  namespace Core {
    struct CpuStateFrame;
    struct InternalThreadState;
  }
}


namespace FEX::HLE {
class SyscallHandler;
class SignalDelegator;


class LinuxThreadData final{
public:
  uint64_t GetUID()  const { return UID; }
  uint64_t GetGID()  const { return GID; }
  uint64_t GetEUID() const { return EUID; }
  uint64_t GetEGID() const { return EGID; }
  uint64_t GetTID()  const { return TID; }
  uint64_t GetPID()  const { return PID; }

  uint64_t UID{1000};
  uint64_t GID{1000};
  uint64_t EUID{1000};
  uint64_t EGID{1000};
  std::atomic<uint64_t> TID{1};
  uint64_t PID{1};
  int32_t *set_child_tid{0};
  int32_t *clear_child_tid{0};
  uint64_t parent_tid{0};
  uint64_t robust_list_head{0};
};

struct ThreadStateObject : public FEXCore::Allocator::FEXAllocOperators {
  FEXCore::Core::InternalThreadState *Thread;
  LinuxThreadData ThreadManager;
};

class ThreadManager final {
  public:
    ThreadManager(FEXCore::Context::Context *CTX, FEX::HLE::SignalDelegator *SignalDelegation)
      : CTX {CTX}
      , SignalDelegation {SignalDelegation} {}

    ~ThreadManager();

    FEX::HLE::ThreadStateObject *CreateThread(uint64_t InitialRIP, uint64_t StackPointer, FEXCore::Core::CPUState *NewThreadState = nullptr, uint64_t ParentTID = 0);
    void TrackThread(FEX::HLE::ThreadStateObject *Thread) {
      std::lock_guard lk(ThreadCreationMutex);
      Threads.emplace_back(Thread);
    }

    void DestroyThread(FEX::HLE::ThreadStateObject *Thread);
    void StopThread(FEX::HLE::ThreadStateObject *Thread);
    void RunThread(FEX::HLE::ThreadStateObject *Thread);

    void Pause();
    void Run();
    void Step();
    void Stop(bool IgnoreCurrentThread = false);

    void WaitForIdle();
    void WaitForIdleWithTimeout();
    void WaitForThreadsToRun();

    void SleepThread(FEXCore::Context::Context *CTX, FEXCore::Core::CpuStateFrame *Frame);

    void UnlockAfterFork(FEXCore::Core::InternalThreadState *Thread, bool Child);

    void IncrementIdleRefCount() {
      ++IdleWaitRefCount;
    }

    void InvalidateGuestCodeRange(FEXCore::Core::InternalThreadState *CallingThread, uint64_t Start, uint64_t Length) {
      std::lock_guard lk(ThreadCreationMutex);

      // Potential deferred since Thread might not be valid.
      // Thread object isn't valid very early in frontend's initialization.
      // To be more optimal the frontend should provide this code with a valid Thread object earlier.
      auto CodeInvalidationlk = GuardSignalDeferringSectionWithFallback(CTX->GetCodeInvalidationMutex(), CallingThread);

      for (auto &Thread : Threads) {
        CTX->InvalidateGuestCodeRange(Thread->Thread, Start, Length);
      }
    }

    void InvalidateGuestCodeRange(FEXCore::Core::InternalThreadState *CallingThread, uint64_t Start, uint64_t Length, FEXCore::Context::CodeRangeInvalidationFn callback) {
      std::lock_guard lk(ThreadCreationMutex);

      // Potential deferred since Thread might not be valid.
      // Thread object isn't valid very early in frontend's initialization.
      // To be more optimal the frontend should provide this code with a valid Thread object earlier.
      auto CodeInvalidationlk = GuardSignalDeferringSectionWithFallback(CTX->GetCodeInvalidationMutex(), CallingThread);

      for (auto &Thread : Threads) {
        CTX->InvalidateGuestCodeRange(Thread->Thread, Start, Length, callback);
      }
    }

    fextl::vector<FEX::HLE::ThreadStateObject *> const *GetThreads() const {
      return &Threads;
    }

  private:
    FEXCore::Context::Context *CTX;
    FEX::HLE::SignalDelegator *SignalDelegation;

    FEXCore::ForkableUniqueMutex ThreadCreationMutex;
    fextl::vector<FEX::HLE::ThreadStateObject *> Threads;

    // Thread idling support.
    bool Running{};
    std::mutex IdleWaitMutex;
    std::condition_variable IdleWaitCV;
    std::atomic<uint32_t> IdleWaitRefCount{};

    void HandleThreadDeletion(FEX::HLE::ThreadStateObject *Thread);
    void NotifyPause();
};

}
