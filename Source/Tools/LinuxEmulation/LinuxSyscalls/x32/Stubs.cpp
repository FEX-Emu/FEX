// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#include <FEXCore/Utils/LogManager.h>

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x32/Syscalls.h"

#include <errno.h>
#include <stdint.h>
#include <sys/types.h>

#define SYSCALL_STUB(name)                         \
  do {                                             \
    ERROR_AND_DIE_FMT("Syscall: " #name " stub!"); \
    return -ENOSYS;                                \
  } while (0)

namespace FEXCore::Core {
struct CpuStateFrame;
}

namespace FEX::HLE::x32 {
auto modify_ldt(FEXCore::Core::CpuStateFrame* Frame) -> uint64_t {
  SYSCALL_STUB(readdir);
}

auto readdir(FEXCore::Core::CpuStateFrame* Frame) -> uint64_t {
  SYSCALL_STUB(readdir);
}

auto vm86old(FEXCore::Core::CpuStateFrame* Frame) -> uint64_t {
  return -ENOSYS;
}

auto vm86(FEXCore::Core::CpuStateFrame* Frame) -> uint64_t {
  return -ENOSYS;
}

void RegisterStubs(FEX::HLE::SyscallHandler* Handler) {
  REGISTER_SYSCALL_IMPL_X32(modify_ldt, modify_ldt);
  REGISTER_SYSCALL_IMPL_X32(readdir, readdir);
  REGISTER_SYSCALL_IMPL_X32(vm86old, vm86old);
  REGISTER_SYSCALL_IMPL_X32(vm86, vm86);
}
} // namespace FEX::HLE::x32
