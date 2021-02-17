#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"
#include <FEXCore/Debug/InternalThreadState.h>

#include <bitset>
#include <map>
#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <unistd.h>

static std::string get_fdpath(int fd)
{
    std::vector<char> buf(400);
    ssize_t len;

    std::string fdToName = "/proc/self/fd/" + std::to_string(fd);

    do
    {
        buf.resize(buf.size() + 100);
        len = ::readlink(fdToName.c_str(), &(buf[0]), buf.size());
    } while (buf.size() == len);

    if (len > 0)
    {
        buf[len] = '\0';
        return (std::string(&(buf[0])));
    }
    /* handle error */
    return "";
}

namespace FEX::HLE::x32 {

  void RegisterMemory() {
    REGISTER_SYSCALL_IMPL_X32(mmap, [](FEXCore::Core::InternalThreadState *Thread, uint32_t addr, uint32_t length, int prot, int flags, int fd, int32_t offset) -> uint64_t {
      auto Result = (uint64_t)static_cast<FEX::HLE::x32::x32SyscallHandler*>(FEX::HLE::_SyscallHandler)->GetAllocator()->
        mmap(reinterpret_cast<void*>(addr), length, prot,flags, fd, offset);

      if (Result != -1 && !(flags & MAP_ANONYMOUS)) {
        auto filename = get_fdpath(fd);

        FEXCore::Context::AddNamedRegion(Thread->CTX, Result, length, offset, filename);
      }

      return Result;
    });

    REGISTER_SYSCALL_IMPL_X32(mmap2, [](FEXCore::Core::InternalThreadState *Thread, uint32_t addr, uint32_t length, int prot, int flags, int fd, uint32_t pgoffset) -> uint64_t {
      auto Result = (uint64_t)static_cast<FEX::HLE::x32::x32SyscallHandler*>(FEX::HLE::_SyscallHandler)->GetAllocator()->
        mmap(reinterpret_cast<void*>(addr), length, prot,flags, fd, (uint64_t)pgoffset * 0x1000);
      
      if (Result != -1 && !(flags & MAP_ANONYMOUS)) {
        auto filename = get_fdpath(fd);

        FEXCore::Context::AddNamedRegion(Thread->CTX, Result, length, pgoffset * 0x1000, filename);
      }

      return Result;
    });

    REGISTER_SYSCALL_IMPL_X32(munmap, [](FEXCore::Core::InternalThreadState *Thread, void *addr, size_t length) -> uint64_t {
      auto Result = static_cast<FEX::HLE::x32::x32SyscallHandler*>(FEX::HLE::_SyscallHandler)->GetAllocator()->
        munmap(addr, length);
      if (Result != -1) {
        FEXCore::Context::RemoveNamedRegion(Thread->CTX, (uintptr_t)addr, length);
      }
      return Result;
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
