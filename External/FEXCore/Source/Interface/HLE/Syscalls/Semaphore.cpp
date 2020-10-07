#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"
#include "Interface/HLE/x32/Syscalls.h"
#include "Interface/Context/Context.h"
#include "Interface/Core/InternalThreadState.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {

  void RegisterSemaphore() {
    REGISTER_SYSCALL_IMPL(semget, [](FEXCore::Core::InternalThreadState *Thread, key_t key, int nsems, int semflg) -> uint64_t {
      uint64_t Result = ::semget(key, nsems, semflg);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(semctl, [](FEXCore::Core::InternalThreadState *Thread, int semid, int semnum, int cmd, void* semun) -> uint64_t {
      uint64_t Result = ::semctl(semid, semnum, cmd, semun);
      SYSCALL_ERRNO();
    });
  }
}
