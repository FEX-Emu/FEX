/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"

#include <FEXCore/IR/IR.h>
#include <FEXHeaderUtils/Syscalls.h>

#include <sys/types.h>

namespace FEX::HLE {
  void RegisterSemaphore(FEX::HLE::SyscallHandler *Handler) {
    using namespace FEXCore::IR;

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(semget, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, key_t key, int nsems, int semflg) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(semget), key, nsems, semflg);
      SYSCALL_ERRNO();
    });
  }
}
