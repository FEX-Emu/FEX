/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#include <FEXCore/Utils/LogManager.h>

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"

#include <errno.h>
#include <stdint.h>
#include <sys/types.h>

#define SYSCALL_STUB(name) do { ERROR_AND_DIE_FMT("Syscall: " #name " stub!"); return -ENOSYS; } while(0)

namespace FEXCore::Core {
  struct CpuStateFrame;
}

namespace FEX::HLE::x32 {
  void RegisterStubs(FEX::HLE::SyscallHandler *Handler) {
    REGISTER_SYSCALL_IMPL_X32(readdir, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      SYSCALL_STUB(readdir);
    });

    REGISTER_SYSCALL_IMPL_X32(vm86old, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      return -ENOSYS;
    });

    REGISTER_SYSCALL_IMPL_X32(vm86, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      return -ENOSYS;
    });
  }
}
