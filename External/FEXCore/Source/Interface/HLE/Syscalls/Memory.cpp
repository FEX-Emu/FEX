#include "Interface/HLE/Syscalls.h"
#include "Interface/Context/Context.h"
#include "Interface/Core/InternalThreadState.h"

#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

//#define MEM_PASSTHROUGH
namespace FEXCore::HLE {
  uint64_t Mmap(FEXCore::Core::InternalThreadState *Thread, void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
#ifdef MEM_PASSTHROUGH
    uint64_t Result = reinterpret_cast<uint64_t>(::mmap(addr, length, prot, flags, fd, offset));
    SYSCALL_ERRNO();
#else
    return Thread->CTX->SyscallHandler->HandleMMAP(Thread, addr, length, prot, flags, fd, offset);
#endif
  }

  uint64_t Mprotect(FEXCore::Core::InternalThreadState *Thread, void *addr, size_t len, int prot) {
    uint64_t Result = ::mprotect(addr, len, prot);
    SYSCALL_ERRNO();
  }

  uint64_t Munmap(FEXCore::Core::InternalThreadState *Thread, void *addr, size_t length) {
    uint64_t Result = ::munmap(addr, length);
    SYSCALL_ERRNO();
  }

  uint64_t Brk(FEXCore::Core::InternalThreadState *Thread, void *addr) {
    uint64_t Result = Thread->CTX->SyscallHandler->HandleBRK(Thread, addr);
    SYSCALL_ERRNO();
  }

  uint64_t Mremap(FEXCore::Core::InternalThreadState *Thread, void *old_address, size_t old_size, size_t new_size, int flags, void *new_address) {
    uint64_t Result = reinterpret_cast<uint64_t>(::mremap(old_address, old_size, new_size, flags, new_address));
    SYSCALL_ERRNO();
  }

  uint64_t Msync(FEXCore::Core::InternalThreadState *Thread, void *addr, size_t length, int32_t flags) {
    uint64_t Result = ::msync(addr, length, flags);
    SYSCALL_ERRNO();
  }

  uint64_t Mincore(FEXCore::Core::InternalThreadState *Thread, void *addr, size_t length, uint8_t *vec) {
    uint64_t Result = ::mincore(addr, length, vec);
    SYSCALL_ERRNO();
  }

  uint64_t Madvise(FEXCore::Core::InternalThreadState *Thread, void *addr, size_t length, int32_t advice) {
    uint64_t Result = ::madvise(addr, length, advice);
    SYSCALL_ERRNO();
  }
}
