#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"
#include "LogManager.h"

#include <stdint.h>
#include <sys/epoll.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

#define REGISTER_SYSCALL_NOT_IMPL(name) REGISTER_SYSCALL_IMPL(name, [](FEXCore::Core::InternalThreadState *Thread) -> uint64_t { \
  LogMan::Msg::D("Using deprecated/removed syscall: " #name); \
  return -ENOSYS; \
});

namespace FEXCore::HLE {

  // these are removed/not implemented in the linux kernel we present

  void RegisterNotImplemented() {
      REGISTER_SYSCALL_NOT_IMPL(uselib);
      REGISTER_SYSCALL_NOT_IMPL(create_module);
      REGISTER_SYSCALL_NOT_IMPL(get_kernel_syms);
      REGISTER_SYSCALL_NOT_IMPL(query_module);
      REGISTER_SYSCALL_NOT_IMPL(nfsservctl);
      REGISTER_SYSCALL_NOT_IMPL(getpmsg);
      REGISTER_SYSCALL_NOT_IMPL(putpmsg);
      REGISTER_SYSCALL_NOT_IMPL(afs_syscall);
      REGISTER_SYSCALL_NOT_IMPL(tuxcall);
      REGISTER_SYSCALL_NOT_IMPL(security);
      REGISTER_SYSCALL_NOT_IMPL(set_thread_area);
      REGISTER_SYSCALL_NOT_IMPL(get_thread_area);
      REGISTER_SYSCALL_NOT_IMPL(epoll_ctl_old);
      REGISTER_SYSCALL_NOT_IMPL(epoll_wait_old);
      REGISTER_SYSCALL_NOT_IMPL(vserver);
      REGISTER_SYSCALL_NOT_IMPL(_sysctl);
  }
}