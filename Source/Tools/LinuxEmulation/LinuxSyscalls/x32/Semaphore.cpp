// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x32/Syscalls.h"
#include "LinuxSyscalls/x32/Types.h"

#include "LinuxSyscalls/x64/Syscalls.h"

#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/vector.h>
#include <FEXHeaderUtils/Syscalls.h>

#include <cstdint>
#include <errno.h>
#include <limits>
#include <string.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <time.h>
#include <type_traits>

namespace FEXCore::Core {
struct CpuStateFrame;
}

namespace FEX::HLE::x32 {
// Define the IPC ops
enum IPCOp {
  OP_SEMOP = 1,
  OP_SEMGET = 2,
  OP_SEMCTL = 3,
  OP_SEMTIMEDOP = 4,
  OP_MSGSND = 11,
  OP_MSGRCV = 12,
  OP_MSGGET = 13,
  OP_MSGCTL = 14,
  OP_SHMAT = 21,
  OP_SHMDT = 22,
  OP_SHMGET = 23,
  OP_SHMCTL = 24,
};

struct msgbuf_32 {
  compat_long_t mtype;
  char mtext[1];
};

union semun_32 {
  int32_t val;                          // Value for SETVAL
  compat_ptr<semid_ds_32> buf32;        // struct semid_ds* - Buffer ptr for IPC_STAT, IPC_SET
  compat_ptr<semid_ds_64> buf64;        // struct semid_ds* - Buffer ptr for IPC_STAT, IPC_SET
  uint32_t array;                       // uint16_t array for GETALL, SETALL
  compat_ptr<struct fex_seminfo> __buf; // struct seminfo * - Buffer for IPC_INFO
};

union msgun_32 {
  int32_t val;                      // Value for SETVAL
  compat_ptr<msqid_ds_32> buf32;    // struct msgid_ds* - Buffer ptr for IPC_STAT, IPC_SET
  compat_ptr<msqid_ds_64> buf64;    // struct msgid_ds* - Buffer ptr for IPC_STAT, IPC_SET
  uint32_t array;                   // uint16_t array for GETALL, SETALL
  compat_ptr<struct msginfo> __buf; // struct msginfo * - Buffer for IPC_INFO
};

union shmun_32 {
  int32_t val;                           // Value for SETVAL
  compat_ptr<shmid_ds_32> buf32;         // struct shmid_ds* - Buffer ptr for IPC_STAT, IPC_SET
  compat_ptr<shmid_ds_64> buf64;         // struct shmid_ds* - Buffer ptr for IPC_STAT, IPC_SET
  uint32_t array;                        // uint16_t array for GETALL, SETALL
  compat_ptr<struct shminfo_32> __buf32; // struct shminfo * - Buffer for IPC_INFO
  compat_ptr<struct shminfo_64> __buf64; // struct shminfo * - Buffer for IPC_INFO

