/*
$info$
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#include "Tests/LinuxSyscalls/LinuxAllocator.h"
#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include <FEXCore/Core/Context.h>
#include <FEXCore/Debug/InternalThreadState.h>

#include <FEXCore/IR/IR.h>

#include <sys/mman.h>
#include <sys/shm.h>
#include <map>
#include <unistd.h>

#include <FEXCore/Core/Context.h>
#include <FEXCore/Config/Config.h>
#include <FEXCore/Utils/Allocator.h>
#include <fstream>
#include <filesystem>

namespace FEX::HLE::x64 {

  void *x64SyscallHandler::GuestMmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    uint64_t Result{};

    bool Map32Bit = flags & FEX::HLE::X86_64_MAP_32BIT;
    if (Map32Bit) {
      Result = (uint64_t)Get32BitAllocator()->mmap(addr, length, prot,flags, fd, offset);
      if (FEX::HLE::HasSyscallError(Result)) {
        errno = -Result;
        Result = -1;
      }
    } else {
      Result = reinterpret_cast<uint64_t>(FEXCore::Allocator::mmap(reinterpret_cast<void*>(addr), length, prot, flags, fd, offset));
    }

    if (Result != -1) {
      FEX::HLE::_SyscallHandler->TrackMmap((uintptr_t)Result, length, prot, flags, fd, offset);
    }

    return reinterpret_cast<void*>(Result);
  }

  int x64SyscallHandler::GuestMunmap(void *addr, uint64_t length) {
    uint64_t Result{};
    if (reinterpret_cast<uintptr_t>(addr) < 0x1'0000'0000ULL) {
      Result = Get32BitAllocator()->munmap(addr, length);

      if (FEX::HLE::HasSyscallError(Result)) {
        errno = -Result;
        Result = -1;
      }
    } else {
      Result = FEXCore::Allocator::munmap(addr, length);
    }

    if (Result != -1) {
      FEX::HLE::_SyscallHandler->TrackMunmap(reinterpret_cast<uintptr_t>(addr), length);
    }

    return Result;
  }

  void RegisterMemory(FEX::HLE::SyscallHandler *const Handler) {
    using namespace FEXCore::IR;

    REGISTER_SYSCALL_IMPL_X64_FLAGS(mmap, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, void *addr, size_t length, int prot, int flags, int fd, off_t offset) -> uint64_t {
        uint64_t Result = (uint64_t) static_cast<FEX::HLE::x64::x64SyscallHandler*>(FEX::HLE::_SyscallHandler)->
          GuestMmap(addr, length, prot, flags, fd, offset);

        SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_FLAGS(munmap, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, void *addr, size_t length) -> uint64_t {
        uint64_t Result = static_cast<FEX::HLE::x64::x64SyscallHandler*>(FEX::HLE::_SyscallHandler)->
          GuestMunmap(addr, length);

      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_FLAGS(mremap, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, void *old_address, size_t old_size, size_t new_size, int flags, void *new_address) -> uint64_t {
      uint64_t Result = reinterpret_cast<uint64_t>(::mremap(old_address, old_size, new_size, flags, new_address));

      if (Result != -1) {
        FEX::HLE::_SyscallHandler->TrackMremap((uintptr_t)old_address, old_size, new_size, flags, Result);
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_FLAGS(mprotect, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, void *addr, size_t len, int prot) -> uint64_t {
      uint64_t Result = ::mprotect(addr, len, prot);

      if (Result != -1) {
        FEX::HLE::_SyscallHandler->TrackMprotect((uintptr_t)addr, len, prot);
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(mlockall, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int flags) -> uint64_t {
      uint64_t Result = ::mlockall(flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(munlockall, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      uint64_t Result = ::munlockall();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_FLAGS(shmat, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int shmid, const void *shmaddr, int shmflg) -> uint64_t {
      uint64_t Result = reinterpret_cast<uint64_t>(shmat(shmid, shmaddr, shmflg));

      if (Result != -1) {
        FEX::HLE::_SyscallHandler->TrackShmat(shmid, Result, shmflg);
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_FLAGS(shmdt, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, const void *shmaddr) -> uint64_t {
      uint64_t Result = ::shmdt(shmaddr);

      if (Result != -1) {
        FEX::HLE::_SyscallHandler->TrackShmdt((uintptr_t)shmaddr);
      }
      SYSCALL_ERRNO();
    });

    if (Handler->IsHostKernelVersionAtLeast(5, 10, 0)) {
      REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(process_madvise, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
        [](FEXCore::Core::CpuStateFrame *Frame, int pidfd, const struct iovec *iovec, size_t vlen, int advice, unsigned int flags) -> uint64_t {
        uint64_t Result = ::syscall(SYSCALL_DEF(process_madvise), pidfd, iovec, vlen, advice, flags);
        SYSCALL_ERRNO();
      });
    }
    else {
      REGISTER_SYSCALL_IMPL_X64(process_madvise, UnimplementedSyscallSafe);
    }
  }
}
