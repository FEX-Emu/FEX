/*
$info$
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#include <FEXCore/Utils/LogManager.h>
#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"

namespace FEX::HLE::x64 {
#define REGISTER_SYSCALL_NOT_IMPL_X64(name) REGISTER_SYSCALL_IMPL_X64(name, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t { \
  LogMan::Msg::D("Using deprecated/removed syscall: " #name); \
  return -ENOSYS; \
});
#define REGISTER_SYSCALL_NO_PERM_X64(name) REGISTER_SYSCALL_IMPL_X64(name, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t { \
  return -EPERM; \
});

  // these are removed/not implemented in the linux kernel we present
  void RegisterNotImplemented() {
    REGISTER_SYSCALL_NOT_IMPL_X64(tuxcall);
    REGISTER_SYSCALL_NOT_IMPL_X64(security);
    REGISTER_SYSCALL_NOT_IMPL_X64(set_thread_area);
    REGISTER_SYSCALL_NOT_IMPL_X64(get_thread_area);
    REGISTER_SYSCALL_NOT_IMPL_X64(epoll_ctl_old);
    REGISTER_SYSCALL_NOT_IMPL_X64(epoll_wait_old);
    REGISTER_SYSCALL_NO_PERM_X64(kexec_file_load);
  }
}
