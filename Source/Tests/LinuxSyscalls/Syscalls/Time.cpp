/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"

#include <unistd.h>

namespace FEX::HLE {
  void RegisterTime() {
    REGISTER_SYSCALL_IMPL_PASS(pause, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      uint64_t Result = ::pause();
      SYSCALL_ERRNO();
    });
  }
}
