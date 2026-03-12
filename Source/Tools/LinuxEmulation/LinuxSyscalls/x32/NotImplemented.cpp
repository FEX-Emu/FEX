// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#include "LinuxSyscalls/x32/Syscalls.h"
#include <FEXCore/Utils/LogManager.h>

#include <errno.h>
#include <stdint.h>

namespace FEXCore::Core {
struct CpuStateFrame;
}

namespace FEX::HLE::x32 {
#define REGISTER_SYSCALL_NOT_IMPL_X32(name)                                             \
  REGISTER_SYSCALL_IMPL_X32(name, [](FEXCore::Core::CpuStateFrame* Frame) -> uint64_t { \
    LogMan::Msg::DFmt("Using deprecated/removed syscall: " #name);                      \
    return -ENOSYS;                                                                     \
  });
#define REGISTER_SYSCALL_NO_PERM_X32(name) \
  REGISTER_SYSCALL_IMPL_X32(name, [](FEXCore::Core::CpuStateFrame* Frame) -> uint64_t { return -EPERM; });

#define SYSCALL_NOT_IMPL_X32(name)                                 \
  auto name(FEXCore::Core::CpuStateFrame* Frame) -> uint64_t {     \
    LogMan::Msg::DFmt("Using deprecated/removed syscall: " #name); \
    return -ENOSYS;                                                \
  };
#define SYSCALL_NO_PERM_X32(name)                              \
  auto name(FEXCore::Core::CpuStateFrame* Frame) -> uint64_t { \
    return -EPERM;                                             \
  };

SYSCALL_NOT_IMPL_X32(_break);
SYSCALL_NOT_IMPL_X32(stty);
SYSCALL_NOT_IMPL_X32(gtty);
SYSCALL_NOT_IMPL_X32(prof);
SYSCALL_NOT_IMPL_X32(ftime);
SYSCALL_NOT_IMPL_X32(mpx);
SYSCALL_NOT_IMPL_X32(lock);
SYSCALL_NOT_IMPL_X32(ulimit);
SYSCALL_NOT_IMPL_X32(profil);
SYSCALL_NOT_IMPL_X32(idle);

SYSCALL_NO_PERM_X32(stime);
SYSCALL_NO_PERM_X32(bdflush);
// these are removed/not implemented in the linux kernel we present
void RegisterNotImplemented(FEX::HLE::SyscallHandler* Handler) {
  REGISTER_SYSCALL_IMPL_X32(break, _break);
  REGISTER_SYSCALL_IMPL_X32(stty, stty);
  REGISTER_SYSCALL_IMPL_X32(gtty, gtty);
  REGISTER_SYSCALL_IMPL_X32(prof, prof);
  REGISTER_SYSCALL_IMPL_X32(ftime, ftime);
  REGISTER_SYSCALL_IMPL_X32(mpx, mpx);
  REGISTER_SYSCALL_IMPL_X32(lock, lock);
  REGISTER_SYSCALL_IMPL_X32(ulimit, ulimit);
  REGISTER_SYSCALL_IMPL_X32(profil, profil);
  REGISTER_SYSCALL_IMPL_X32(idle, idle);

  REGISTER_SYSCALL_IMPL_X32(stime, stime);
  REGISTER_SYSCALL_IMPL_X32(bdflush, bdflush);
}
} // namespace FEX::HLE::x32
