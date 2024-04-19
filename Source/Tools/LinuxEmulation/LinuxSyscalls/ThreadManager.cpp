// SPDX-License-Identifier: MIT

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/SignalDelegator.h"

#include <FEXHeaderUtils/Syscalls.h>

namespace FEX::HLE {
FEXCore::Core::InternalThreadState*
ThreadManager::CreateThread(uint64_t InitialRIP, uint64_t StackPointer, FEXCore::Core::CPUState* NewThreadState, uint64_t ParentTID) {
  return CTX->CreateThread(InitialRIP, StackPointer, NewThreadState, ParentTID);
}

void ThreadManager::DestroyThread(FEXCore::Core::InternalThreadState* Thread) {
  {
    std::lock_guard lk(ThreadCreationMutex);
    auto It = std::find(Threads.begin(), Threads.end(), Thread);
    LOGMAN_THROW_A_FMT(It != Threads.end(), "Thread wasn't in Threads");
    Threads.erase(It);
  }

  HandleThreadDeletion(Thread);
}

void ThreadManager::StopThread(FEXCore::Core::InternalThreadState* Thread) {
  if (Thread->RunningEvents.Running.exchange(false)) {
    SignalDelegation->SignalThread(Thread, FEXCore::Core::SignalEvent::Stop);
  }
}

void ThreadManager::RunThread(FEXCore::Core::InternalThreadState* Thread) {
  // Tell the thread to start executing
  Thread->StartRunning.NotifyAll();
  ++IdleWaitRefCount;
}

void ThreadManager::RunPrimaryThread(FEXCore::Context::Context* CTX, FEXCore::Core::InternalThreadState* Thread) {
  Thread->ThreadManager.TID = FHU::Syscalls::gettid();
  Thread->ThreadManager.PID = ::getpid();

  if (GdbServer()) {
    Thread->StartRunning.Wait();
  }

  ++IdleWaitRefCount;
  CTX->RunUntilExit(Thread);
}

void ThreadManager::HandleThreadDeletion(FEXCore::Core::InternalThreadState* Thread) {
  if (Thread->ExecutionThread) {
    if (Thread->ExecutionThread->joinable()) {
      Thread->ExecutionThread->join(nullptr);
    }

    if (Thread->ExecutionThread->IsSelf()) {
      Thread->ExecutionThread->detach();
    }
  }

  CTX->DestroyThread(Thread);
  --IdleWaitRefCount;
  IdleWaitCV.notify_all();
}

void ThreadManager::NotifyPause() {
  // Tell all the threads that they should pause
  std::lock_guard lk(ThreadCreationMutex);
  for (auto& Thread : Threads) {
    if (Thread->RunningEvents.Running.load()) {
      SignalDelegation->SignalThread(Thread, FEXCore::Core::SignalEvent::Pause);
    }
  }
}

void ThreadManager::Pause() {
  NotifyPause();
  WaitForIdle();
}

void ThreadManager::Pausing(FEXCore::Core::InternalThreadState* Thread) {
  Thread->RunningEvents.Running.store(false);
  --IdleWaitRefCount;
}

void ThreadManager::Run() {
  // Spin up all the threads
  std::lock_guard lk(ThreadCreationMutex);
  for (auto& Thread : Threads) {
    Thread->SignalReason.store(FEXCore::Core::SignalEvent::Return);
  }

  for (auto& Thread : Threads) {
    Thread->StartRunning.NotifyAll();
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
      CTX->ClearCodeCache(Thread);
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
  FEXCore::Core::InternalThreadState* CurrentThread {};

  // Tell all the threads that they should stop
  {
    std::lock_guard lk(ThreadCreationMutex);
    for (auto& Thread : Threads) {
      if (IgnoreCurrentThread && Thread->ThreadManager.TID == tid) {
        // If we are callign stop from the current thread then we can ignore sending signals to this thread
        // This means that this thread is already gone
      } else if (Thread->ThreadManager.TID == tid) {
        // We need to save the current thread for last to ensure all threads receive their stop signals
        CurrentThread = Thread;
        continue;
      }

      if (Thread->RunningEvents.Running.load()) {
        StopThread(Thread);
      }

      // If the thread is waiting to start but immediately killed then there can be a hang
      // This occurs in the case of gdb attach with immediate kill
      if (Thread->RunningEvents.WaitingToStart.load()) {
        Thread->RunningEvents.EarlyExit = true;
        Thread->StartRunning.NotifyAll();
      }
    }
  }

  // Stop the current thread now if we aren't ignoring it
  if (CurrentThread) {
    StopThread(CurrentThread);
  }
}

void ThreadManager::SleepThread(FEXCore::Context::Context* CTX, FEXCore::Core::CpuStateFrame* Frame) {
  auto Thread = Frame->Thread;

  --IdleWaitRefCount;
  IdleWaitCV.notify_all();

  Thread->RunningEvents.ThreadSleeping = true;

  // Go to sleep
  Thread->StartRunning.Wait();

  Thread->RunningEvents.Running = true;
  ++IdleWaitRefCount;
  Thread->RunningEvents.ThreadSleeping = false;

  IdleWaitCV.notify_all();
}

void ThreadManager::UnlockAfterFork(FEXCore::Core::InternalThreadState* LiveThread, bool Child) {
  if (!Child) {
    return;
  }

  // This function is called after fork
  // We need to cleanup some of the thread data that is dead
  for (auto& DeadThread : Threads) {
    if (DeadThread == LiveThread) {
      continue;
    }

    // Setting running to false ensures that when they are shutdown we won't send signals to kill them
    DeadThread->RunningEvents.Running = false;

    // Despite what google searches may susgest, glibc actually has special code to handle forks
    // with multiple active threads.
    // It cleans up the stacks of dead threads and marks them as terminated.
    // It also cleans up a bunch of internal mutexes.

    // FIXME: TLS is probally still alive. Investigate

    // Deconstructing the Interneal thread state should clean up most of the state.
    // But if anything on the now deleted stack is holding a refrence to the heap, it will be leaked
    CTX->DestroyThread(DeadThread);

    // FIXME: Make sure sure nothing gets leaked via the heap. Ideas:
    //         * Make sure nothing is allocated on the heap without ref in InternalThreadState
    //         * Surround any code that heap allocates with a per-thread mutex.
    //           Before forking, the the forking thread can lock all thread mutexes.
  }

  // Remove all threads but the live thread from Threads
  Threads.clear();
  Threads.push_back(LiveThread);

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
