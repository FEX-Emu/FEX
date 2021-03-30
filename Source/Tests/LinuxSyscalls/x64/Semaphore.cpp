/*
$info$
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include <FEXCore/Core/Context.h>

#include <sys/sem.h>

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
  }
}
