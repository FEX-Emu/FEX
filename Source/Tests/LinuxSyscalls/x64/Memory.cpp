/*
$info$
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include <FEXCore/Core/Context.h>
#include <FEXCore/Debug/InternalThreadState.h>

#include <sys/mman.h>
#include <sys/shm.h>
#include <map>
#include <unistd.h>

#include <FEXCore/Core/Context.h>
#include <FEXCore/Config/Config.h>
#include <FEXCore/Utils/Allocator.h>
#include <fstream>
#include <filesystem>

static std::string get_fdpath(int fd)
{
  std::error_code ec;
  return std::filesystem::canonical(std::filesystem::path("/proc/self/fd") / std::to_string(fd), ec).string();
}

namespace FEX::HLE::x64 {
  void RegisterMemory(FEX::HLE::SyscallHandler *const Handler) {
    REGISTER_SYSCALL_IMPL_X64(munmap, [](FEXCore::Core::CpuStateFrame *Frame, void *addr, size_t length) -> uint64_t {
      uint64_t Result = FEXCore::Allocator::munmap(addr, length);

      auto Thread = Frame->Thread;
      if (Result != -1) {
        FEXCore::Context::RemoveNamedRegion(Thread->CTX, (uintptr_t)addr, length);
        FEXCore::Context::FlushCodeRange(Thread, (uintptr_t)addr, length);
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(mmap, [](FEXCore::Core::CpuStateFrame *Frame, void *addr, size_t length, int prot, int flags, int fd, off_t offset) -> uint64_t {
      static FEX_CONFIG_OPT(AOTIRLoad, AOTIRLOAD);

#ifdef _M_ARM_64
      constexpr uint32_t X86_64_MAP_32BIT = 0x40;

      // Handle MAP_32BIT without a full allocator
      // Set address to a low address to force the kernel to allocate bottom up
      bool Map32Bit = (flags & X86_64_MAP_32BIT) &&
          !(flags & (MAP_FIXED | MAP_FIXED_NOREPLACE));
      if (Map32Bit) {
        addr = reinterpret_cast<void*>(0x1'0000);
        // Remove the flag
        flags &= ~X86_64_MAP_32BIT;
      }
#endif
      uint64_t Result = reinterpret_cast<uint64_t>(FEXCore::Allocator::mmap(addr, length, prot, flags, fd, offset));

      auto Thread = Frame->Thread;
      if (Result != -1) {
#ifdef _M_ARM_64
        if (Map32Bit) {
          if ((Result + length) >= 0x1'0000'0000ULL) {
            FEXCore::Allocator::munmap(reinterpret_cast<void*>(Result), length);
            return -ENOMEM;
          }
        }
#endif
        if (!(flags & MAP_ANONYMOUS)) {
          auto filename = get_fdpath(fd);

          FEXCore::Context::AddNamedRegion(Thread->CTX, Result, length, offset, filename);
        }
        FEXCore::Context::FlushCodeRange(Thread, (uintptr_t)Result, length);
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(mremap, [](FEXCore::Core::CpuStateFrame *Frame, void *old_address, size_t old_size, size_t new_size, int flags, void *new_address) -> uint64_t {
      uint64_t Result = reinterpret_cast<uint64_t>(::mremap(old_address, old_size, new_size, flags, new_address));
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(mprotect, [](FEXCore::Core::CpuStateFrame *Frame, void *addr, size_t len, int prot) -> uint64_t {
      uint64_t Result = ::mprotect(addr, len, prot);

      auto Thread = Frame->Thread;
      if (Result != -1 && prot & PROT_EXEC) {
        FEXCore::Context::FlushCodeRange(Thread, (uintptr_t)addr, len);
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(mlockall, [](FEXCore::Core::CpuStateFrame *Frame, int flags) -> uint64_t {
      uint64_t Result = ::mlockall(flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(munlockall, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      uint64_t Result = ::munlockall();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(shmat, [](FEXCore::Core::CpuStateFrame *Frame, int shmid, const void *shmaddr, int shmflg) -> uint64_t {
      uint64_t Result = reinterpret_cast<uint64_t>(shmat(shmid, shmaddr, shmflg));
      SYSCALL_ERRNO();
    });

#ifndef SYS_process_madvise
#define SYS_process_madvise 440
#endif
    if (Handler->IsHostKernelVersionAtLeast(5, 10, 0)) {
      REGISTER_SYSCALL_IMPL_X64(process_madvise, [](FEXCore::Core::CpuStateFrame *Frame, int pidfd, const struct iovec *iovec, size_t vlen, int advice, unsigned int flags) -> uint64_t {
        uint64_t Result = ::syscall(SYS_process_madvise, pidfd, iovec, vlen, advice, flags);
        SYSCALL_ERRNO();
      });
    }
    else {
      REGISTER_SYSCALL_IMPL_X64(process_madvise, UnimplementedSyscallSafe);
    }
  }
}
