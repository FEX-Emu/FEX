#include <FEXCore/Utils/Threads.h>

#include <cstring>
#include <pthread.h>

namespace FEXCore::Threads {
  class PThread final : public Thread {
    public:
    PThread(FEXCore::Threads::ThreadFunc Func, void *Arg) {
      pthread_attr_t Attr{};
      pthread_attr_init(&Attr);
      pthread_create(&Thread, &Attr, Func, Arg);
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

    private:
    pthread_t Thread;
  };

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
