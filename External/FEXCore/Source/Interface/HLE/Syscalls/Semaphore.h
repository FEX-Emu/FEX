#pragma once
#include <stdint.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {
  uint64_t Semget(FEXCore::Core::InternalThreadState *Thread, key_t key, int nsems, int semflg);
  uint64_t Semop(FEXCore::Core::InternalThreadState *Thread, int semid, struct sembuf *sops, size_t nsops);
  uint64_t Semctl(FEXCore::Core::InternalThreadState *Thread, int semid, int semnum, int cmd, void* semun);
  uint64_t Semtimedop(FEXCore::Core::InternalThreadState *Thread, int semid, struct sembuf *sops, size_t nsops, const struct timespec *timeout);
}
