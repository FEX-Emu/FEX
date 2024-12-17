// SPDX-License-Identifier: MIT

#include "LinuxSyscalls/GdbServer.h"
#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/SignalDelegator.h"

#include <FEXHeaderUtils/Syscalls.h>

namespace FEX::HLE {
FEX::HLE::ThreadStateObject* ThreadManager::CreateThread(uint64_t InitialRIP, uint64_t StackPointer, const FEXCore::Core::CPUState* NewThreadState,
                                                         uint64_t ParentTID, FEX::HLE::ThreadStateObject* InheritThread) {
  auto ThreadStateObject = new FEX::HLE::ThreadStateObject;

  ThreadStateObject->ThreadInfo.parent_tid = ParentTID;
  ThreadStateObject->ThreadInfo.PID = ::getpid();

  if (ParentTID == 0) {
    ThreadStateObject->ThreadInfo.TID = FHU::Syscalls::gettid();
  }

  ThreadStateObject->Thread = CTX->CreateThread(InitialRIP, StackPointer, NewThreadState, ParentTID);
  ThreadStateObject->Thread->FrontendPtr = ThreadStateObject;

  if (InheritThread) {
    FEX::HLE::_SyscallHandler->SeccompEmulator.InheritSeccompFilters(InheritThread, ThreadStateObject);
    ThreadStateObject->persona = InheritThread->persona;
  }

  ++IdleWaitRefCount;
  return ThreadStateObject;
}

void ThreadManager::TrackThread(FEX::HLE::ThreadStateObject* Thread) {
  {
    std::lock_guard lk(ThreadCreationMutex);
    Threads.emplace_back(Thread);
  }

  auto GdbServer = SyscallHandler->GetGdbServer();
  if (GdbServer) {
    GdbServer->OnThreadCreated(Thread);
  }
}

void ThreadManager::DestroyThread(FEX::HLE::ThreadStateObject* Thread, bool NeedsTLSUninstall) {
  {
    std::lock_guard lk(ThreadCreationMutex);
    auto It = std::find(Threads.begin(), Threads.end(), Thread);
    LOGMAN_THROW_A_FMT(It != Threads.end(), "Thread wasn't in Threads");
    Threads.erase(It);
  }

  HandleThreadDeletion(Thread, NeedsTLSUninstall);
}

void ThreadManager::StopThread(FEX::HLE::ThreadStateObject* Thread) {
  SignalDelegation->SignalThread(Thread->Thread, SignalEvent::Stop);
}

void ThreadManager::HandleThreadDeletion(FEX::HLE::ThreadStateObject* Thread, bool NeedsTLSUninstall) {
  if (Thread->ExecutionThread) {
    if (Thread->ExecutionThread->joinable()) {
      Thread->ExecutionThread->join(nullptr);
    }

    if (Thread->ExecutionThread->IsSelf()) {
      Thread->ExecutionThread->detach();
    }
  }

  if (NeedsTLSUninstall) {
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
    // Sanity check. This can only be called from the owning thread.
    {
      const auto pid = ::getpid();
      const auto tid = FHU::Syscalls::gettid();
      LOGMAN_THROW_A_FMT(Thread->ThreadInfo.PID == pid && Thread->ThreadInfo.TID == tid, "Can't delete TLS data from a different thread!");
    }
#endif
    FEXCore::Allocator::UninstallTLSData(Thread->Thread);
  }

  CTX->DestroyThread(Thread->Thread);
  FEX::HLE::_SyscallHandler->SeccompEmulator.FreeSeccompFilters(Thread);

  delete Thread;
  --IdleWaitRefCount;
  IdleWaitCV.notify_all();
}

void ThreadManager::NotifyPause() {
  // Tell all the threads that they should pause
  std::lock_guard lk(ThreadCreationMutex);
  for (auto& Thread : Threads) {
    if (!Thread->ThreadSleeping.HasWaiter()) {
      // Only signal if it isn't already sleeping.
      SignalDelegation->SignalThread(Thread->Thread, SignalEvent::Pause);
    }
  }
}

void ThreadManager::Pause() {
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
  // Sanity check. This can't be called from an emulation thread.
  {
    const auto pid = ::getpid();
    const auto tid = FHU::Syscalls::gettid();
    std::lock_guard lk(ThreadCreationMutex);
    for (auto& Thread : Threads) {
      LOGMAN_THROW_A_FMT(!(Thread->ThreadInfo.PID == pid && Thread->ThreadInfo.TID == tid), "Can't put threads to sleep from inside emulation thread!");
    }
  }
#endif
  NotifyPause();
  WaitForIdle();
}

void ThreadManager::Run() {
  // Spin up all the threads
  std::lock_guard lk(ThreadCreationMutex);
  for (auto& Thread : Threads) {
    Thread->SignalReason.store(SignalEvent::Return);
  }
}

void ThreadManager::WaitForIdleWithTimeout() {
  std::unique_lock<std::mutex> lk(IdleWaitMutex);
  bool WaitResult = IdleWaitCV.wait_for(lk, std::chrono::milliseconds(1500), [this] { return IdleWaitRefCount.load() == 0; });

  if (!WaitResult) {
    // The wait failed, this will occur if we stepped in to a syscall
    // That's okay, we just need to pause the threads manually
    NotifyPause();
  }

  // We have sent every thread a pause signal
  // Now wait again because they /will/ be going to sleep
  WaitForIdle();
}

void ThreadManager::WaitForThreadsToRun() {
  size_t NumThreads {};
  {
    std::lock_guard lk(ThreadCreationMutex);
    NumThreads = Threads.size();
  }

  // Spin while waiting for the threads to start up
  std::unique_lock<std::mutex> lk(IdleWaitMutex);
  IdleWaitCV.wait(lk, [this, NumThreads] { return IdleWaitRefCount.load() >= NumThreads; });

  Running = true;
}

void ThreadManager::Step() {
  LogMan::Msg::AFmt("ThreadManager::Step currently not implemented");
  {
    std::lock_guard lk(ThreadCreationMutex);
    // Walk the threads and tell them to clear their caches
    // Useful when our block size is set to a large number and we need to step a single instruction
    for (auto& Thread : Threads) {
      CTX->ClearCodeCache(Thread->Thread);
    }
  }

  // TODO: Set to single step mode.
  Run();
  WaitForThreadsToRun();
  WaitForIdle();
  // TODO: Set back to full running mode.
}

void ThreadManager::Stop(bool IgnoreCurrentThread) {
  pid_t tid = FHU::Syscalls::gettid();
  FEX::HLE::ThreadStateObject* CurrentThread {};

  // Tell all the threads that they should stop
  {
    std::lock_guard lk(ThreadCreationMutex);
    for (auto& Thread : Threads) {
      if (IgnoreCurrentThread && Thread->ThreadInfo.TID == tid) {
        // If we are callign stop from the current thread then we can ignore sending signals to this thread
        // This means that this thread is already gone
      } else if (Thread->ThreadInfo.TID == tid) {
        // We need to save the current thread for last to ensure all threads receive their stop signals
        CurrentThread = Thread;
        continue;
      }

      StopThread(Thread);
    }
  }

  // Stop the current thread now if we aren't ignoring it
  if (CurrentThread) {
    StopThread(CurrentThread);
  }
}

void ThreadManager::SleepThread(FEXCore::Context::Context* CTX, FEX::HLE::ThreadStateObject* ThreadObject) {
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
    // Sanity check. This can only be called from the owning thread.
    {
      const auto pid = ::getpid();
      const auto tid = FHU::Syscalls::gettid();
      LOGMAN_THROW_A_FMT(ThreadObject->ThreadInfo.PID == pid && ThreadObject->ThreadInfo.TID == tid, "Can't delete TLS data from a different thread!");
    }
#endif

  --IdleWaitRefCount;
  IdleWaitCV.notify_all();

  // Go to sleep.
  ThreadObject->ThreadSleeping.Wait();

  ++IdleWaitRefCount;

  IdleWaitCV.notify_all();
}

void ThreadManager::UnpauseThread(FEX::HLE::ThreadStateObject* Thread) {
  Thread->ThreadSleeping.NotifyOne();
}

void ThreadManager::UnlockAfterFork(FEXCore::Core::InternalThreadState* LiveThread, bool Child) {
  if (!Child) {
    return;
  }

  // This function is called after fork
  // We need to cleanup some of the thread data that is dead
  for (auto& DeadThread : Threads) {
    if (DeadThread->Thread == LiveThread) {
      continue;
    }

    // Despite what google searches may susgest, glibc actually has special code to handle forks
    // with multiple active threads.
    // It cleans up the stacks of dead threads and marks them as terminated.
    // It also cleans up a bunch of internal mutexes.

    // FIXME: TLS is probally still alive. Investigate

    // Deconstructing the Interneal thread state should clean up most of the state.
    // But if anything on the now deleted stack is holding a refrence to the heap, it will be leaked
    CTX->DestroyThread(DeadThread->Thread);
    delete DeadThread;

    // FIXME: Make sure sure nothing gets leaked via the heap. Ideas:
    //         * Make sure nothing is allocated on the heap without ref in InternalThreadState
    //         * Surround any code that heap allocates with a per-thread mutex.
    //           Before forking, the the forking thread can lock all thread mutexes.
  }

  // Remove all threads but the live thread from Threads
  Threads.clear();

  auto ThreadObject = FEX::HLE::ThreadManager::GetStateObjectFromCPUState(LiveThread->CurrentFrame);
  Threads.push_back(ThreadObject);

  // Clean up dead stacks
  FEXCore::Threads::Thread::CleanupAfterFork();

  // We now only have one thread.
  IdleWaitRefCount = 1;
  ThreadCreationMutex.StealAndDropActiveLocks();
}

void ThreadManager::WaitForIdle() {
  std::unique_lock<std::mutex> lk(IdleWaitMutex);
  IdleWaitCV.wait(lk, [this] { return IdleWaitRefCount.load() == 0; });

  Running = false;
}

ThreadManager::~ThreadManager() {
  std::lock_guard lk(ThreadCreationMutex);

  for (auto& Thread : Threads) {
    HandleThreadDeletion(Thread);
  }
  Threads.clear();
}
} // namespace FEX::HLE
