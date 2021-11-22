/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/Types.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"

#include <stddef.h>
#include <stdint.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace FEX::HLE {
  void RegisterTimer() {

    REGISTER_SYSCALL_IMPL_PASS(alarm, [](FEXCore::Core::CpuStateFrame *Frame, unsigned int seconds) -> uint64_t {
      uint64_t Result = ::alarm(seconds);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(timer_getoverrun, [](FEXCore::Core::CpuStateFrame *Frame, kernel_timer_t timerid) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(timer_getoverrun), timerid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(timer_delete, [](FEXCore::Core::CpuStateFrame *Frame, kernel_timer_t timerid) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(timer_delete), timerid);
      SYSCALL_ERRNO();
    });
  }
}
