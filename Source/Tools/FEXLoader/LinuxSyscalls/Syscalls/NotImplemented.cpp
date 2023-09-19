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

#define REGISTER_SYSCALL_NOT_IMPL(name) REGISTER_SYSCALL_IMPL(name, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t { \
  LogMan::Msg::DFmt("Using deprecated/removed syscall: " #name); \
  return -ENOSYS; \
});

#define REGISTER_SYSCALL_NO_PERM(name) REGISTER_SYSCALL_IMPL(name, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t { \
  return -EPERM; \
});

#define REGISTER_SYSCALL_NO_ACCESS(name) REGISTER_SYSCALL_IMPL(name, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t { \
  return -EACCES; \
});

namespace FEX::HLE {
  // these are removed/not implemented in the linux kernel we present

  void RegisterNotImplemented(FEX::HLE::SyscallHandler *Handler) {
      REGISTER_SYSCALL_NOT_IMPL(uselib);
      REGISTER_SYSCALL_NOT_IMPL(create_module);
      REGISTER_SYSCALL_NOT_IMPL(get_kernel_syms);
      REGISTER_SYSCALL_NOT_IMPL(query_module);
      REGISTER_SYSCALL_NOT_IMPL(nfsservctl); // Was removed in Linux 3.1
      REGISTER_SYSCALL_NOT_IMPL(getpmsg);
      REGISTER_SYSCALL_NOT_IMPL(putpmsg);
      REGISTER_SYSCALL_NOT_IMPL(afs_syscall);
      REGISTER_SYSCALL_NOT_IMPL(vserver);
      REGISTER_SYSCALL_NOT_IMPL(_sysctl); // Was removed in Linux 5.5

      REGISTER_SYSCALL_NO_PERM(vhangup);
      REGISTER_SYSCALL_NO_PERM(reboot)
      REGISTER_SYSCALL_NO_PERM(sethostname);
      REGISTER_SYSCALL_NO_PERM(setdomainname);
      REGISTER_SYSCALL_NO_PERM(kexec_load);
      REGISTER_SYSCALL_NO_PERM(finit_module);
      REGISTER_SYSCALL_NO_PERM(bpf);
      REGISTER_SYSCALL_NO_PERM(lookup_dcookie);
      REGISTER_SYSCALL_NO_PERM(init_module)
      REGISTER_SYSCALL_NO_PERM(delete_module);
      REGISTER_SYSCALL_NO_PERM(quotactl);
      REGISTER_SYSCALL_NO_ACCESS(perf_event_open);
  }
}
