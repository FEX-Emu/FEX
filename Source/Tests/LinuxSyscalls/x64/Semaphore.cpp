/*
$info$
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Types.h"

#include <stddef.h>
#include <stdint.h>
#include <sys/sem.h>

namespace FEXCore::Core {
  struct CpuStateFrame;
}

ARG_TO_STR(FEX::HLE::x64::semun, "%lx")

namespace FEX::HLE::x64 {
  void RegisterSemaphore() {
   REGISTER_SYSCALL_IMPL_X64(semop, [](FEXCore::Core::CpuStateFrame *Frame, int semid, struct sembuf *sops, size_t nsops) -> uint64_t {
      uint64_t Result = ::semop(semid, sops, nsops);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(semtimedop, [](FEXCore::Core::CpuStateFrame *Frame, int semid, struct sembuf *sops, size_t nsops, const struct timespec *timeout) -> uint64_t {
      uint64_t Result = ::semtimedop(semid, sops, nsops, timeout);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(semctl, [](FEXCore::Core::CpuStateFrame *Frame, int semid, int semnum, int cmd, FEX::HLE::x64::semun semun) -> uint64_t {
      uint64_t Result{};
      switch (cmd) {
        case IPC_SET: {
          struct semid_ds buf{};
          buf = *semun.buf;
          Result = ::semctl(semid, semnum, cmd, &buf);
          if (Result != -1) {
            *semun.buf = buf;
          }
          break;
        }
        case SEM_STAT:
        case SEM_STAT_ANY:
        case IPC_STAT: {
          struct semid_ds buf{};
          Result = ::semctl(semid, semnum, cmd, &buf);
          if (Result != -1) {
            *semun.buf = buf;
          }
          break;
        }
        case SEM_INFO:
        case IPC_INFO: {
          struct seminfo si{};
          Result = ::semctl(semid, semnum, cmd, &si);
          if (Result != -1) {
            memcpy(semun.__buf, &si, sizeof(si));
          }
          break;
        }
        case GETALL:
        case SETALL: {
          // ptr is just a int32_t* in this case
          Result = ::semctl(semid, semnum, cmd, semun.array);
          break;
        }
        case SETVAL: {
          // ptr is just a int32_t in this case
          Result = ::semctl(semid, semnum, cmd, semun.val);
          break;
        }
        case IPC_RMID:
        case GETPID:
        case GETNCNT:
        case GETZCNT:
        case GETVAL:
          Result = ::semctl(semid, semnum, cmd, semun);
          break;
        default: LOGMAN_MSG_A("Unhandled semctl cmd: %d", cmd); return -EINVAL; break;

      }
      SYSCALL_ERRNO();
    });
  }
}
