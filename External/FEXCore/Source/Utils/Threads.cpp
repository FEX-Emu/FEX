#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/Threads.h>
#include <FEXCore/fextl/deque.h>

#include <alloca.h>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <pthread.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/signal.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace FEXCore::Threads {
  // Stack pool handling
  struct StackPoolItem {
    void *Ptr;
    size_t Size;
  };
  std::mutex DeadStackPoolMutex{};
  std::mutex LiveStackPoolMutex{};

  static fextl::deque<StackPoolItem> DeadStackPool{};
  static fextl::deque<StackPoolItem> LiveStackPool{};

  void *AllocateStackObject(size_t Size) {
    std::lock_guard lk{DeadStackPoolMutex};
    if (DeadStackPool.size() == 0) {
      // Nothing in the pool, just allocate
      return FEXCore::Allocator::mmap(nullptr, Size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    }

    // Keep the first item in the stack pool
    auto Result = DeadStackPool.front().Ptr;
    DeadStackPool.pop_front();

    // Erase the rest as a garbage collection step
    for (auto &Item : DeadStackPool) {
      FEXCore::Allocator::munmap(Item.Ptr, Item.Size);
    }
    return Result;
  }

  void AddStackToDeadPool(void *Ptr, size_t Size) {
    std::lock_guard lk{DeadStackPoolMutex};
    DeadStackPool.emplace_back(StackPoolItem{Ptr, Size});
  }

  void AddStackToLivePool(void *Ptr, size_t Size) {
    std::lock_guard lk{LiveStackPoolMutex};
    LiveStackPool.emplace_back(StackPoolItem{Ptr, Size});
  }

  void RemoveStackFromLivePool(void *Ptr) {
    std::lock_guard lk{LiveStackPoolMutex};
    for (auto it = LiveStackPool.begin(); it != LiveStackPool.end(); ++it) {
      if (it->Ptr == Ptr) {
        LiveStackPool.erase(it);
        return;
      }
    }
  }

  void DeallocateStackObject(void *Ptr, size_t Size) {
    RemoveStackFromLivePool(Ptr);
    AddStackToDeadPool(Ptr, Size);
  }

  void Shutdown() {
    std::lock_guard lk{DeadStackPoolMutex};
    std::lock_guard lk2{LiveStackPoolMutex};
    // Erase all the dead stack pools
    for (auto &Item : DeadStackPool) {
      FEXCore::Allocator::munmap(Item.Ptr, Item.Size);
    }

    // Now clean up any that are considered to still be live
    // We are in shutdown phase, everything in the process is dead
    for (auto &Item : LiveStackPool) {
      FEXCore::Allocator::munmap(Item.Ptr, Item.Size);
    }

    DeadStackPool.clear();
    LiveStackPool.clear();
  }

  void *InitializeThread(void *Ptr);

  class PThread final : public Thread {
    public:
    PThread(FEXCore::Threads::ThreadFunc Func, void *Arg)
      : UserFunc {Func}
      , UserArg {Arg} {
      pthread_attr_t Attr{};
      Stack = AllocateStackObject(STACK_SIZE);
      AddStackToLivePool(Stack, STACK_SIZE);
      pthread_attr_init(&Attr);
      pthread_attr_setstack(&Attr, Stack, STACK_SIZE);
      pthread_create(&Thread, &Attr, Func, Arg);

      pthread_attr_destroy(&Attr);
    }

    bool joinable() override {
      pthread_attr_t Attr{};
      if (pthread_getattr_np(Thread, &Attr) == 0) {
        int AttachState{};
        if (pthread_attr_getdetachstate(&Attr, &AttachState) == 0) {
          if (AttachState == PTHREAD_CREATE_JOINABLE) {
            return true;
          }
        }
      }
      return false;
    }

    bool join(void **ret) override {
      return pthread_join(Thread, ret) == 0;
    }

    bool detach() override {
      return pthread_detach(Thread) == 0;
    }

    bool IsSelf() override {
      auto self = pthread_self();
      return self == Thread;
    }

    void *Execute() {
      return UserFunc(UserArg);
    }

    void FreeStack() {
      DeallocateStackObject(Stack, STACK_SIZE);
    }

    private:
    pthread_t Thread;
    FEXCore::Threads::ThreadFunc UserFunc;
    void *UserArg;
    void *Stack{};
    constexpr static size_t STACK_SIZE = 8 * 1024 * 1024;
  };

  void *InitializeThread(void *Ptr) {
    PThread *Thread{reinterpret_cast<PThread*>(Ptr)};

    // Run the user function
    void *Result = Thread->Execute();

    // Put the stack back in to the stack pool
    Thread->FreeStack();
    return Result;
  }

  std::unique_ptr<FEXCore::Threads::Thread> CreateThread_PThread(
    ThreadFunc Func,
    void* Arg) {
    return std::make_unique<PThread>(Func, Arg);
  }

  void CleanupAfterFork_PThread() {
    // We don't need to pull the mutex here
    // After a fork we are the only thread running
    // Just need to make sure not to delete our own stack
    uintptr_t StackLocation = reinterpret_cast<uintptr_t>(alloca(0));

    auto ClearStackPool = [&](auto &StackPool) {
      for (auto it = StackPool.begin(); it != StackPool.end(); ) {
        StackPoolItem &Item = *it;
        uintptr_t ItemStack = reinterpret_cast<uintptr_t>(Item.Ptr);
        if (ItemStack <= StackLocation && (ItemStack + Item.Size) > StackLocation) {
          // This is our stack item, skip it
          ++it;
        }
        else {
          // Untracked stack. Clean it up
          FEXCore::Allocator::munmap(Item.Ptr, Item.Size);
          it = StackPool.erase(it);
        }
      }
    };

    // Clear both dead stacks and live stacks
    ClearStackPool(DeadStackPool);
    ClearStackPool(LiveStackPool);

    LogMan::Throw::AFmt((DeadStackPool.size() + LiveStackPool.size()) <= 1,
                        "After fork we should only have zero or one tracked stacks!");
  }

  static FEXCore::Threads::Pointers Ptrs = {
    .CreateThread = CreateThread_PThread,
    .CleanupAfterFork = CleanupAfterFork_PThread,
  };

  std::unique_ptr<FEXCore::Threads::Thread> FEXCore::Threads::Thread::Create(
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
