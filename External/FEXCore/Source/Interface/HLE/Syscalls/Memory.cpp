#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"
#include "Interface/Context/Context.h"
#include "Interface/Core/InternalThreadState.h"

#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <numaif.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

#define MEM_PASSTHROUGH
namespace FEXCore::HLE {

  void RegisterMemory() {
    REGISTER_SYSCALL_IMPL(mmap, [](FEXCore::Core::InternalThreadState *Thread, void *addr, size_t length, int prot, int flags, int fd, off_t offset) -> uint64_t {
  #ifdef MEM_PASSTHROUGH
      uint64_t Result = reinterpret_cast<uint64_t>(::mmap(addr, length, prot, flags, fd, offset));
      SYSCALL_ERRNO();
  #else
      return Thread->CTX->SyscallHandler->HandleMMAP(Thread, addr, length, prot, flags, fd, offset);
  #endif
    });

    REGISTER_SYSCALL_IMPL(mprotect, [](FEXCore::Core::InternalThreadState *Thread, void *addr, size_t len, int prot) -> uint64_t {
      uint64_t Result = ::mprotect(addr, len, prot);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(munmap, [](FEXCore::Core::InternalThreadState *Thread, void *addr, size_t length) -> uint64_t {
      uint64_t Result = ::munmap(addr, length);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(brk, [](FEXCore::Core::InternalThreadState *Thread, void *addr) -> uint64_t {
      uint64_t Result = Thread->CTX->SyscallHandler->HandleBRK(Thread, addr);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(mremap, [](FEXCore::Core::InternalThreadState *Thread, void *old_address, size_t old_size, size_t new_size, int flags, void *new_address) -> uint64_t {
      uint64_t Result = reinterpret_cast<uint64_t>(::mremap(old_address, old_size, new_size, flags, new_address));
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(msync, [](FEXCore::Core::InternalThreadState *Thread, void *addr, size_t length, int32_t flags) -> uint64_t {
      uint64_t Result = ::msync(addr, length, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(mincore, [](FEXCore::Core::InternalThreadState *Thread, void *addr, size_t length, uint8_t *vec) -> uint64_t {
      uint64_t Result = ::mincore(addr, length, vec);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(madvise, [](FEXCore::Core::InternalThreadState *Thread, void *addr, size_t length, int32_t advice) -> uint64_t {
      uint64_t Result = ::madvise(addr, length, advice);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(mlock, [](FEXCore::Core::InternalThreadState *Thread, const void *addr, size_t len) -> uint64_t {
      uint64_t Result = ::mlock(addr, len);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(munlock, [](FEXCore::Core::InternalThreadState *Thread, const void *addr, size_t len) -> uint64_t {
      uint64_t Result = ::munlock(addr, len);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(mlockall, [](FEXCore::Core::InternalThreadState *Thread, int flags) -> uint64_t {
      uint64_t Result = ::mlockall(flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(munlockall, [](FEXCore::Core::InternalThreadState *Thread) -> uint64_t {
      uint64_t Result = ::munlockall();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(mlock2, [](FEXCore::Core::InternalThreadState *Thread, const void *addr, size_t len, int flags) -> uint64_t {
      uint64_t Result = ::mlock2(addr, len, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(get_mempolicy, [](FEXCore::Core::InternalThreadState *Thread, int *mode, unsigned long *nodemask, unsigned long maxnode, void *addr, unsigned long flags) -> uint64_t {
      uint64_t Result = ::get_mempolicy(mode, nodemask, maxnode, addr, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(set_mempolicy, [](FEXCore::Core::InternalThreadState *Thread, int mode, const unsigned long *nodemask, unsigned long maxnode) -> uint64_t {
      uint64_t Result = ::set_mempolicy(mode, nodemask, maxnode);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(membarrier, [](FEXCore::Core::InternalThreadState *Thread, int cmd, int flags) -> uint64_t {
        uint64_t Result = syscall(SYS_membarrier, cmd, flags);
        SYSCALL_ERRNO();
    });
  }
}
