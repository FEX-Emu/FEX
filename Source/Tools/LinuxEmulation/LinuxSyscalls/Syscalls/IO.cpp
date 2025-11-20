// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x64/Syscalls.h"
#include "LinuxSyscalls/x32/Syscalls.h"

#include <FEXCore/IR/IR.h>

#include <linux/aio_abi.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace FEX::HLE {
auto noperm(FEXCore::Core::CpuStateFrame* Frame, int level) -> uint64_t {
  // Just claim we don't have permission
  return -EPERM;
};


void RegisterIO(FEX::HLE::SyscallHandler* Handler) {
  using namespace FEXCore::IR;

  REGISTER_SYSCALL_IMPL(iopl, noperm);
  REGISTER_SYSCALL_IMPL(ioperm, noperm);
}
} // namespace FEX::HLE
