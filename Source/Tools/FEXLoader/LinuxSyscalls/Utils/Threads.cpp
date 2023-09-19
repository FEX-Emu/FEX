// SPDX-License-Identifier: MIT
#include "LinuxSyscalls/Utils/Threads.h"

#include <FEXCore/Core/Context.h>
#include <FEXCore/Utils/Threads.h>

#include <FEXCore/fextl/deque.h>

namespace FEX::LinuxEmulation::Threads {
  // Stack pool handling
  struct StackPoolItem {
    void *Ptr;
    size_t Size;
  };
  std::mutex DeadStackPoolMutex{};
  std::mutex LiveStackPoolMutex{};

  static fextl::deque<StackPoolItem> DeadStackPool{};
  static fextl::deque<StackPoolItem> LiveStackPool{};

  void *AllocateStackObject() {
    std::lock_guard lk{DeadStackPoolMutex};
    if (DeadStackPool.size() == 0) {
      // Nothing in the pool, just allocate
      return FEXCore::Allocator::mmap(nullptr, FEX::LinuxEmulation::Threads::STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
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

  void AddStackToDeadPool(void *Ptr) {
    std::lock_guard lk{DeadStackPoolMutex};
    DeadStackPool.emplace_back(StackPoolItem{Ptr, FEX::LinuxEmulation::Threads::STACK_SIZE});
  }

  void AddStackToLivePool(void *Ptr) {
    std::lock_guard lk{LiveStackPoolMutex};
    LiveStackPool.emplace_back(StackPoolItem{Ptr, FEX::LinuxEmulation::Threads::STACK_SIZE});
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

  void DeallocateStackObject(void *Ptr) {
    RemoveStackFromLivePool(Ptr);
    AddStackToDeadPool(Ptr);
  }

  namespace PThreads {
    void *InitializeThread(void *Ptr);

    class PThread final : public FEXCore::Threads::Thread {
      public:
      PThread(FEXCore::Threads::ThreadFunc Func, void *Arg)
        : UserFunc {Func}
        , UserArg {Arg} {
        pthread_attr_t Attr{};
        Stack = AllocateStackObject();
        // pthreads allocates its dtv region behind our back and there is nothing we can do about it.
        FEXCore::Allocator::YesIKnowImNotSupposedToUseTheGlibcAllocator glibc;
        AddStackToLivePool(Stack);
        pthread_attr_init(&Attr);
        pthread_attr_setstack(&Attr, Stack, FEX::LinuxEmulation::Threads::STACK_SIZE);
        // TODO: Thread creation should be using this instead.
        // Causes Steam to crash early though.
        // pthread_create(&Thread, &Attr, InitializeThread, this);
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
        DeallocateStackObject(Stack);
      }

      private:
      pthread_t Thread;
      FEXCore::Threads::ThreadFunc UserFunc;
      void *UserArg;
      void *Stack{};
    };

    void *InitializeThread(void *Ptr) {
      PThread *Thread{reinterpret_cast<PThread*>(Ptr)};

      // Run the user function
      void *Result = Thread->Execute();

      // Put the stack back in to the stack pool
      Thread->FreeStack();

      // TLS/DTV teardown is something FEX can't control. Disable glibc checking when we leave a pthread.
      FEXCore::Allocator::YesIKnowImNotSupposedToUseTheGlibcAllocator::HardDisable();

      return Result;
    }

    fextl::unique_ptr<FEXCore::Threads::Thread> CreateThread_PThread(
      FEXCore::Threads::ThreadFunc Func,
      void* Arg) {
      return fextl::make_unique<PThread>(Func, Arg);
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
  };

  void SetupThreadHandlers() {
    FEXCore::Threads::Pointers Ptrs = {
      .CreateThread = PThreads::CreateThread_PThread,
      .CleanupAfterFork = PThreads::CleanupAfterFork_PThread,
    };

    FEXCore::Threads::Thread::SetInternalPointers(Ptrs);
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
}
