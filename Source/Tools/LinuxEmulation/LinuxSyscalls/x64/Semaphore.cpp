// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x64/Syscalls.h"
#include "LinuxSyscalls/x64/Types.h"

#include <FEXHeaderUtils/Syscalls.h>

#include <linux/sem.h>
#include <stddef.h>
#include <stdint.h>

namespace FEXCore::Core {
struct CpuStateFrame;
}

ARG_TO_STR(FEX::HLE::x64::semun, "%lx")

namespace FEX::HLE::x64 {
void RegisterSemaphore(FEX::HLE::SyscallHandler* Handler) {
  REGISTER_SYSCALL_IMPL_X64(semctl, [](FEXCore::Core::CpuStateFrame* Frame, int semid, int semnum, int cmd, FEX::HLE::x64::semun semun) -> uint64_t {
    uint64_t Result {};
    switch (cmd) {
    case IPC_SET: {
      struct semid64_ds buf {};
      FaultSafeUserMemAccess::VerifyIsReadable(semun.buf, sizeof(*semun.buf));
      buf = *semun.buf;
      Result = ::syscall(SYSCALL_DEF(semctl), semid, semnum, cmd, &buf);
      if (Result != -1) {
        FaultSafeUserMemAccess::VerifyIsWritable(semun.buf, sizeof(*semun.buf));
        *semun.buf = buf;
      }
      break;
    }
    case SEM_STAT:
    case SEM_STAT_ANY:
    case IPC_STAT: {
      struct semid64_ds buf {};
      Result = ::syscall(SYSCALL_DEF(semctl), semid, semnum, cmd, &buf);
      if (Result != -1) {
        FaultSafeUserMemAccess::VerifyIsWritable(semun.buf, sizeof(*semun.buf));
        *semun.buf = buf;
      }
      break;
    }
    case SEM_INFO:
    case IPC_INFO: {
      struct fex_seminfo si {};
      Result = ::syscall(SYSCALL_DEF(semctl), semid, semnum, cmd, &si);
      if (Result != -1) {
        FaultSafeUserMemAccess::VerifyIsWritable(semun.__buf, sizeof(si));
        memcpy(semun.__buf, &si, sizeof(si));
      }
      break;
    }
    case GETALL:
    case SETALL: {
      // ptr is just a int32_t* in this case
      Result = ::syscall(SYSCALL_DEF(semctl), semid, semnum, cmd, semun.array);
      break;
    }
    case SETVAL: {
      // ptr is just a int32_t in this case
      Result = ::syscall(SYSCALL_DEF(semctl), semid, semnum, cmd, semun.val);
      break;
    }
    case IPC_RMID:
    case GETPID:
    case GETNCNT:
    case GETZCNT:
    case GETVAL: Result = ::syscall(SYSCALL_DEF(semctl), semid, semnum, cmd, semun); break;
    default: LOGMAN_MSG_A_FMT("Unhandled semctl cmd: {}", cmd); return -EINVAL;
    }
    SYSCALL_ERRNO();
  });
}
} // namespace FEX::HLE::x64
