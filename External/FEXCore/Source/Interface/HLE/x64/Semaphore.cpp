#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"
#include "Interface/Context/Context.h"

#include <sys/sem.h>

namespace FEXCore::HLE::x64 {

  void RegisterSemaphore() {
   REGISTER_SYSCALL_IMPL_X64(semop, [](FEXCore::Core::InternalThreadState *Thread, int semid, struct sembuf *sops, size_t nsops) -> uint64_t {
      uint64_t Result = ::semop(semid, sops, nsops);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(semtimedop, [](FEXCore::Core::InternalThreadState *Thread, int semid, struct sembuf *sops, size_t nsops, const struct timespec *timeout) -> uint64_t {
      uint64_t Result = ::semtimedop(semid, sops, nsops, timeout);
      SYSCALL_ERRNO();
    });
  }
}
