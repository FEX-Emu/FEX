// SPDX-License-Identifier: MIT
/*
$info$
meta: LinuxSyscalls|syscalls-shared ~ Syscall implementations shared between x86 and x86-64
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x64/Syscalls.h"
#include "LinuxSyscalls/x32/Syscalls.h"

#include <FEXCore/IR/IR.h>

#include <stdint.h>
#include <sys/epoll.h>

namespace FEX::HLE {
#define PASSTHROUGH_HANDLERS
#include "GeneratedSyscallHandlers.inl"

  void RegisterGenerated(FEX::HLE::SyscallHandler *Handler) {
    using namespace FEXCore::IR;
#define SYSCALL_COMMON_IMPL
#include "GeneratedSyscallHandlers.inl"
  }

  namespace x64 {
    void RegisterGenerated(FEX::HLE::SyscallHandler *Handler) {
      using namespace FEXCore::IR;
#define SYSCALL_X64_IMPL
#include "GeneratedSyscallHandlers.inl"
    }
  }

  namespace x32 {
    void RegisterGenerated(FEX::HLE::SyscallHandler *Handler) {
      using namespace FEXCore::IR;
#define SYSCALL_X32_IMPL
#include "GeneratedSyscallHandlers.inl"
    }
  }
}
