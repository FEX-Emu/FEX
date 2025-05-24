// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#include "LinuxSyscalls/LinuxAllocator.h"
#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x64/Syscalls.h"
#include <FEXCore/Core/Context.h>
#include <FEXCore/Debug/InternalThreadState.h>

#include <FEXCore/IR/IR.h>

#include <sys/mman.h>
#include <sys/shm.h>
#include <unistd.h>

#include <FEXCore/Core/Context.h>
#include <FEXCore/Config/Config.h>
#include <FEXCore/Utils/Allocator.h>

namespace FEX::HLE::x64 {

void RegisterMemory(FEX::HLE::SyscallHandler* Handler) {
  using namespace FEXCore::IR;

  REGISTER_SYSCALL_IMPL_X64_FLAGS(
    mmap, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
    [](FEXCore::Core::CpuStateFrame* Frame, void* addr, size_t length, int prot, int flags, int fd, off_t offset) -> uint64_t {
      auto Result = reinterpret_cast<uint64_t>(FEX::HLE::_SyscallHandler->GuestMmap(true, Frame->Thread, addr, length, prot, flags, fd, offset));
      SYSCALL_ERRNO();
    });

  REGISTER_SYSCALL_IMPL_X64_FLAGS(
    munmap, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
    [](FEXCore::Core::CpuStateFrame* Frame, void* addr, size_t length) -> uint64_t {
      uint64_t Result = FEX::HLE::_SyscallHandler->GuestMunmap(true, Frame->Thread, addr, length);

      SYSCALL_ERRNO();
    });

  REGISTER_SYSCALL_IMPL_X64_FLAGS(
    mremap, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
    [](FEXCore::Core::CpuStateFrame* Frame, void* old_address, size_t old_size, size_t new_size, int flags, void* new_address) -> uint64_t {
      uint64_t Result = reinterpret_cast<uint64_t>(::mremap(old_address, old_size, new_size, flags, new_address));

      if (Result != -1) {
        FEX::HLE::_SyscallHandler->TrackMremap(Frame->Thread, (uintptr_t)old_address, old_size, new_size, flags, Result);
      }
      SYSCALL_ERRNO();
    });

  REGISTER_SYSCALL_IMPL_X64_FLAGS(mprotect, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                  [](FEXCore::Core::CpuStateFrame* Frame, void* addr, size_t len, int prot) -> uint64_t {
                                    uint64_t Result = ::mprotect(addr, len, prot);

                                    if (Result != -1) {
                                      FEX::HLE::_SyscallHandler->TrackMprotect(Frame->Thread, (uintptr_t)addr, len, prot);
                                    }
                                    SYSCALL_ERRNO();
                                  });

  REGISTER_SYSCALL_IMPL_X64_FLAGS(shmat, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                  [](FEXCore::Core::CpuStateFrame* Frame, int shmid, const void* shmaddr, int shmflg) -> uint64_t {
                                    uint64_t Result = reinterpret_cast<uint64_t>(shmat(shmid, shmaddr, shmflg));

                                    if (Result != -1) {
                                      FEX::HLE::_SyscallHandler->TrackShmat(Frame->Thread, shmid, Result, shmflg);
                                    }
                                    SYSCALL_ERRNO();
                                  });

  REGISTER_SYSCALL_IMPL_X64_FLAGS(shmdt, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                  [](FEXCore::Core::CpuStateFrame* Frame, const void* shmaddr) -> uint64_t {
                                    uint64_t Result = ::shmdt(shmaddr);

                                    if (Result != -1) {
                                      FEX::HLE::_SyscallHandler->TrackShmdt(Frame->Thread, (uintptr_t)shmaddr);
                                    }
                                    SYSCALL_ERRNO();
                                  });
}
} // namespace FEX::HLE::x64
