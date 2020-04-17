#include "Interface/HLE/Syscalls.h"
#include "Interface/Context/Context.h"
#include "Interface/Core/InternalThreadState.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {
  uint64_t Semget(FEXCore::Core::InternalThreadState *Thread, key_t key, int nsems, int semflg) {
    uint64_t Result = ::semget(key, nsems, semflg);
    SYSCALL_ERRNO();
  }

  uint64_t Semop(FEXCore::Core::InternalThreadState *Thread, int semid, struct sembuf *sops, size_t nsops) {
    uint64_t Result = ::semop(semid, sops, nsops);
    SYSCALL_ERRNO();
  }

  uint64_t Semctl(FEXCore::Core::InternalThreadState *Thread, int semid, int semnum, int cmd, void* semun) {
    uint64_t Result = ::semctl(semid, semnum, cmd, semun);
    SYSCALL_ERRNO();
  }

  uint64_t Semtimedop(FEXCore::Core::InternalThreadState *Thread, int semid, struct sembuf *sops, size_t nsops, const struct timespec *timeout) {
    uint64_t Result = ::semtimedop(semid, sops, nsops, timeout);
    SYSCALL_ERRNO();
  }
}
