/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"
#include <FEXCore/Utils/LogManager.h>

namespace FEX::HLE::x32 {
#define REGISTER_SYSCALL_NOT_IMPL_X32(name) REGISTER_SYSCALL_IMPL_X32(name, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t { \
  LogMan::Msg::D("Using deprecated/removed syscall: " #name); \
  return -ENOSYS; \
});
#define REGISTER_SYSCALL_NO_PERM_X32(name) REGISTER_SYSCALL_IMPL_X32(name, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t { \
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
