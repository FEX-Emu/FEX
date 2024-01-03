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
  void RegisterInfo(FEX::HLE::SyscallHandler *Handler) {
    using namespace FEXCore::IR;

    REGISTER_SYSCALL_IMPL_X64_PASS(sysinfo, [](FEXCore::Core::CpuStateFrame *Frame, struct sysinfo *info) -> uint64_t {
      uint64_t Result = ::sysinfo(info);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(getrusage, [](FEXCore::Core::CpuStateFrame *Frame, int who, struct rusage *usage) -> uint64_t {
      uint64_t Result = ::getrusage(who, usage);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(getrlimit, [](FEXCore::Core::CpuStateFrame *Frame, int resource, struct rlimit *rlim) -> uint64_t {
      uint64_t Result = ::getrlimit(resource, rlim);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(setrlimit, [](FEXCore::Core::CpuStateFrame *Frame, int resource, const struct rlimit *rlim) -> uint64_t {
      uint64_t Result = ::setrlimit(resource, rlim);
      SYSCALL_ERRNO();
    });

    if (Handler->IsHostKernelVersionAtLeast(6, 6, 0)) {
      REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(map_shadow_stack, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
        [](FEXCore::Core::CpuStateFrame *Frame, uint64_t addr, uint64_t size, uint32_t flags) -> uint64_t {
        // Claim that shadow stack isn't supported.
        return -EOPNOTSUPP;
      });
    }
    else {
      REGISTER_SYSCALL_IMPL_X64(map_shadow_stack, UnimplementedSyscallSafe);
    }
  }
}
