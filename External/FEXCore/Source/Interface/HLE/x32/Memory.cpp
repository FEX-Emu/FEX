#include "Interface/HLE/Syscalls.h"
#include "Interface/Context/Context.h"
#include "Interface/Core/InternalThreadState.h"

#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

#define MEM_PASSTHROUGH
namespace FEXCore::HLE::x32 {
  uint32_t Mmap(FEXCore::Core::InternalThreadState *Thread, void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
#ifdef MAP_32BIT
      uint64_t Result = reinterpret_cast<uint64_t>(::mmap(addr, length, prot, flags, fd, offset));
      SYSCALL_ERRNO();
#endif

#ifdef MEM_PASSTHROUGH
    if (!(flags & MAP_FIXED)) {
      uint64_t Result = reinterpret_cast<uint64_t>(::mmap(addr, length, prot, flags, fd, offset));
      SYSCALL_ERRNO();
    }
    else {
      // Okay, MAP_32BIT doesn't exist here
      // We need to spin through the lower 32bit and try and find a free address
      // In the future we should definitely use a bitmap allocator to understand where we can place pages

      // Skip the first page since we can never allocate that
      static uint64_t StartingPage = Core::PAGE_SIZE;
      constexpr uint64_t Max4GB = 0x1'0000'0000;
      flags |= MAP_FIXED_NOREPLACE;
      uint64_t Result{};
      for (; StartingPage < Max4GB; StartingPage += Core::PAGE_SIZE) {
        Result = reinterpret_cast<uint64_t>(::mmap(reinterpret_cast<void*>(StartingPage), length, prot, flags, fd, offset));
        if (Result != ~0ULL) {
          return Result;
        }
      }
      // If we get here then we are out of memory
      return -ENOMEM;
    }
#else
    return Thread->CTX->SyscallHandler->HandleMMAP(Thread, addr, length, prot, flags, fd, offset);
#endif
  }

}
