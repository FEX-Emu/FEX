/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

namespace FEX::HLE {
  void RegisterSemaphore() {
    REGISTER_SYSCALL_IMPL(semget, [](FEXCore::Core::CpuStateFrame *Frame, key_t key, int nsems, int semflg) -> uint64_t {
      uint64_t Result = ::semget(key, nsems, semflg);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(semctl, [](FEXCore::Core::CpuStateFrame *Frame, int semid, int semnum, int cmd, void* semun) -> uint64_t {
      uint64_t Result = ::semctl(semid, semnum, cmd, semun);
      SYSCALL_ERRNO();
    });
  }
}
