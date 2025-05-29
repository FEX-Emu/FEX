// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x32/Syscalls.h"
#include "LinuxSyscalls/x64/Syscalls.h"
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/Utils/MathUtils.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <system_error>
#include <filesystem>

namespace FEX::HLE::x32 {

void RegisterMemory(FEX::HLE::SyscallHandler* Handler) {
  struct old_mmap_struct {
    uint32_t addr;
    uint32_t len;
    uint32_t prot;
    uint32_t flags;
    uint32_t fd;
    uint32_t offset;
  };
  REGISTER_SYSCALL_IMPL_X32(mmap, [](FEXCore::Core::CpuStateFrame* Frame, const old_mmap_struct* arg) -> uint64_t {
    return reinterpret_cast<uint64_t>(FEX::HLE::_SyscallHandler->GuestMmap(false, Frame->Thread, reinterpret_cast<void*>(arg->addr), arg->len, arg->prot, arg->flags, arg->fd, arg->offset));
  });

  REGISTER_SYSCALL_IMPL_X32(
    mmap2, [](FEXCore::Core::CpuStateFrame* Frame, uint32_t addr, uint32_t length, int prot, int flags, int fd, uint32_t pgoffset) -> uint64_t {
      return reinterpret_cast<uint64_t>(FEX::HLE::_SyscallHandler->GuestMmap(false, Frame->Thread, reinterpret_cast<void*>(addr), length, prot, flags, fd, (uint64_t)pgoffset * 0x1000));
    });

  REGISTER_SYSCALL_IMPL_X32(munmap, [](FEXCore::Core::CpuStateFrame* Frame, void* addr, size_t length) -> uint64_t {
    return FEX::HLE::_SyscallHandler->GuestMunmap(Frame->Thread, addr, length);
  });

  REGISTER_SYSCALL_IMPL_X32(mprotect, [](FEXCore::Core::CpuStateFrame* Frame, void* addr, uint32_t len, int prot) -> uint64_t {
    auto Thread = Frame->Thread;
    uint64_t Result {};

    {
      auto lk = FEXCore::GuardSignalDeferringSection(FEX::HLE::_SyscallHandler->VMATracking.Mutex, Thread);
      Result = ::mprotect(addr, len, prot);
      if (Result == -1) {
        SYSCALL_ERRNO();
      }

      FEX::HLE::_SyscallHandler->TrackMprotect(Thread, addr, len, prot);
    }


    FEX::HLE::_SyscallHandler->InvalidateCodeRangeIfNecessary(Thread, reinterpret_cast<uint64_t>(addr), len);

    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(
    mremap, [](FEXCore::Core::CpuStateFrame* Frame, void* old_address, size_t old_size, size_t new_size, int flags, void* new_address) -> uint64_t {
      auto Thread = Frame->Thread;
      uint64_t Result {};

      {
        auto lk = FEXCore::GuardSignalDeferringSection(FEX::HLE::_SyscallHandler->VMATracking.Mutex, Thread);
        Result = reinterpret_cast<uint64_t>(FEX::HLE::_SyscallHandler->Get32BitAllocator()->Mremap(old_address, old_size, new_size, flags, new_address));

        if (FEX::HLE::HasSyscallError(Result)) {
          return Result;
        }

        FEX::HLE::_SyscallHandler->TrackMremap(Thread, reinterpret_cast<uint64_t>(old_address), old_size, new_size, flags, Result);
      }

      FEX::HLE::_SyscallHandler->InvalidateCodeRangeIfNecessaryOnRemap(Thread, reinterpret_cast<uint64_t>(old_address), Result, old_size, new_size);

      return Result;
    });

  REGISTER_SYSCALL_IMPL_X32(mlockall, [](FEXCore::Core::CpuStateFrame* Frame, int flags) -> uint64_t {
    uint64_t Result = ::syscall(SYSCALL_DEF(mlock2), reinterpret_cast<void*>(0x1'0000), 0x1'0000'0000ULL - 0x1'0000, flags);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(munlockall, [](FEXCore::Core::CpuStateFrame* Frame) -> uint64_t {
    uint64_t Result = ::munlock(reinterpret_cast<void*>(0x1'0000), 0x1'0000'0000ULL - 0x1'0000);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(shmat, [](FEXCore::Core::CpuStateFrame* Frame, int shmid, const void* shmaddr, int shmflg) -> uint64_t {
    // also implemented in ipc:OP_SHMAT
    auto Thread = Frame->Thread;
    auto CTX = Thread->CTX;
    uint64_t Result {};
    uint32_t ResultAddr {};
    uint64_t Length {};
    CTX->MarkMemoryShared(Thread);

    {
      auto lk = FEXCore::GuardSignalDeferringSection(FEX::HLE::_SyscallHandler->VMATracking.Mutex, Thread);

      Result = FEX::HLE::_SyscallHandler->Get32BitAllocator()->Shmat(shmid, reinterpret_cast<const void*>(shmaddr), shmflg, &ResultAddr);

      if (FEX::HLE::HasSyscallError(Result)) {
        return Result;
      }

      shmid_ds stat;

      [[maybe_unused]] auto res = shmctl(shmid, IPC_STAT, &stat);
      LOGMAN_THROW_A_FMT(res != -1, "shmctl IPC_STAT failed");

      Length = stat.shm_segsz;
      FEX::HLE::_SyscallHandler->TrackShmat(Thread, shmid, ResultAddr, shmflg, Length);
    }

    FEX::HLE::_SyscallHandler->InvalidateCodeRangeIfNecessary(Thread, ResultAddr, Length);
    return ResultAddr;
  });

  REGISTER_SYSCALL_IMPL_X32(shmdt, [](FEXCore::Core::CpuStateFrame* Frame, const void* shmaddr) -> uint64_t {
    // also implemented in ipc:OP_SHMDT
    auto Thread = Frame->Thread;
    uint64_t Result {};
    uint64_t Length {};
    {
      auto lk = FEXCore::GuardSignalDeferringSection(FEX::HLE::_SyscallHandler->VMATracking.Mutex, Thread);
      Result = FEX::HLE::_SyscallHandler->Get32BitAllocator()->Shmdt(shmaddr);

      if (FEX::HLE::HasSyscallError(Result)) {
        return Result;
      }

      Length = FEX::HLE::_SyscallHandler->TrackShmdt(Thread, reinterpret_cast<uintptr_t>(shmaddr));
    }

    FEX::HLE::_SyscallHandler->InvalidateCodeRangeIfNecessary(Thread, reinterpret_cast<uintptr_t>(shmaddr), Length);
    return Result;
  });
}

} // namespace FEX::HLE::x32
