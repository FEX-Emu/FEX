#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"
#include "Interface/Context/Context.h"

#include <sys/mman.h>

#define MEM_PASSTHROUGH
namespace FEXCore::HLE::x64 {
  void RegisterMemory() {
    REGISTER_SYSCALL_IMPL_X64(mmap, [](FEXCore::Core::InternalThreadState *Thread, void *addr, size_t length, int prot, int flags, int fd, off_t offset) -> uint64_t {
  #ifdef MEM_PASSTHROUGH
      uint64_t Result = reinterpret_cast<uint64_t>(::mmap(addr, length, prot, flags, fd, offset));
      SYSCALL_ERRNO();
  #else
      return Thread->CTX->SyscallHandler->HandleMMAP(Thread, addr, length, prot, flags, fd, offset);
  #endif
    });

    REGISTER_SYSCALL_IMPL_X64(mprotect, [](FEXCore::Core::InternalThreadState *Thread, void *addr, size_t len, int prot) -> uint64_t {
      uint64_t Result = ::mprotect(addr, len, prot);
      SYSCALL_ERRNO();
    });
  }
}
