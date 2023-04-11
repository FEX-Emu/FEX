/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/Types.h"
#include "LinuxSyscalls/x64/Syscalls.h"
#include "LinuxSyscalls/x32/Syscalls.h"

#include <FEXCore/IR/IR.h>

#include <stddef.h>
#include <stdint.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace FEX::HLE {
  void RegisterTimer(FEX::HLE::SyscallHandler *Handler) {
    using namespace FEXCore::IR;

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(alarm, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, unsigned int seconds) -> uint64_t {
      uint64_t Result = ::alarm(seconds);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(timer_getoverrun, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, kernel_timer_t timerid) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(timer_getoverrun), timerid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(timer_delete, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, kernel_timer_t timerid) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(timer_delete), timerid);
      SYSCALL_ERRNO();
    });
  }
}
