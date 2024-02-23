// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x64/Syscalls.h"
#include "LinuxSyscalls/x32/Syscalls.h"

#include <FEXCore/Utils/MathUtils.h>

#include <FEXCore/IR/IR.h>

#include <stdint.h>
#include <sched.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace FEX::HLE {
void RegisterSched(FEX::HLE::SyscallHandler* Handler) {
  using namespace FEXCore::IR;

  REGISTER_SYSCALL_IMPL_PASS_FLAGS(sched_yield, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   [](FEXCore::Core::CpuStateFrame* Frame) -> uint64_t {
    uint64_t Result = ::sched_yield();
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_PASS_FLAGS(getpriority, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   [](FEXCore::Core::CpuStateFrame* Frame, int which, int who) -> uint64_t {
    uint64_t Result = ::syscall(SYSCALL_DEF(getpriority), which, who);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_PASS_FLAGS(setpriority, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   [](FEXCore::Core::CpuStateFrame* Frame, int which, int who, int prio) -> uint64_t {
    uint64_t Result = ::setpriority(which, who, prio);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_PASS_FLAGS(sched_setparam, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   [](FEXCore::Core::CpuStateFrame* Frame, pid_t pid, const struct sched_param* param) -> uint64_t {
    uint64_t Result = ::sched_setparam(pid, param);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_PASS_FLAGS(sched_getparam, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   [](FEXCore::Core::CpuStateFrame* Frame, pid_t pid, struct sched_param* param) -> uint64_t {
    uint64_t Result = ::sched_getparam(pid, param);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_PASS_FLAGS(sched_setscheduler, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   [](FEXCore::Core::CpuStateFrame* Frame, pid_t pid, int policy, const struct sched_param* param) -> uint64_t {
    uint64_t Result = ::sched_setscheduler(pid, policy, param);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_PASS_FLAGS(sched_getscheduler, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   [](FEXCore::Core::CpuStateFrame* Frame, pid_t pid) -> uint64_t {
    uint64_t Result = ::sched_getscheduler(pid);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_PASS_FLAGS(sched_get_priority_max, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   [](FEXCore::Core::CpuStateFrame* Frame, int policy) -> uint64_t {
    uint64_t Result = ::sched_get_priority_max(policy);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_PASS_FLAGS(sched_get_priority_min, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   [](FEXCore::Core::CpuStateFrame* Frame, int policy) -> uint64_t {
    uint64_t Result = ::sched_get_priority_min(policy);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_FLAGS(sched_setaffinity, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY | SyscallFlags::NOSIDEEFFECTS,
                              [](FEXCore::Core::CpuStateFrame* Frame, pid_t pid, size_t cpusetsize, const unsigned long* mask) -> uint64_t {
    uint64_t Result = ::syscall(SYSCALL_DEF(sched_setaffinity), pid, cpusetsize, mask);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_FLAGS(sched_getaffinity, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                              [](FEXCore::Core::CpuStateFrame* Frame, pid_t pid, size_t cpusetsize, unsigned char* mask) -> uint64_t {
    uint64_t Result = ::syscall(SYSCALL_DEF(sched_getaffinity), pid, cpusetsize, mask);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_PASS_FLAGS(sched_setattr, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   [](FEXCore::Core::CpuStateFrame* Frame, pid_t pid, struct sched_attr* attr, unsigned int flags) -> uint64_t {
    uint64_t Result = ::syscall(SYSCALL_DEF(sched_setattr), pid, attr, flags);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_PASS_FLAGS(
    sched_getattr, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
    [](FEXCore::Core::CpuStateFrame* Frame, pid_t pid, struct sched_attr* attr, unsigned int size, unsigned int flags) -> uint64_t {
    uint64_t Result = ::syscall(SYSCALL_DEF(sched_getattr), pid, attr, size, flags);
    SYSCALL_ERRNO();
  });
}
} // namespace FEX::HLE
