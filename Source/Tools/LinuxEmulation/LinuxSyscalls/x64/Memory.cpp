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
#include <FEXCore/Utils/MathUtils.h>

namespace FEX::HLE::x64 {

void RegisterMemory(FEX::HLE::SyscallHandler* Handler) {
  using namespace FEXCore::IR;

  REGISTER_SYSCALL_IMPL_X64_FLAGS(
    mmap, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
    [](FEXCore::Core::CpuStateFrame* Frame, void* addr, size_t length, int prot, int flags, int fd, off_t offset) -> uint64_t {
      return (uint64_t)FEX::HLE::_SyscallHandler->GuestMmap(Frame->Thread, addr, length, prot, flags, fd, offset);
    });

  REGISTER_SYSCALL_IMPL_X64_FLAGS(munmap, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                  [](FEXCore::Core::CpuStateFrame* Frame, void* addr, size_t length) -> uint64_t {
                                    return FEX::HLE::_SyscallHandler->GuestMunmap(Frame->Thread, addr, length);
                                  });

  REGISTER_SYSCALL_IMPL_X64_FLAGS(
    mremap, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
    [](FEXCore::Core::CpuStateFrame* Frame, void* old_address, size_t old_size, size_t new_size, int flags, void* new_address) -> uint64_t {
      return FEX::HLE::_SyscallHandler->GuestMremap(true, Frame->Thread, old_address, old_size, new_size, flags, new_address);
    });

  REGISTER_SYSCALL_IMPL_X64_FLAGS(mprotect, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                  [](FEXCore::Core::CpuStateFrame* Frame, void* addr, size_t len, int prot) -> uint64_t {
                                    return FEX::HLE::_SyscallHandler->GuestMprotect(Frame->Thread, addr, len, prot);
                                  });

  REGISTER_SYSCALL_IMPL_X64_FLAGS(shmat, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                  ([](FEXCore::Core::CpuStateFrame* Frame, int shmid, const void* shmaddr, int shmflg) -> uint64_t {
                                    return FEX::HLE::_SyscallHandler->GuestShmat(true, Frame->Thread, shmid, shmaddr, shmflg);
                                  }));

  REGISTER_SYSCALL_IMPL_X64_FLAGS(shmdt, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                  [](FEXCore::Core::CpuStateFrame* Frame, const void* shmaddr) -> uint64_t {
                                    return FEX::HLE::_SyscallHandler->GuestShmdt(true, Frame->Thread, shmaddr);
                                  });
}
} // namespace FEX::HLE::x64
