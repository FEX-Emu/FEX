#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/Threads.h>

#include <cstring>
#include <mutex>
#include <pthread.h>
#include <sys/mman.h>
#include <deque>

namespace FEXCore::Threads {
  // Stack pool handling
  struct StackPoolItem {
    void *Ptr;
    size_t Size;
  };
  std::mutex StackPoolMutex{};
  std::deque<StackPoolItem> StackPool;

  void *AllocateStackObject(size_t Size) {
    std::unique_lock<std::mutex> lk{StackPoolMutex};
    if (StackPool.size() == 0) {
      // Nothing in the pool, just allocate
      return FEXCore::Allocator::mmap(nullptr, Size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_GROWSDOWN, -1, 0);
    }

    // Keep the first item in the stack pool
    auto Result = StackPool.front().Ptr;
    StackPool.pop_front();

    // Erase the rest as a garbage collection step
    for (auto &Item : StackPool) {
      FEXCore::Allocator::munmap(Item.Ptr, Item.Size);
    }
    return Result;
  }

  void AddStackToPool(void *Ptr, size_t Size) {
    std::unique_lock<std::mutex> lk{StackPoolMutex};
    StackPool.emplace_back(StackPoolItem{Ptr, Size});
  }

  void *InitializeThread(void *Ptr);

  class PThread final : public Thread {
    public:
    PThread(FEXCore::Threads::ThreadFunc Func, void *Arg)
      : UserFunc {Func}
      , UserArg {Arg} {
      pthread_attr_t Attr{};
      Stack = AllocateStackObject(STACK_SIZE);
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
      AddStackToPool(Stack, STACK_SIZE);
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

  static FEXCore::Threads::Pointers Ptrs = {
    .CreateThread = CreateThread_PThread,
  };

  std::unique_ptr<FEXCore::Threads::Thread> FEXCore::Threads::Thread::Create(
    ThreadFunc Func,
    void* Arg) {
    return Ptrs.CreateThread(Func, Arg);
  }

  void FEXCore::Threads::Thread::SetInternalPointers(Pointers const &_Ptrs) {
    memcpy(&Ptrs, &_Ptrs, sizeof(FEXCore::Threads::Pointers));
  }
}
