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
      auto Thread = Frame->Thread;
      uint64_t Result {};

      {
        auto lk = FEXCore::GuardSignalDeferringSection(FEX::HLE::_SyscallHandler->VMATracking.Mutex, Thread);
        Result = reinterpret_cast<uint64_t>(::mremap(old_address, old_size, new_size, flags, new_address));

        if (Result == -1) {
          SYSCALL_ERRNO();
        }
        FEX::HLE::_SyscallHandler->TrackMremap(Thread, reinterpret_cast<uint64_t>(old_address), old_size, new_size, flags, Result);
      }

      FEX::HLE::_SyscallHandler->InvalidateCodeRangeIfNecessaryOnRemap(Thread, reinterpret_cast<uint64_t>(old_address), Result, old_size, new_size);

      return Result;
    });

  REGISTER_SYSCALL_IMPL_X64_FLAGS(mprotect, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                  [](FEXCore::Core::CpuStateFrame* Frame, void* addr, size_t len, int prot) -> uint64_t {
                                    auto Result = FEX::HLE::_SyscallHandler->GuestMprotect(Frame->Thread, addr, len, prot);
                                    SYSCALL_ERRNO();
                                  });

  REGISTER_SYSCALL_IMPL_X64_FLAGS(shmat, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                  ([](FEXCore::Core::CpuStateFrame* Frame, int shmid, const void* shmaddr, int shmflg) -> uint64_t {
                                    auto Result = FEX::HLE::_SyscallHandler->GuestShmat(true, Frame->Thread, shmid, shmaddr, shmflg);
                                    SYSCALL_ERRNO();
                                  }));

  REGISTER_SYSCALL_IMPL_X64_FLAGS(
    shmdt, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY, [](FEXCore::Core::CpuStateFrame* Frame, const void* shmaddr) -> uint64_t {
      auto Thread = Frame->Thread;
      uint64_t Result {};
      uint64_t Length {};
      {
        auto lk = FEXCore::GuardSignalDeferringSection(FEX::HLE::_SyscallHandler->VMATracking.Mutex, Thread);
        Result = ::shmdt(shmaddr);

        if (Result == -1) {
          SYSCALL_ERRNO();
          return Result;
        }

        Length = FEX::HLE::_SyscallHandler->TrackShmdt(Thread, reinterpret_cast<uintptr_t>(shmaddr));
      }

      FEX::HLE::_SyscallHandler->InvalidateCodeRangeIfNecessary(Thread, reinterpret_cast<uintptr_t>(shmaddr), Length);
      SYSCALL_ERRNO();
    });
}
} // namespace FEX::HLE::x64
