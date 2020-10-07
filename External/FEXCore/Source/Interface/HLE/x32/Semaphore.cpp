#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x32/Syscalls.h"
#include "Interface/Context/Context.h"

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE::x32 {
  // Define the IPC ops
  enum IPCOp {
    OP_SEMOP      = 1,
    OP_SEMGET     = 2,
    OP_SEMCTL     = 3,
    OP_SEMTIMEDOP = 4,
    OP_MSGSND     = 11,
    OP_MSGRCV     = 12,
    OP_MSGGET     = 13,
    OP_MSGCTL     = 14,
    OP_SHMAT      = 21,
    OP_SHMDT      = 22,
    OP_SHMGET     = 23,
    OP_SHMCTL     = 24,
  };

  void RegisterSemaphore() {
    REGISTER_SYSCALL_IMPL_X32(ipc, [](FEXCore::Core::InternalThreadState *Thread, uint32_t call, int32_t first, int32_t second, int32_t third, uint32_t ptr, int32_t fifth) -> uint32_t {
      uint64_t Result{};
      switch (static_cast<IPCOp>(call)) {
        case OP_SEMOP: {
          Result = ::semop(first, reinterpret_cast<struct sembuf*>(ptr), second);
          break;
        }
        case OP_SEMGET: {
          Result = ::semget(first, second, third);
          break;
        }
        case OP_SEMCTL: {
          Result = ::semctl(first, second, third, ptr);
          break;
        }
        case OP_SEMTIMEDOP: {
          Result = ::semtimedop(first, reinterpret_cast<struct sembuf*>(ptr), second, reinterpret_cast<struct timespec*>(fifth));
          break;
        }
        case OP_MSGSND: {
          Result = ::msgsnd(first, reinterpret_cast<void*>(ptr), second, third);
          break;
        }
        case OP_MSGRCV: {
          Result = ::msgrcv(first, reinterpret_cast<void*>(ptr), second, fifth, third);
          break;
        }
        case OP_MSGGET: {
          Result = ::msgget(first, second);
          break;
        }
        case OP_MSGCTL: {
          Result = ::msgctl(first, second, reinterpret_cast<struct msqid_ds*>(ptr));
          break;
        }
        case OP_SHMAT: {
          Result = reinterpret_cast<uint64_t>(::shmat(first, reinterpret_cast<void*>(ptr), second));
          if (Result != -1) {
            *reinterpret_cast<uint32_t*>(third) = static_cast<uint32_t>(Result);
            Result = 0;
          }
          break;
        }
        case OP_SHMDT: {
          Result = ::shmdt(reinterpret_cast<void*>(ptr));
          break;
        }
        case OP_SHMGET: {
          Result = ::shmget(first, second, third);
          break;
        }
        case OP_SHMCTL: {
          Result = ::shmctl(first, second, reinterpret_cast<struct shmid_ds*>(ptr));
          break;
        }

        default: return -ENOSYS;
      }
      SYSCALL_ERRNO();
    });
  }
}
