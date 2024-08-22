// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x32/Syscalls.h"
#include "LinuxSyscalls/SignalDelegator.h"

#include <stddef.h>
#include <stdint.h>
#include <sys/mount.h>
#include <unistd.h>

namespace FEX::HLE::x32 {
void RegisterFS(FEX::HLE::SyscallHandler* Handler) {
  REGISTER_SYSCALL_IMPL_X32(umount, [](FEXCore::Core::CpuStateFrame* Frame, const char* target) -> uint64_t {
    uint64_t Result = ::umount(target);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(
    truncate64, [](FEXCore::Core::CpuStateFrame* Frame, const char* path, uint32_t offset_low, uint32_t offset_high) -> uint64_t {
      uint64_t Offset = offset_high;
      Offset <<= 32;
      Offset |= offset_low;
      uint64_t Result = ::truncate(path, Offset);
      SYSCALL_ERRNO();
    });

  REGISTER_SYSCALL_IMPL_X32(ftruncate64, [](FEXCore::Core::CpuStateFrame* Frame, int fd, uint32_t offset_low, uint32_t offset_high) -> uint64_t {
    uint64_t Offset = offset_high;
    Offset <<= 32;
    Offset |= offset_low;
    uint64_t Result = ::ftruncate(fd, Offset);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(
    sigprocmask, [](FEXCore::Core::CpuStateFrame* Frame, int how, const uint64_t* set, uint64_t* oldset, size_t sigsetsize) -> uint64_t {
      return FEX::HLE::_SyscallHandler->GetSignalDelegator()->GuestSigProcMask(FEX::HLE::ThreadManager::GetStateObjectFromCPUState(Frame),
                                                                               how, set, oldset);
    });
}
} // namespace FEX::HLE::x32
