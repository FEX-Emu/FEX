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
#include <fstream>

std::string get_fdpath(int fd)
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

namespace FEX::HLE::x64 {
  void RegisterMemory() {
    REGISTER_SYSCALL_IMPL_X64(munmap, [](FEXCore::Core::InternalThreadState *Thread, void *addr, size_t length) -> uint64_t {
      uint64_t Result = ::munmap(addr, length);
      if (Result != -1) {
        FEXCore::Context::RemoveNamedRegion(Thread->CTX, (uintptr_t)addr, length);
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(mmap, [](FEXCore::Core::InternalThreadState *Thread, void *addr, size_t length, int prot, int flags, int fd, off_t offset) -> uint64_t {
      static FEXCore::Config::Value<bool> AOTIRLoad(FEXCore::Config::CONFIG_AOTIR_LOAD, false);
      uint64_t Result = reinterpret_cast<uint64_t>(::mmap(addr, length, prot, flags, fd, offset));
      if (Result != -1 && !(flags & MAP_ANONYMOUS)) {
        auto filename = get_fdpath(fd);

        FEXCore::Context::AddNamedRegion(Thread->CTX, Result, length, offset, filename);
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(mremap, [](FEXCore::Core::InternalThreadState *Thread, void *old_address, size_t old_size, size_t new_size, int flags, void *new_address) -> uint64_t {
      uint64_t Result = reinterpret_cast<uint64_t>(::mremap(old_address, old_size, new_size, flags, new_address));
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(mprotect, [](FEXCore::Core::InternalThreadState *Thread, void *addr, size_t len, int prot) -> uint64_t {
      uint64_t Result = ::mprotect(addr, len, prot);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(mlockall, [](FEXCore::Core::InternalThreadState *Thread, int flags) -> uint64_t {
      uint64_t Result = ::mlockall(flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(munlockall, [](FEXCore::Core::InternalThreadState *Thread) -> uint64_t {
      uint64_t Result = ::munlockall();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(shmat, [](FEXCore::Core::InternalThreadState *Thread, int shmid, const void *shmaddr, int shmflg) -> uint64_t {
      uint64_t Result = reinterpret_cast<uint64_t>(shmat(shmid, shmaddr, shmflg));
      SYSCALL_ERRNO();
    });
  }
}
