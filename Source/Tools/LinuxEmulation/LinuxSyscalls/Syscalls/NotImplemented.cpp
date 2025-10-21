// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include <FEXCore/Utils/LogManager.h>
#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x64/Syscalls.h"
#include "LinuxSyscalls/x32/Syscalls.h"

#include <stdint.h>
#include <sys/epoll.h>

#define SYSCALL_NOT_IMPL(name)                                     \
  auto name(FEXCore::Core::CpuStateFrame* Frame) -> uint64_t {     \
    LogMan::Msg::DFmt("Using deprecated/removed syscall: " #name); \
    return -ENOSYS;                                                \
  }

#define SYSCALL_NO_PERM(name)                                  \
  auto name(FEXCore::Core::CpuStateFrame* Frame) -> uint64_t { \
    return -EPERM;                                             \
  }

#define SYSCALL_NO_ACCESS(name)                                \
  auto name(FEXCore::Core::CpuStateFrame* Frame) -> uint64_t { \
    return -EACCES;                                            \
  }

namespace FEX::HLE {
// these are removed/not implemented in the linux kernel we present
SYSCALL_NOT_IMPL(ustat);
SYSCALL_NOT_IMPL(sysfs);
SYSCALL_NOT_IMPL(uselib);
SYSCALL_NOT_IMPL(create_module);
SYSCALL_NOT_IMPL(get_kernel_syms);
SYSCALL_NOT_IMPL(query_module);
SYSCALL_NOT_IMPL(nfsservctl); // Was removed in Linux 3.1
SYSCALL_NOT_IMPL(getpmsg);
SYSCALL_NOT_IMPL(putpmsg);
SYSCALL_NOT_IMPL(afs_syscall);
SYSCALL_NOT_IMPL(vserver);
SYSCALL_NOT_IMPL(_sysctl); // Was removed in Linux 5.5

SYSCALL_NO_PERM(vhangup);
SYSCALL_NO_PERM(reboot)
SYSCALL_NO_PERM(sethostname);
SYSCALL_NO_PERM(setdomainname);
SYSCALL_NO_PERM(kexec_load);
SYSCALL_NO_PERM(finit_module);
SYSCALL_NO_PERM(bpf);
SYSCALL_NO_PERM(lookup_dcookie);
SYSCALL_NO_PERM(init_module)
SYSCALL_NO_PERM(delete_module);
SYSCALL_NO_PERM(quotactl);
SYSCALL_NO_ACCESS(perf_event_open);

void RegisterNotImplemented(FEX::HLE::SyscallHandler* Handler) {
  REGISTER_SYSCALL_IMPL(ustat, ustat);
  REGISTER_SYSCALL_IMPL(sysfs, sysfs);
  REGISTER_SYSCALL_IMPL(uselib, uselib);
  REGISTER_SYSCALL_IMPL(create_module, create_module);
  REGISTER_SYSCALL_IMPL(get_kernel_syms, get_kernel_syms);
  REGISTER_SYSCALL_IMPL(query_module, query_module);
  REGISTER_SYSCALL_IMPL(nfsservctl, nfsservctl); // Was removed in Linux 3.1
  REGISTER_SYSCALL_IMPL(getpmsg, getpmsg);
  REGISTER_SYSCALL_IMPL(putpmsg, putpmsg);
  REGISTER_SYSCALL_IMPL(afs_syscall, afs_syscall);
  REGISTER_SYSCALL_IMPL(vserver, vserver);
  REGISTER_SYSCALL_IMPL(_sysctl, _sysctl); // Was removed in Linux 5.5

  REGISTER_SYSCALL_IMPL(vhangup, vhangup);
  REGISTER_SYSCALL_IMPL(reboot, reboot);
  REGISTER_SYSCALL_IMPL(sethostname, sethostname);
  REGISTER_SYSCALL_IMPL(setdomainname, setdomainname);
  REGISTER_SYSCALL_IMPL(kexec_load, kexec_load);
  REGISTER_SYSCALL_IMPL(finit_module, finit_module);
  REGISTER_SYSCALL_IMPL(bpf, bpf);
  REGISTER_SYSCALL_IMPL(lookup_dcookie, lookup_dcookie);
  REGISTER_SYSCALL_IMPL(init_module, init_module);
  REGISTER_SYSCALL_IMPL(delete_module, delete_module);
  REGISTER_SYSCALL_IMPL(quotactl, quotactl);
  REGISTER_SYSCALL_IMPL(perf_event_open, perf_event_open);
}
} // namespace FEX::HLE
