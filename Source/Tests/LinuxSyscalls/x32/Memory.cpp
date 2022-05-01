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

static std::string get_fdpath(int fd)
{
  std::error_code ec;
  return std::filesystem::canonical(std::filesystem::path("/proc/self/fd") / std::to_string(fd), ec).string();
}

namespace FEX::HLE::x32 {
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
      auto Result = (uint64_t)static_cast<FEX::HLE::x32::x32SyscallHandler*>(FEX::HLE::_SyscallHandler)->GetAllocator()->
        mmap(reinterpret_cast<void*>(arg->addr), arg->len, arg->prot, arg->flags, arg->fd, arg->offset);

      if (!FEX::HLE::HasSyscallError(Result)) {
        auto CTX = Frame->Thread->CTX;
        if (!(arg->flags & MAP_ANONYMOUS)) {
          auto filename = get_fdpath(arg->fd);

          FEXCore::Context::AddNamedRegion(CTX, Result, arg->len, arg->offset, filename);
        }
        FEXCore::Context::FlushCodeRange(CTX, (uintptr_t)Result, arg->len);
      }
      return Result;
    });

    REGISTER_SYSCALL_IMPL_X32(mmap2, [](FEXCore::Core::CpuStateFrame *Frame, uint32_t addr, uint32_t length, int prot, int flags, int fd, uint32_t pgoffset) -> uint64_t {
      auto Result = (uint64_t)static_cast<FEX::HLE::x32::x32SyscallHandler*>(FEX::HLE::_SyscallHandler)->GetAllocator()->
        mmap(reinterpret_cast<void*>(addr), length, prot,flags, fd, (uint64_t)pgoffset * 0x1000);

      if (!FEX::HLE::HasSyscallError(Result)) {
        auto CTX = Frame->Thread->CTX;
        if (!(flags & MAP_ANONYMOUS)) {
          auto filename = get_fdpath(fd);

          FEXCore::Context::AddNamedRegion(CTX, Result, length, pgoffset * 0x1000, filename);
        }
        FEXCore::Context::FlushCodeRange(CTX, (uintptr_t)Result, length);
      }

      return Result;
    });

    REGISTER_SYSCALL_IMPL_X32(munmap, [](FEXCore::Core::CpuStateFrame *Frame, void *addr, size_t length) -> uint64_t {
      auto Result = static_cast<FEX::HLE::x32::x32SyscallHandler*>(FEX::HLE::_SyscallHandler)->GetAllocator()->
        munmap(addr, length);

      if (Result == 0) {
        auto CTX = Frame->Thread->CTX;
        FEXCore::Context::RemoveNamedRegion(CTX, (uintptr_t)addr, length);
        FEXCore::Context::FlushCodeRange(CTX, (uintptr_t)addr, length);
      }

      return Result;
    });

    REGISTER_SYSCALL_IMPL_X32(mprotect, [](FEXCore::Core::CpuStateFrame *Frame, void *addr, uint32_t len, int prot) -> uint64_t {
      uint64_t Result = ::mprotect(addr, len, prot);

      if (Result != -1 && prot & PROT_EXEC) {
        auto CTX = Frame->Thread->CTX;
        FEXCore::Context::FlushCodeRange(CTX, (uintptr_t)addr, len);
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(mremap, [](FEXCore::Core::CpuStateFrame *Frame, void *old_address, size_t old_size, size_t new_size, int flags, void *new_address) -> uint64_t {
      return reinterpret_cast<uint64_t>(static_cast<FEX::HLE::x32::x32SyscallHandler*>(FEX::HLE::_SyscallHandler)->GetAllocator()->
        mremap(old_address, old_size, new_size, flags, new_address));
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
      if (Result == 0) {
        return ResultAddr;
      }
      else {
        return Result;
      }
    });

    REGISTER_SYSCALL_IMPL_X32(shmdt, [](FEXCore::Core::CpuStateFrame *Frame, const void *shmaddr) -> uint64_t {
      uint64_t Result = static_cast<FEX::HLE::x32::x32SyscallHandler*>(FEX::HLE::_SyscallHandler)->GetAllocator()->
        shmdt(shmaddr);
      SYSCALL_ERRNO();
    });
  }

}
