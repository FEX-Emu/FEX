#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"

#include <bitset>
#include <map>
#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/ipc.h>

namespace FEX::HLE::x32 {

  void RegisterMemory() {
    REGISTER_SYSCALL_IMPL_X32(mmap, [](FEXCore::Core::InternalThreadState *Thread, uint32_t addr, uint32_t length, int prot, int flags, int fd, int32_t offset) -> uint64_t {
      return (uint64_t)static_cast<FEX::HLE::x32::x32SyscallHandler*>(FEX::HLE::_SyscallHandler)->GetAllocator()->
        mmap(reinterpret_cast<void*>(addr), length, prot,flags, fd, offset);
    });

    REGISTER_SYSCALL_IMPL_X32(mmap2, [](FEXCore::Core::InternalThreadState *Thread, uint32_t addr, uint32_t length, int prot, int flags, int fd, uint32_t pgoffset) -> uint64_t {
      return (uint64_t)static_cast<FEX::HLE::x32::x32SyscallHandler*>(FEX::HLE::_SyscallHandler)->GetAllocator()->
        mmap(reinterpret_cast<void*>(addr), length, prot,flags, fd, (uint64_t)pgoffset * 0x1000);
    });

    REGISTER_SYSCALL_IMPL_X32(munmap, [](FEXCore::Core::InternalThreadState *Thread, void *addr, size_t length) -> uint64_t {
      return static_cast<FEX::HLE::x32::x32SyscallHandler*>(FEX::HLE::_SyscallHandler)->GetAllocator()->
        munmap(addr, length);
    });

    REGISTER_SYSCALL_IMPL_X32(mprotect, [](FEXCore::Core::InternalThreadState *Thread, void *addr, uint32_t len, int prot) -> uint64_t {
      uint64_t Result = ::mprotect(addr, len, prot);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(mremap, [](FEXCore::Core::InternalThreadState *Thread, void *old_address, size_t old_size, size_t new_size, int flags, void *new_address) -> uint64_t {
      return reinterpret_cast<uint64_t>(static_cast<FEX::HLE::x32::x32SyscallHandler*>(FEX::HLE::_SyscallHandler)->GetAllocator()->
        mremap(old_address, old_size, new_size, flags, new_address));
    });

    REGISTER_SYSCALL_IMPL_X32(mlockall, [](FEXCore::Core::InternalThreadState *Thread, int flags) -> uint64_t {
      uint64_t Result = ::mlock2(reinterpret_cast<void*>(0x1'0000), 0x1'0000'0000ULL - 0x1'0000, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(munlockall, [](FEXCore::Core::InternalThreadState *Thread) -> uint64_t {
      uint64_t Result = ::munlock(reinterpret_cast<void*>(0x1'0000), 0x1'0000'0000ULL - 0x1'0000);
      SYSCALL_ERRNO();
    });
  }

}
