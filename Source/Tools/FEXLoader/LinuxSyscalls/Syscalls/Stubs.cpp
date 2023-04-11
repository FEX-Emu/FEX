/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include <FEXCore/Utils/LogManager.h>

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"

#include <errno.h>
#include <stdint.h>
#include <sys/types.h>

#define SYSCALL_STUB(name) do { ERROR_AND_DIE_FMT("Syscall: " #name " stub!"); return -ENOSYS; } while(0)

namespace FEXCore::Core {
  struct CpuStateFrame;
}

namespace FEX::HLE {
  void RegisterStubs(FEX::HLE::SyscallHandler *Handler) {

    REGISTER_SYSCALL_IMPL(ptrace, [](FEXCore::Core::CpuStateFrame *Frame, int /*enum __ptrace_request*/ request, pid_t pid, void *addr, void *data) -> uint64_t {
      // We don't support this
      return -EPERM;
    });

    REGISTER_SYSCALL_IMPL(modify_ldt, [](FEXCore::Core::CpuStateFrame *Frame, int func, void *ptr, unsigned long bytecount) -> uint64_t {
      SYSCALL_STUB(modify_ldt);
    });

    REGISTER_SYSCALL_IMPL(restart_syscall, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      SYSCALL_STUB(restart_syscall);
    });

    REGISTER_SYSCALL_IMPL(rseq, [](FEXCore::Core::CpuStateFrame *Frame,  struct rseq  *rseq, uint32_t rseq_len, int flags, uint32_t sig) -> uint64_t {
      // We don't support this
      return -ENOSYS;
    });
  }
}
