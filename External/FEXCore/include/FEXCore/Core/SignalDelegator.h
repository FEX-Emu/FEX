#pragma once
#include <functional>

namespace FEXCore {
namespace Core {
  struct InternalThreadState;
}

  using HostSignalDelegatorFunction = std::function<bool(FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext)>;
}
