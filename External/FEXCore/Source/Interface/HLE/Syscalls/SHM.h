#pragma once
#include <stddef.h>
#include <stdint.h>
#include <sys/shm.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {
  uint64_t Shmget(FEXCore::Core::InternalThreadState *Thread, key_t key, size_t size, int shmflg);
  uint64_t Shmat(FEXCore::Core::InternalThreadState *Thread, int shmid, const void *shmaddr, int shmflg);
  uint64_t Shmctl(FEXCore::Core::InternalThreadState *Thread, int shmid, int cmd, struct shmid_ds *buf);
  uint64_t Shmdt(FEXCore::Core::InternalThreadState *Thread, const void *shmaddr);

}
