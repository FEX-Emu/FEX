#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x32/Syscalls.h"

#include "Interface/Context/Context.h"
#include "Interface/Core/InternalThreadState.h"
#include "Interface/HLE/Syscalls.h"

#include <sys/mount.h>

namespace FEXCore::HLE::x32 {
  void RegisterFS() {
    REGISTER_SYSCALL_IMPL_X32(umount, [](FEXCore::Core::InternalThreadState *Thread, const char *target) -> uint32_t {
      uint64_t Result = ::umount(target);
      SYSCALL_ERRNO();
    });
  }
}
