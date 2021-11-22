/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"

#include <stdint.h>
#include <sched.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace FEX::HLE {
  void RegisterSched() {

    REGISTER_SYSCALL_IMPL_PASS(sched_yield, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      uint64_t Result = ::sched_yield();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(getpriority, [](FEXCore::Core::CpuStateFrame *Frame, int which, int who) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(getpriority), which, who);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(setpriority, [](FEXCore::Core::CpuStateFrame *Frame, int which, int who, int prio) -> uint64_t {
      uint64_t Result = ::setpriority(which, who, prio);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(sched_setparam, [](FEXCore::Core::CpuStateFrame *Frame, pid_t pid, const struct sched_param *param) -> uint64_t {
      uint64_t Result = ::sched_setparam(pid, param);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(sched_getparam, [](FEXCore::Core::CpuStateFrame *Frame, pid_t pid, struct sched_param *param) -> uint64_t {
      uint64_t Result = ::sched_getparam(pid, param);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(sched_setscheduler, [](FEXCore::Core::CpuStateFrame *Frame, pid_t pid, int policy, const struct sched_param *param) -> uint64_t {
      uint64_t Result = ::sched_setscheduler(pid, policy, param);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(sched_getscheduler, [](FEXCore::Core::CpuStateFrame *Frame, pid_t pid) -> uint64_t {
      uint64_t Result = ::sched_getscheduler(pid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(sched_get_priority_max, [](FEXCore::Core::CpuStateFrame *Frame, int policy) -> uint64_t {
      uint64_t Result = ::sched_get_priority_max(policy);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(sched_get_priority_min, [](FEXCore::Core::CpuStateFrame *Frame, int policy) -> uint64_t {
      uint64_t Result = ::sched_get_priority_min(policy);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(sched_setaffinity, [](FEXCore::Core::CpuStateFrame *Frame, pid_t pid, size_t cpusetsize, const unsigned long *mask) -> uint64_t {
      return 0;
    });

    REGISTER_SYSCALL_IMPL(sched_getaffinity, [](FEXCore::Core::CpuStateFrame *Frame, pid_t pid, size_t cpusetsize, unsigned char *mask) -> uint64_t {
      uint64_t Cores = FEX::HLE::_SyscallHandler->ThreadsConfig();
      uint64_t Bytes = ((Cores+7) / 8);
      // If we don't have at least one byte in the resulting structure
      // then we need to return -EINVAL
      if (cpusetsize < Bytes) {
        return -EINVAL;
      }

      memset(mask, 0, Bytes);

      for (uint64_t i = 0; i < Cores; ++i) {
        mask[i / 8] |= (1 << (i % 8));
      }

      // Returns the number of bytes written in to mask
      return Bytes;
    });

    REGISTER_SYSCALL_IMPL_PASS(sched_setattr, [](FEXCore::Core::CpuStateFrame *Frame, pid_t pid, struct sched_attr *attr, unsigned int flags) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(sched_setattr), pid, attr, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(sched_getattr, [](FEXCore::Core::CpuStateFrame *Frame, pid_t pid, struct sched_attr *attr, unsigned int size, unsigned int flags) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(sched_getattr), pid, attr, size, flags);
      SYSCALL_ERRNO();
    });
  }
}
