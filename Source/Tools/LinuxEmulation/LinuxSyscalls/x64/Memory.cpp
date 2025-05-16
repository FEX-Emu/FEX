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

void* x64SyscallHandler::GuestMmap(FEXCore::Core::InternalThreadState* Thread, void* addr, size_t length, int prot, int flags, int fd, off_t offset) {
  uint64_t Result {};
  size_t Size = FEXCore::AlignUp(length, FEXCore::Utils::FEX_PAGE_SIZE);

  if (flags & MAP_SHARED) {
    CTX->MarkMemoryShared(Thread);
  }

  {
    // NOTE: Frontend calls this with a nullptr Thread during initialization, but
    //       providing this code with a valid Thread object earlier would allow
    //       us to be more optimal by using GuardSignalDeferringSection instead
    auto lk = FEXCore::GuardSignalDeferringSectionWithFallback(VMATracking.Mutex, Thread);

    bool Map32Bit = flags & FEX::HLE::X86_64_MAP_32BIT;
    if (Map32Bit) {
      Result = (uint64_t)Get32BitAllocator()->Mmap(addr, length, prot, flags, fd, offset);
      if (FEX::HLE::HasSyscallError(Result)) {
        return reinterpret_cast<void*>(Result);
      }
    } else {
      Result = reinterpret_cast<uint64_t>(::mmap(reinterpret_cast<void*>(addr), length, prot, flags, fd, offset));
      if (Result == ~0ULL) {
        return reinterpret_cast<void*>(-errno);
      }
    }

    FEX::HLE::_SyscallHandler->TrackMmap(Thread, Result, length, prot, flags, fd, offset);
  }

  FEX::HLE::_SyscallHandler->InvalidateCodeRangeIfNecessary(Thread, Result, Size);
  return reinterpret_cast<void*>(Result);
}

uint64_t x64SyscallHandler::GuestMunmap(FEXCore::Core::InternalThreadState* Thread, void* addr, uint64_t length) {
  uint64_t Result {};
  uint64_t Size = FEXCore::AlignUp(length, FEXCore::Utils::FEX_PAGE_SIZE);

  {
    // Frontend calls this with nullptr Thread during initialization.
    // This is why `GuardSignalDeferringSectionWithFallback` is used here.
    // To be more optimal the frontend should provide this code with a valid Thread object earlier.
    auto lk = FEXCore::GuardSignalDeferringSectionWithFallback(VMATracking.Mutex, Thread);

    if (reinterpret_cast<uintptr_t>(addr) < 0x1'0000'0000ULL) {
      Result = FEX::HLE::_SyscallHandler->Get32BitAllocator()->Munmap(addr, length);
      if (FEX::HLE::HasSyscallError(Result)) {
        return Result;
      }
    } else {
      Result = ::munmap(addr, length);
      if (Result == -1) {
        return -errno;
      }
    }
    FEX::HLE::_SyscallHandler->TrackMunmap(Thread, addr, length);
  }
  FEX::HLE::_SyscallHandler->InvalidateCodeRangeIfNecessary(Thread, reinterpret_cast<uint64_t>(addr), Size);

  return Result;
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

  REGISTER_SYSCALL_IMPL_X64_FLAGS(shmat, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                  ([](FEXCore::Core::CpuStateFrame* Frame, int shmid, const void* shmaddr, int shmflg) -> uint64_t {
                                    auto Thread = Frame->Thread;
                                    auto CTX = Thread->CTX;
                                    uint64_t Result {};
                                    uint64_t Length {};
                                    CTX->MarkMemoryShared(Thread);

                                    {
                                      auto lk = FEXCore::GuardSignalDeferringSection(FEX::HLE::_SyscallHandler->VMATracking.Mutex, Thread);
                                      Result = reinterpret_cast<uint64_t>(::shmat(shmid, shmaddr, shmflg));

                                      if (Result == -1) {
                                        SYSCALL_ERRNO();
                                      }

                                      shmid_ds stat;

                                      [[maybe_unused]] auto res = shmctl(shmid, IPC_STAT, &stat);
                                      LOGMAN_THROW_A_FMT(res != -1, "shmctl IPC_STAT failed");

                                      Length = stat.shm_segsz;
                                      FEX::HLE::_SyscallHandler->TrackShmat(Thread, shmid, Result, shmflg, Length);
                                    }

                                    FEX::HLE::_SyscallHandler->InvalidateCodeRangeIfNecessary(Thread, Result, Length);
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
