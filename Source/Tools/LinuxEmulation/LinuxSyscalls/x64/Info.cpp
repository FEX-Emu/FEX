// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x64/Syscalls.h"
#include <FEXCore/Core/Context.h>

#include <cstring>
#include <sys/utsname.h>
#include <sys/resource.h>
#include <sys/sysinfo.h>

namespace FEX::HLE::x64 {
void RegisterInfo(FEX::HLE::SyscallHandler* Handler) {
  using namespace FEXCore::IR;

  if (Handler->IsHostKernelVersionAtLeast(6, 6, 0)) {
    REGISTER_SYSCALL_IMPL_X64(map_shadow_stack, [](FEXCore::Core::CpuStateFrame* Frame, uint64_t addr, uint64_t size, uint32_t flags) -> uint64_t {
      // Claim that shadow stack isn't supported.
      return -EOPNOTSUPP;
    });
  } else {
    REGISTER_SYSCALL_IMPL_X64(map_shadow_stack, UnimplementedSyscallSafe);
  }
}
} // namespace FEX::HLE::x64
