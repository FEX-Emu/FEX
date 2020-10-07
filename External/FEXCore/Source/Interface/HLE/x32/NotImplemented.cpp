#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x32/Syscalls.h"
#include "LogManager.h"

namespace FEXCore::HLE::x32 {
#define REGISTER_SYSCALL_NOT_IMPL_X32(name) REGISTER_SYSCALL_IMPL_X32(name, [](FEXCore::Core::InternalThreadState *Thread) -> uint64_t { \
  LogMan::Msg::D("Using deprecated/removed syscall: " #name); \
  return -ENOSYS; \
});
#define REGISTER_SYSCALL_NO_PERM_X32(name) REGISTER_SYSCALL_IMPL_X32(name, [](FEXCore::Core::InternalThreadState *Thread) -> uint64_t { \
  return -EPERM; \
});

  // these are removed/not implemented in the linux kernel we present
  void RegisterNotImplemented() {
    REGISTER_SYSCALL_NOT_IMPL_X32(break);
    REGISTER_SYSCALL_NOT_IMPL_X32(stty);
    REGISTER_SYSCALL_NOT_IMPL_X32(gtty);
    REGISTER_SYSCALL_NOT_IMPL_X32(prof);
    REGISTER_SYSCALL_NOT_IMPL_X32(ftime);
    REGISTER_SYSCALL_NOT_IMPL_X32(mpx);
    REGISTER_SYSCALL_NOT_IMPL_X32(lock);
    REGISTER_SYSCALL_NOT_IMPL_X32(ulimit);
    REGISTER_SYSCALL_NOT_IMPL_X32(profil);
    REGISTER_SYSCALL_NOT_IMPL_X32(idle);

    REGISTER_SYSCALL_NO_PERM_X32(stime);
    REGISTER_SYSCALL_NO_PERM_X32(bdflush);
  }
}
