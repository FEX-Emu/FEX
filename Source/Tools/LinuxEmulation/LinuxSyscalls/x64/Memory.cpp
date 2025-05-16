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

void* x64SyscallHandler::GuestMmap(FEXCore::Core::InternalThreadState* Thread, void* addr, size_t length, int prot, int flags, int fd, off_t offset) {
  return reinterpret_cast<void*>(EmulateMmap(
    Thread, addr, length, prot, flags, fd, offset, [](void* addr, size_t length, int prot, int flags, int fd, off_t offset) -> uint64_t {
      bool Map32Bit = flags & FEX::HLE::X86_64_MAP_32BIT;
      if (Map32Bit) {
        return (uint64_t)FEX::HLE::_SyscallHandler->Get32BitAllocator()->Mmap(addr, length, prot, flags, fd, offset);
      }

      uint64_t Result = reinterpret_cast<uint64_t>(::mmap(reinterpret_cast<void*>(addr), length, prot, flags, fd, offset));
      SYSCALL_ERRNO();
    }));
}

uint64_t x64SyscallHandler::GuestMunmap(FEXCore::Core::InternalThreadState* Thread, void* addr, uint64_t length) {
  return EmulateMunmap(Thread, addr, length, [](void* addr, uint64_t length) -> uint64_t {
    if (reinterpret_cast<uintptr_t>(addr) < 0x1'0000'0000ULL) {
      return FEX::HLE::_SyscallHandler->Get32BitAllocator()->Munmap(addr, length);
    }

    uint64_t Result = ::munmap(addr, length);
    SYSCALL_ERRNO();
  });
}

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
      return FEX::HLE::_SyscallHandler->EmulateMremap(
        Frame->Thread, old_address, old_size, new_size, flags, new_address,
        [](void* old_address, size_t old_size, size_t new_size, int flags, void* new_address) -> uint64_t {
          uint64_t Result = reinterpret_cast<uint64_t>(::mremap(old_address, old_size, new_size, flags, new_address));
          SYSCALL_ERRNO();
        });
    });

  REGISTER_SYSCALL_IMPL_X64_FLAGS(mprotect, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                  [](FEXCore::Core::CpuStateFrame* Frame, void* addr, size_t len, int prot) -> uint64_t {
                                    return FEX::HLE::_SyscallHandler->EmulateMprotect(Frame->Thread, addr, len, prot);
                                  });

  REGISTER_SYSCALL_IMPL_X64_FLAGS(shmat, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                  [](FEXCore::Core::CpuStateFrame* Frame, int shmid, const void* shmaddr, int shmflg) -> uint64_t {
                                    return FEX::HLE::_SyscallHandler->EmulateShmat(
                                      Frame->Thread, shmid, shmaddr, shmflg, [](int shmid, const void* shmaddr, int shmflg) -> uint64_t {
                                        uint64_t Result = reinterpret_cast<uint64_t>(::shmat(shmid, shmaddr, shmflg));
                                        SYSCALL_ERRNO();
                                      });
                                  });

  REGISTER_SYSCALL_IMPL_X64_FLAGS(shmdt, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                  [](FEXCore::Core::CpuStateFrame* Frame, const void* shmaddr) -> uint64_t {
                                    return FEX::HLE::_SyscallHandler->EmulateShmdt(Frame->Thread, shmaddr, [](const void* shmaddr) -> uint64_t {
                                      uint64_t Result = ::shmdt(shmaddr);
                                      SYSCALL_ERRNO();
                                    });
                                  });
}
} // namespace FEX::HLE::x64
