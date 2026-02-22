// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#include <FEXCore/Utils/LogManager.h>
#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x64/Syscalls.h"

#include <errno.h>
#include <stdint.h>

namespace FEXCore::Core {
struct CpuStateFrame;
}

namespace FEX::HLE::x64 {
#define SYSCALL_NOT_IMPL_X64(name)                                 \
  auto name(FEXCore::Core::CpuStateFrame* Frame) -> uint64_t {     \
    LogMan::Msg::DFmt("Using deprecated/removed syscall: " #name); \
    return -ENOSYS;                                                \
  };

#define SYSCALL_NOT_IMPL_SAFE_X64(name)                        \
  auto name(FEXCore::Core::CpuStateFrame* Frame) -> uint64_t { \
    return -ENOSYS;                                            \
  }

#define SYSCALL_NO_PERM_X64(name)                              \
  auto name(FEXCore::Core::CpuStateFrame* Frame) -> uint64_t { \
    return -EPERM;                                             \
  }

SYSCALL_NOT_IMPL_X64(tuxcall);
SYSCALL_NOT_IMPL_X64(security);
SYSCALL_NOT_IMPL_X64(set_thread_area);
SYSCALL_NOT_IMPL_X64(get_thread_area);
SYSCALL_NOT_IMPL_X64(epoll_ctl_old);
SYSCALL_NOT_IMPL_X64(epoll_wait_old);
SYSCALL_NO_PERM_X64(kexec_file_load);
SYSCALL_NOT_IMPL_SAFE_X64(uretprobe);

// these are removed/not implemented in the linux kernel we present
void RegisterNotImplemented(FEX::HLE::SyscallHandler* Handler) {
  REGISTER_SYSCALL_IMPL_X64(tuxcall, tuxcall);
  REGISTER_SYSCALL_IMPL_X64(security, security);
  REGISTER_SYSCALL_IMPL_X64(set_thread_area, set_thread_area);
  REGISTER_SYSCALL_IMPL_X64(get_thread_area, get_thread_area);
  REGISTER_SYSCALL_IMPL_X64(epoll_ctl_old, epoll_ctl_old);
  REGISTER_SYSCALL_IMPL_X64(epoll_wait_old, epoll_wait_old);
  REGISTER_SYSCALL_IMPL_X64(kexec_file_load, kexec_file_load);
  REGISTER_SYSCALL_IMPL_X64(uretprobe, uretprobe);
}
} // namespace FEX::HLE::x64
