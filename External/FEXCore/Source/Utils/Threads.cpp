#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/Threads.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/deque.h>

#include <alloca.h>
#include <cstring>
#include <functional>
#include <mutex>
#include <pthread.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/signal.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace FEXCore::Threads {
  fextl::unique_ptr<FEXCore::Threads::Thread> CreateThread_Default(
    ThreadFunc Func,
    void* Arg) {
    ERROR_AND_DIE_FMT("Frontend didn't setup thread creation!");
  }

  void CleanupAfterFork_Default() {
    ERROR_AND_DIE_FMT("Frontend didn't setup thread creation!");
  }

  static FEXCore::Threads::Pointers Ptrs = {
    .CreateThread = CreateThread_Default,
    .CleanupAfterFork = CleanupAfterFork_Default,
  };

  fextl::unique_ptr<FEXCore::Threads::Thread> FEXCore::Threads::Thread::Create(
    ThreadFunc Func,
    void* Arg) {
    return Ptrs.CreateThread(Func, Arg);
  }

  void FEXCore::Threads::Thread::CleanupAfterFork() {
    return Ptrs.CleanupAfterFork();
  }

  void FEXCore::Threads::Thread::SetInternalPointers(Pointers const &_Ptrs) {
    memcpy(&Ptrs, &_Ptrs, sizeof(FEXCore::Threads::Pointers));
  }

  uint64_t SetSignalMask(uint64_t Mask) {
    ::syscall(SYS_rt_sigprocmask, SIG_SETMASK, &Mask, &Mask, 8);
    return Mask;
  }
}
