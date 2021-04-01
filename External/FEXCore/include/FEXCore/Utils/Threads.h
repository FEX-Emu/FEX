#pragma once
#include <functional>
#include <memory>

namespace FEXCore::Threads {
  using ThreadFunc = void*(*)(void* user_ptr);

  class Thread;
  using CreateThreadFunc = std::function<std::unique_ptr<Thread>(ThreadFunc Func, void* Arg)>;
  struct Pointers {
    CreateThreadFunc CreateThread;
  };

  // API
  class Thread {
    public:
    virtual ~Thread() = default;
    virtual bool joinable() = 0;
    virtual bool join(void **ret) = 0;
    virtual bool detach() = 0;
    virtual bool IsSelf() = 0;
    static std::unique_ptr<Thread> Create(
      ThreadFunc Func,
      void* Arg);

    static void SetInternalPointers(Pointers const &_Ptrs);
  };
}
