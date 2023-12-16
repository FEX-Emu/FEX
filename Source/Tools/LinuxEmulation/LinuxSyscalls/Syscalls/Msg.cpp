// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x64/Syscalls.h"
#include "LinuxSyscalls/x32/Syscalls.h"

#include <FEXCore/IR/IR.h>

#include <sys/types.h>
#include <sys/msg.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace FEX::HLE {
  void RegisterMsg(FEX::HLE::SyscallHandler *Handler) {
    using namespace FEXCore::IR;

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(msgget, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, key_t key, int msgflg) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(msgget), key, msgflg);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(msgsnd, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int msqid, const void *msgp, size_t msgsz, int msgflg) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(msgsnd), msqid, msgp, msgsz, msgflg);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(msgrcv, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(msgrcv), msqid, msgp, msgsz, msgtyp, msgflg);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(msgctl, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int msqid, int cmd, struct msqid_ds *buf) -> uint64_t {
      // A quirk of this syscall
      // On 32-bit this syscall ONLY supports IPC_64 msqid_ds encoding
      // If an application want to use the old style encoding then it needs to use the ipc syscall with MSGCTL command
      // ipc syscall supports both IPC_64 and old encoding
      uint64_t Result = ::syscall(SYSCALL_DEF(msgctl), msqid, cmd, buf);
      SYSCALL_ERRNO();
    });

    // last two parameters are optional
    REGISTER_SYSCALL_IMPL_PASS_FLAGS(mq_unlink, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, const char *name) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(mq_unlink), name);
      SYSCALL_ERRNO();
    });
  }
}