  compat_ptr<struct shm_info_32> __buf_info_32; // struct shm_info * - Buffer for SHM_INFO
};

union semun {
  int val;                   /* value for SETVAL */
  struct semid_ds_32* buf;   /* buffer for IPC_STAT & IPC_SET */
  unsigned short* array;     /* array for GETALL & SETALL */
  struct fex_seminfo* __buf; /* buffer for IPC_INFO */
  void* __pad;
};

uint64_t _ipc(FEXCore::Core::CpuStateFrame* Frame, uint32_t call, uint32_t first, uint32_t second, uint32_t third, uint32_t ptr, uint32_t fifth) {
  uint64_t Result {};

  const int Version = call >> 16;
  call &= 0xffff;

  switch (static_cast<IPCOp>(call)) {
  case OP_SEMOP: {
    Result = ::syscall(SYSCALL_DEF(semop), first, reinterpret_cast<struct sembuf*>(ptr), second);
    break;
  }
  case OP_SEMGET: {
    Result = ::syscall(SYSCALL_DEF(semget), first, second, third);
    break;
  }
  case OP_SEMCTL: {
    uint32_t semid = first;
    uint32_t semnum = second;
    // Upper 16bits used for a different flag?
    int32_t cmd = third & 0xFF;
    auto_compat_ptr<semun_32> semun(ptr);
    bool IPC64 = third & 0x100;
    switch (cmd) {
    case IPC_SET: {
      struct semid64_ds buf {};
      if (IPC64) {
        FaultSafeUserMemAccess::VerifyIsReadable(semun->buf64, sizeof(*semun->buf64));
        buf = *semun->buf64;
      } else {
        FaultSafeUserMemAccess::VerifyIsReadable(semun->buf32, sizeof(*semun->buf32));
        buf = *semun->buf32;
      }
      Result = ::syscall(SYSCALL_DEF(semctl), semid, semnum, cmd, &buf);
      if (Result != -1) {
        if (IPC64) {
          FaultSafeUserMemAccess::VerifyIsWritable(semun->buf64, sizeof(*semun->buf64));
          *semun->buf64 = buf;
        } else {
          FaultSafeUserMemAccess::VerifyIsWritable(semun->buf32, sizeof(*semun->buf32));
          *semun->buf32 = buf;
        }
      }
      break;
    }
    case SEM_STAT:
    case SEM_STAT_ANY:
    case IPC_STAT: {
      struct semid64_ds buf {};
      Result = ::syscall(SYSCALL_DEF(semctl), semid, semnum, cmd, &buf);
      if (Result != -1) {
        if (IPC64) {
          FaultSafeUserMemAccess::VerifyIsWritable(semun->buf64, sizeof(*semun->buf64));
          *semun->buf64 = buf;
        } else {
          FaultSafeUserMemAccess::VerifyIsWritable(semun->buf32, sizeof(*semun->buf32));
          *semun->buf32 = buf;
        }
      }
      break;
    }
    case SEM_INFO:
    case IPC_INFO: {
      struct fex_seminfo si {};
      Result = ::syscall(SYSCALL_DEF(semctl), semid, semnum, cmd, &si);
      if (Result != -1) {
        FaultSafeUserMemAccess::VerifyIsWritable(semun->__buf, sizeof(*semun->__buf));
        memcpy(semun->__buf, &si, sizeof(si));
      }
      break;
    }
    case GETALL:
    case SETALL: {
      // ptr is just a int32_t* in this case
      Result = ::syscall(SYSCALL_DEF(semctl), semid, semnum, cmd, semun->array);
      break;
    }
    case SETVAL: {
      // ptr is just a int32_t in this case
      Result = ::syscall(SYSCALL_DEF(semctl), semid, semnum, cmd, semun->val);
      break;
    }
    case IPC_RMID:
    case GETPID:
    case GETNCNT:
    case GETZCNT:
    case GETVAL: Result = ::syscall(SYSCALL_DEF(semctl), semid, semnum, cmd); break;
    default: LOGMAN_MSG_A_FMT("Unhandled semctl cmd: {}", cmd); return -EINVAL;
    }
    break;
  }
  case OP_SEMTIMEDOP: {
    timespec32* timeout = reinterpret_cast<timespec32*>(fifth);
    struct timespec tp64 {};
    struct timespec* timed_ptr {};
    if (timeout) {
      FaultSafeUserMemAccess::VerifyIsReadable(timeout, sizeof(*timeout));
      tp64 = *timeout;
      timed_ptr = &tp64;
    }

    Result = ::syscall(SYSCALL_DEF(semtimedop), first, reinterpret_cast<struct sembuf*>(ptr), second, timed_ptr);
    break;
  }
  case OP_MSGSND: {
    // Requires a temporary buffer
    fextl::vector<uint8_t> Tmp(second + sizeof(size_t));
    struct msgbuf* TmpMsg = reinterpret_cast<struct msgbuf*>(&Tmp.at(0));
    msgbuf_32* src = reinterpret_cast<msgbuf_32*>(ptr);
    FaultSafeUserMemAccess::VerifyIsReadable(src, sizeof(*src));
    FaultSafeUserMemAccess::VerifyIsReadable(src->mtext, second);
    TmpMsg->mtype = src->mtype;
    memcpy(TmpMsg->mtext, src->mtext, second);

    Result = ::syscall(SYSCALL_DEF(msgsnd), first, TmpMsg, second, third);
    break;
  }
  case OP_MSGRCV: {
    fextl::vector<uint8_t> Tmp(second + sizeof(size_t));
    struct msgbuf* TmpMsg = reinterpret_cast<struct msgbuf*>(&Tmp.at(0));

    if (Version != 0) {
      Result = ::syscall(SYSCALL_DEF(msgrcv), first, TmpMsg, second, fifth, third);
      if (Result != -1) {
        msgbuf_32* src = reinterpret_cast<msgbuf_32*>(ptr);
        FaultSafeUserMemAccess::VerifyIsWritable(src, sizeof(*src));
        FaultSafeUserMemAccess::VerifyIsWritable(src->mtext, Result);
        src->mtype = TmpMsg->mtype;
        memcpy(src->mtext, TmpMsg->mtext, Result);
      }

    } else {
      struct compat_ipc_kludge {
        compat_uptr_t msgp;
        compat_long_t msgtyp;
      };
      compat_ipc_kludge* ipck = reinterpret_cast<compat_ipc_kludge*>(ptr);
      Result = ::syscall(SYSCALL_DEF(msgrcv), first, TmpMsg, second, ipck->msgtyp, third);
      if (Result != -1) {
        msgbuf_32* src = reinterpret_cast<msgbuf_32*>(ipck->msgp);
        FaultSafeUserMemAccess::VerifyIsWritable(src, sizeof(*src));
        FaultSafeUserMemAccess::VerifyIsWritable(src->mtext, Result);
        ipck->msgtyp = TmpMsg->mtype;
        memcpy(src->mtext, TmpMsg->mtext, Result);
      }
    }

    break;
  }
  case OP_MSGGET: {
    Result = ::syscall(SYSCALL_DEF(msgget), first, second);
    break;
  }
  case OP_MSGCTL: {
    uint32_t msqid = first;
    int32_t cmd = second & 0xFF;
    msgun_32 msgun {};
    msgun.val = ptr;
    bool IPC64 = second & 0x100;
    switch (cmd) {
    case IPC_SET: {
      struct msqid64_ds buf {};
      if (IPC64) {
        FaultSafeUserMemAccess::VerifyIsReadable(msgun.buf64, sizeof(*msgun.buf64));
        buf = *msgun.buf64;
      } else {
        FaultSafeUserMemAccess::VerifyIsReadable(msgun.buf32, sizeof(*msgun.buf32));
        buf = *msgun.buf32;
      }
      Result = ::syscall(SYSCALL_DEF(msgctl), msqid, cmd, &buf);
      break;
    }
    case MSG_STAT:
    case MSG_STAT_ANY:
    case IPC_STAT: {
      struct msqid64_ds buf {};
      Result = ::syscall(SYSCALL_DEF(msgctl), msqid, cmd, &buf);
      if (Result != -1) {
        if (IPC64) {
          FaultSafeUserMemAccess::VerifyIsWritable(msgun.buf64, sizeof(*msgun.buf64));
          *msgun.buf64 = buf;
        } else {
          FaultSafeUserMemAccess::VerifyIsWritable(msgun.buf32, sizeof(*msgun.buf32));
          *msgun.buf32 = buf;
        }
      }
      break;
    }
    case MSG_INFO:
    case IPC_INFO: {
      struct msginfo mi {};
      Result = ::syscall(SYSCALL_DEF(msgctl), msqid, cmd, reinterpret_cast<struct msqid_ds*>(&mi));
      if (Result != -1) {
        FaultSafeUserMemAccess::VerifyIsWritable(msgun.__buf, sizeof(mi));
        memcpy(msgun.__buf, &mi, sizeof(mi));
      }
      break;
    }
    case IPC_RMID: Result = ::syscall(SYSCALL_DEF(msgctl), msqid, cmd, nullptr); break;
    default: LOGMAN_MSG_A_FMT("Unhandled msgctl cmd: {}", cmd); return -EINVAL;
    }
    break;
  }
  case OP_SHMAT: {
    if (Version == 1) {
      // shmat explicitly doesn't support version 1.
      return -EINVAL;
    }
    // also implemented in memory:shmat
    auto Thread = Frame->Thread;
    auto CTX = Thread->CTX;
    uint64_t Result {};
    uint32_t ResultAddr {};
    uint64_t Length {};
    CTX->MarkMemoryShared(Thread);

    {
      auto lk = FEXCore::GuardSignalDeferringSection(FEX::HLE::_SyscallHandler->VMATracking.Mutex, Thread);

      Result = static_cast<FEX::HLE::x32::x32SyscallHandler*>(FEX::HLE::_SyscallHandler)
                 ->GetAllocator()
                 ->Shmat(first, reinterpret_cast<const void*>(ptr), second, &ResultAddr);

      if (FEX::HLE::HasSyscallError(Result)) {
        return Result;
      }

      shmid_ds stat;

      [[maybe_unused]] auto res = shmctl(first, IPC_STAT, &stat);
      LOGMAN_THROW_A_FMT(res != -1, "shmctl IPC_STAT failed");

      Length = stat.shm_segsz;
      FEX::HLE::_SyscallHandler->TrackShmat(Thread, first, ResultAddr, second, Length);
    }

    FEX::HLE::_SyscallHandler->InvalidateCodeRangeIfNecessary(Thread, ResultAddr, Length);

    *reinterpret_cast<uint32_t*>(third) = ResultAddr;
    return Result;
  }
  case OP_SHMDT: {
    // also implemented in memory:shmdt
    auto Thread = Frame->Thread;
    uint64_t Result {};
    uint64_t Length {};
    {
      auto lk = FEXCore::GuardSignalDeferringSection(FEX::HLE::_SyscallHandler->VMATracking.Mutex, Thread);
      Result =
        static_cast<FEX::HLE::x32::x32SyscallHandler*>(FEX::HLE::_SyscallHandler)->GetAllocator()->Shmdt(reinterpret_cast<const void*>(ptr));

      if (FEX::HLE::HasSyscallError(Result)) {
        return Result;
      }

      Length = FEX::HLE::_SyscallHandler->TrackShmdt(Thread, static_cast<uint64_t>(ptr));
    }

    FEX::HLE::_SyscallHandler->InvalidateCodeRangeIfNecessary(Thread, static_cast<uint64_t>(ptr), Length);
    return Result;
  }
  case OP_SHMGET: {
    Result = ::shmget(first, second, third);
    break;
  }
  case OP_SHMCTL: {
    int32_t shmid = first;
    int32_t shmcmd = second;
    int32_t cmd = shmcmd & 0xFF;
    bool IPC64 = shmcmd & 0x100;
    shmun_32 shmun {};
    shmun.val = reinterpret_cast<uint32_t>(ptr);

    switch (cmd) {
    case IPC_SET: {
      struct shmid64_ds buf {};
      if (IPC64) {
        FaultSafeUserMemAccess::VerifyIsReadable(shmun.buf64, sizeof(*shmun.buf64));
        buf = *shmun.buf64;
      } else {
        FaultSafeUserMemAccess::VerifyIsReadable(shmun.buf32, sizeof(*shmun.buf32));
        buf = *shmun.buf32;
      }
      Result = ::syscall(SYSCALL_DEF(shmctl), shmid, cmd, &buf);
      // IPC_SET sets the internal data structure that the kernel uses
      // No need to writeback
      break;
    }
    case SHM_STAT:
    case SHM_STAT_ANY:
    case IPC_STAT: {
      struct shmid64_ds buf {};
      Result = ::syscall(SYSCALL_DEF(shmctl), shmid, cmd, &buf);
      if (Result != -1) {
        if (IPC64) {
          FaultSafeUserMemAccess::VerifyIsWritable(shmun.buf64, sizeof(*shmun.buf64));
          *shmun.buf64 = buf;
        } else {
          FaultSafeUserMemAccess::VerifyIsWritable(shmun.buf32, sizeof(*shmun.buf32));
          *shmun.buf32 = buf;
        }
      }
      break;
    }
    case IPC_INFO: {
      struct shminfo si {};
      Result = ::syscall(SYSCALL_DEF(shmctl), shmid, cmd, reinterpret_cast<struct shmid_ds*>(&si));
      if (Result != -1) {
        if (IPC64) {
          FaultSafeUserMemAccess::VerifyIsWritable(shmun.__buf64, sizeof(*shmun.__buf64));
          *shmun.__buf64 = si;
        } else {
          FaultSafeUserMemAccess::VerifyIsWritable(shmun.__buf32, sizeof(*shmun.__buf32));
          *shmun.__buf32 = si;
        }
      }
      break;
    }
    case SHM_INFO: {
      struct shm_info si {};
      Result = ::syscall(SYSCALL_DEF(shmctl), shmid, cmd, reinterpret_cast<struct shmid_ds*>(&si));
      if (Result != -1) {
        FaultSafeUserMemAccess::VerifyIsWritable(shmun.__buf_info_32, sizeof(*shmun.__buf_info_32));
        // SHM_INFO doesn't follow IPC64 behaviour
        *shmun.__buf_info_32 = si;
      }
      break;
    }
    case SHM_LOCK: Result = ::syscall(SYSCALL_DEF(shmctl), shmid, cmd, nullptr); break;
    case SHM_UNLOCK: Result = ::syscall(SYSCALL_DEF(shmctl), shmid, cmd, nullptr); break;
    case IPC_RMID: Result = ::syscall(SYSCALL_DEF(shmctl), shmid, cmd, nullptr); break;

    default: LOGMAN_MSG_A_FMT("Unhandled shmctl cmd: {}", cmd); return -EINVAL;
    }
    break;
  }

  default: return -ENOSYS;
  }
  SYSCALL_ERRNO();
}
void RegisterSemaphore(FEX::HLE::SyscallHandler* Handler) {
  REGISTER_SYSCALL_IMPL_X32(ipc, _ipc);

  REGISTER_SYSCALL_IMPL_X32(semctl, [](FEXCore::Core::CpuStateFrame* Frame, int semid, int semnum, int cmd, semun_32* semun) -> uint64_t {
    uint64_t Result {};
    bool IPC64 = cmd & 0x100;

    switch (cmd) {
    case IPC_SET: {
      struct semid64_ds buf {};
      if (IPC64) {
        FaultSafeUserMemAccess::VerifyIsReadable(semun->buf64, sizeof(*semun->buf64));
        buf = *semun->buf64;
      } else {
        FaultSafeUserMemAccess::VerifyIsReadable(semun->buf32, sizeof(*semun->buf32));
        buf = *semun->buf32;
      }
      Result = ::syscall(SYSCALL_DEF(semctl), semid, semnum, cmd, &buf);
      if (Result != -1) {
        if (IPC64) {
          FaultSafeUserMemAccess::VerifyIsWritable(semun->buf64, sizeof(*semun->buf64));
          *semun->buf64 = buf;
        } else {
          FaultSafeUserMemAccess::VerifyIsWritable(semun->buf32, sizeof(*semun->buf32));
          *semun->buf32 = buf;
        }
      }
      break;
    }
    case SEM_STAT:
    case SEM_STAT_ANY:
    case IPC_STAT: {
      struct semid64_ds buf {};
      Result = ::syscall(SYSCALL_DEF(semctl), semid, semnum, cmd, &buf);
      if (Result != -1) {
        if (IPC64) {
          FaultSafeUserMemAccess::VerifyIsWritable(semun->buf64, sizeof(*semun->buf64));
          *semun->buf64 = buf;
        } else {
          FaultSafeUserMemAccess::VerifyIsWritable(semun->buf32, sizeof(*semun->buf32));
          *semun->buf32 = buf;
        }
      }
      break;
    }
    case SEM_INFO:
    case IPC_INFO: {
      struct fex_seminfo si {};
      Result = ::syscall(SYSCALL_DEF(semctl), semid, semnum, cmd, &si);
      if (Result != -1) {
        FaultSafeUserMemAccess::VerifyIsWritable(semun->__buf, sizeof(*semun->__buf));
        memcpy(semun->__buf, &si, sizeof(si));
      }
      break;
    }
    case GETALL:
    case SETALL: {
      // ptr is just a int32_t* in this case
      Result = ::syscall(SYSCALL_DEF(semctl), semid, semnum, cmd, semun->array);
      break;
    }
    case SETVAL: {
      // ptr is just a int32_t in this case
      Result = ::syscall(SYSCALL_DEF(semctl), semid, semnum, cmd, semun->val);
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
} // namespace FEX::HLE::x32
