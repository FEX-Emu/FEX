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
#define REGISTER_SYSCALL_NOT_IMPL_X64(name)                                             \
  REGISTER_SYSCALL_IMPL_X64(name, [](FEXCore::Core::CpuStateFrame* Frame) -> uint64_t { \
    LogMan::Msg::DFmt("Using deprecated/removed syscall: " #name);                      \
    return -ENOSYS;                                                                     \
  });

#define REGISTER_SYSCALL_NOT_IMPL_SAFE_X64(name) \
  REGISTER_SYSCALL_IMPL_X64(name, [](FEXCore::Core::CpuStateFrame* Frame) -> uint64_t { return -ENOSYS; });

#define REGISTER_SYSCALL_NO_PERM_X64(name) \
  REGISTER_SYSCALL_IMPL_X64(name, [](FEXCore::Core::CpuStateFrame* Frame) -> uint64_t { return -EPERM; });

// these are removed/not implemented in the linux kernel we present
void RegisterNotImplemented(FEX::HLE::SyscallHandler* Handler) {
  REGISTER_SYSCALL_NOT_IMPL_X64(tuxcall);
  REGISTER_SYSCALL_NOT_IMPL_X64(security);
  REGISTER_SYSCALL_NOT_IMPL_X64(set_thread_area);
  REGISTER_SYSCALL_NOT_IMPL_X64(get_thread_area);
  REGISTER_SYSCALL_NOT_IMPL_X64(epoll_ctl_old);
  REGISTER_SYSCALL_NOT_IMPL_X64(epoll_wait_old);
  REGISTER_SYSCALL_NO_PERM_X64(kexec_file_load);
  REGISTER_SYSCALL_NOT_IMPL_SAFE_X64(uretprobe);
}
} // namespace FEX::HLE::x64
