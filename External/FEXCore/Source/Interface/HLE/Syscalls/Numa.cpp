#include "Interface/HLE/Syscalls.h"

#include <numaif.h>
#include <stdint.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {
  uint64_t Get_mempolicy(FEXCore::Core::InternalThreadState *Thread, int *mode, unsigned long *nodemask, unsigned long maxnode, void *addr, unsigned long flags) {
    uint64_t Result = ::get_mempolicy(mode, nodemask, maxnode, addr, flags);
    SYSCALL_ERRNO();
  }
}
