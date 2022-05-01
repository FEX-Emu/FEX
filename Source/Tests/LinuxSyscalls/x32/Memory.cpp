/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Debug/InternalThreadState.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <system_error>
#include <filesystem>

namespace FEX::HLE::x32 {

  void *x32SyscallHandler::GuestMmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    LOGMAN_THROW_A_FMT((length >> 32) == 0, "values must fit to 32 bits");

    auto Result = (uint64_t)GetAllocator()->mmap((void*)addr, length, prot, flags, fd, offset);

    LOGMAN_THROW_A_FMT((Result >> 32) == 0|| (Result >> 32) == 0xFFFFFFFF, "values must fit to 32 bits");

    if (!FEX::HLE::HasSyscallError(Result)) {
      FEX::HLE::_SyscallHandler->TrackMmap(Result, length, prot, flags, fd, offset);
      return (void *)Result;
    } else {
      errno = -Result;
      return MAP_FAILED;
    }
  }

  int x32SyscallHandler::GuestMunmap(void *addr, uint64_t length) {
    LOGMAN_THROW_A_FMT((uintptr_t(addr) >> 32) == 0, "values must fit to 32 bits");
    LOGMAN_THROW_A_FMT((length >> 32) == 0, "values must fit to 32 bits");

    auto Result = GetAllocator()->munmap(addr, length);

    if (Result == 0) {
      FEX::HLE::_SyscallHandler->TrackMunmap((uintptr_t)addr, length);
      return Result;
    } else {
      errno = -Result;
      return -1;
    }
  }

  void RegisterMemory() {
    struct old_mmap_struct {
      uint32_t addr;
      uint32_t len;
      uint32_t prot;
      uint32_t flags;
      uint32_t fd;
      uint32_t offset;
    };
    REGISTER_SYSCALL_IMPL_X32(mmap, [](FEXCore::Core::CpuStateFrame *Frame, old_mmap_struct const* arg) -> uint64_t {
      uint64_t Result = (uint64_t)static_cast<FEX::HLE::x32::x32SyscallHandler*>(FEX::HLE::_SyscallHandler)->
        GuestMmap(reinterpret_cast<void*>(arg->addr), arg->len, arg->prot, arg->flags, arg->fd, arg->offset);

        SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(mmap2, [](FEXCore::Core::CpuStateFrame *Frame, uint32_t addr, uint32_t length, int prot, int flags, int fd, uint32_t pgoffset) -> uint64_t {
      uint64_t Result = (uint64_t)static_cast<FEX::HLE::x32::x32SyscallHandler*>(FEX::HLE::_SyscallHandler)->
        GuestMmap(reinterpret_cast<void*>(addr), length, prot,flags, fd, (uint64_t)pgoffset * 0x1000);

        SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(munmap, [](FEXCore::Core::CpuStateFrame *Frame, void *addr, size_t length) -> uint64_t {
      uint64_t Result = (uint64_t)static_cast<FEX::HLE::x32::x32SyscallHandler*>(FEX::HLE::_SyscallHandler)->
        GuestMunmap(addr, length);

        SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(mprotect, [](FEXCore::Core::CpuStateFrame *Frame, void *addr, uint32_t len, int prot) -> uint64_t {
      uint64_t Result = ::mprotect(addr, len, prot);
      if (Result != -1) {
        FEX::HLE::_SyscallHandler->TrackMprotect((uintptr_t)addr, len, prot);
      }

      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(mremap, [](FEXCore::Core::CpuStateFrame *Frame, void *old_address, size_t old_size, size_t new_size, int flags, void *new_address) -> uint64_t {
      uint64_t Result = reinterpret_cast<uint64_t>(static_cast<FEX::HLE::x32::x32SyscallHandler*>(FEX::HLE::_SyscallHandler)->GetAllocator()->
        mremap(old_address, old_size, new_size, flags, new_address));

      if (!FEX::HLE::HasSyscallError(Result)) {
        FEX::HLE::_SyscallHandler->TrackMremap((uintptr_t)old_address, old_size, new_size, flags, Result);
      }

      return Result;
    });

    REGISTER_SYSCALL_IMPL_X32(mlockall, [](FEXCore::Core::CpuStateFrame *Frame, int flags) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(mlock2), reinterpret_cast<void*>(0x1'0000), 0x1'0000'0000ULL - 0x1'0000, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(munlockall, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      uint64_t Result = ::munlock(reinterpret_cast<void*>(0x1'0000), 0x1'0000'0000ULL - 0x1'0000);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(shmat, [](FEXCore::Core::CpuStateFrame *Frame, int shmid, const void *shmaddr, int shmflg) -> uint64_t {
      uint32_t ResultAddr{};
      uint64_t Result = static_cast<FEX::HLE::x32::x32SyscallHandler*>(FEX::HLE::_SyscallHandler)->GetAllocator()->
          shmat(shmid, reinterpret_cast<const void*>(shmaddr), shmflg, &ResultAddr);

      if (!FEX::HLE::HasSyscallError(Result)) {
        FEX::HLE::_SyscallHandler->TrackShmat(shmid, ResultAddr, shmflg);
        return ResultAddr;
      }
      else {
        return Result;
      }
    });

    REGISTER_SYSCALL_IMPL_X32(shmdt, [](FEXCore::Core::CpuStateFrame *Frame, const void *shmaddr) -> uint64_t {
      uint64_t Result = static_cast<FEX::HLE::x32::x32SyscallHandler*>(FEX::HLE::_SyscallHandler)->GetAllocator()->
        shmdt(shmaddr);
      
      if (!FEX::HLE::HasSyscallError(Result)) {
        FEX::HLE::_SyscallHandler->TrackShmdt((uintptr_t)shmaddr);
      }

      return Result;
    });
  }

}
