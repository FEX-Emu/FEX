#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {

  void RegisterIO() {
    REGISTER_SYSCALL_IMPL(ioperm, [](FEXCore::Core::InternalThreadState *Thread, unsigned long from, unsigned long num, int turn_on) -> uint64_t {
        // ioperm not available on our architecture
        return -EPERM;
    });
  }
}
