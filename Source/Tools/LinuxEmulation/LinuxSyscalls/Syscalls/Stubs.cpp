// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include <FEXCore/Utils/LogManager.h>

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x32/Syscalls.h"
#include "LinuxSyscalls/x64/Syscalls.h"

#include <errno.h>
#include <stdint.h>
#include <sys/types.h>

namespace FEXCore::Core {
struct CpuStateFrame;
}

namespace FEX::HLE {
auto stub_fault(FEXCore::Core::CpuStateFrame* Frame) -> uint64_t {
  ERROR_AND_DIE_FMT("Syscall: stub!");
  return -ENOSYS;
}
auto stub(FEXCore::Core::CpuStateFrame* Frame) -> uint64_t {
  return -ENOSYS;
}

void RegisterStubs(FEX::HLE::SyscallHandler* Handler) {
  REGISTER_SYSCALL_IMPL(restart_syscall, stub_fault);
  REGISTER_SYSCALL_IMPL(rseq, stub);
}
} // namespace FEX::HLE
