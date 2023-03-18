#pragma once
#include <FEXCore/fextl/memory.h>

#include <functional>

namespace FEXCore::Threads {
  using ThreadFunc = void*(*)(void* user_ptr);

  class Thread;
  using CreateThreadFunc = std::function<fextl::unique_ptr<Thread>(ThreadFunc Func, void* Arg)>;
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

    static fextl::unique_ptr<Thread> Create(
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

  /**
   * @brief Sets the calling thread's signal mask to the one provided
   *
   * @param Mask The new 64-bit signal mask to set
   *
   * @return The previous signal mask
   */
  uint64_t SetSignalMask(uint64_t Mask);
}
