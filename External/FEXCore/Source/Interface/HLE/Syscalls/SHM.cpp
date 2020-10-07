#include "Interface/Context/Context.h"
#include "Interface/Core/InternalThreadState.h"
#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"
#include "Interface/HLE/x32/Syscalls.h"

#include <stddef.h>
#include <stdint.h>
#include <sys/shm.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {

  void RegisterSHM() {
    REGISTER_SYSCALL_IMPL(shmget, [](FEXCore::Core::InternalThreadState *Thread, key_t key, size_t size, int shmflg) -> uint64_t {
      uint64_t Result = shmget(key, size, shmflg);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(shmat, [](FEXCore::Core::InternalThreadState *Thread, int shmid, const void *shmaddr, int shmflg) -> uint64_t {
      const void* HostPointer{};
      uint64_t BasePointer = Thread->CTX->MemoryMapper.GetPointer<uint64_t>(0);
      if (shmaddr) {
        HostPointer = shmaddr;
      }
      else {
        HostPointer = reinterpret_cast<void*>(BasePointer + Thread->CTX->MemoryMapper.GetSHMSize());
      }

      uint64_t Result = reinterpret_cast<uint64_t>(shmat(shmid, HostPointer, shmflg));
      if (Result != -1 && !shmaddr) {
        Result -= BasePointer;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(shmctl, [](FEXCore::Core::InternalThreadState *Thread, int shmid, int cmd, struct shmid_ds *buf) -> uint64_t {
      uint64_t Result = ::shmctl(shmid, cmd, buf);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(shmdt, [](FEXCore::Core::InternalThreadState *Thread, const void *shmaddr) -> uint64_t {
      uint64_t Result = ::shmdt(shmaddr);
      SYSCALL_ERRNO();
    });
  }
}
