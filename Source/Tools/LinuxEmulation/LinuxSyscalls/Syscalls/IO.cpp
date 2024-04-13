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
void RegisterIO(FEX::HLE::SyscallHandler* Handler) {
  using namespace FEXCore::IR;

  REGISTER_SYSCALL_IMPL_FLAGS(iopl, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                              [](FEXCore::Core::CpuStateFrame* Frame, int level) -> uint64_t {
                                // Just claim we don't have permission
                                return -EPERM;
                              });

  REGISTER_SYSCALL_IMPL_FLAGS(ioperm, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                              [](FEXCore::Core::CpuStateFrame* Frame, unsigned long from, unsigned long num, int turn_on) -> uint64_t {
                                // ioperm not available on our architecture
                                return -EPERM;
                              });
}
} // namespace FEX::HLE
