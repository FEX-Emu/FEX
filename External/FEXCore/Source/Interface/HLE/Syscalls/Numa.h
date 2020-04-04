#pragma once
#include <stdint.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {
  uint64_t Get_mempolicy(FEXCore::Core::InternalThreadState *Thread, int *mode, unsigned long *nodemask, unsigned long maxnode, void *addr, unsigned long flags);
}
