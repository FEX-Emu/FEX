#pragma once
#include <functional>
#include <memory>

namespace FEXCore::Threads {
  using ThreadFunc = void*(*)(void* user_ptr);

  class Thread;
  using CreateThreadFunc = std::function<std::unique_ptr<Thread>(ThreadFunc Func, void* Arg)>;
  using CleanupAfterForkFunc = std::function<void()>;

  struct Pointers {
    CreateThreadFunc CreateThread;
    CleanupAfterForkFunc CleanupAfterFork;
  };

  // API
  class Thread {
    public:
    virtual ~Thread() = default;
    virtual bool joinable() = 0;
    virtual bool join(void **ret) = 0;
    virtual bool detach() = 0;
    virtual bool IsSelf() = 0;

    /**
     * @name Calls provided API functions
     * @{ */

    static std::unique_ptr<Thread> Create(
      ThreadFunc Func,
      void* Arg);

    static void CleanupAfterFork();

    /**  @} */

    // Set API functions
    static void SetInternalPointers(Pointers const &_Ptrs);
  };

  void *AllocateStackObject(size_t Size);
  void DeallocateStackObject(void *Ptr, size_t Size);
  void Shutdown();
}
