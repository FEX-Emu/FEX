/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"

#include <mqueue.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace FEX::HLE {
  void RegisterMsg() {
    REGISTER_SYSCALL_IMPL_PASS(msgget, [](FEXCore::Core::CpuStateFrame *Frame, key_t key, int msgflg) -> uint64_t {
      uint64_t Result = ::msgget(key, msgflg);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(msgsnd, [](FEXCore::Core::CpuStateFrame *Frame, int msqid, const void *msgp, size_t msgsz, int msgflg) -> uint64_t {
      uint64_t Result = ::msgsnd(msqid, msgp, msgsz, msgflg);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(msgrcv, [](FEXCore::Core::CpuStateFrame *Frame, int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg) -> uint64_t {
      uint64_t Result = ::msgrcv(msqid, msgp, msgsz, msgtyp, msgflg);
      SYSCALL_ERRNO();
    });

    // XXX: msqid_ds is definitely not correct for 32-bit
    REGISTER_SYSCALL_IMPL_PASS(msgctl, [](FEXCore::Core::CpuStateFrame *Frame, int msqid, int cmd, struct msqid_ds *buf) -> uint64_t {
      uint64_t Result = ::msgctl(msqid, cmd, buf);
      SYSCALL_ERRNO();
    });

    // last two parameters are optional
    REGISTER_SYSCALL_IMPL_PASS(mq_unlink, [](FEXCore::Core::CpuStateFrame *Frame, const char *name) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(mq_unlink), name);
      SYSCALL_ERRNO();
    });
  }
}
