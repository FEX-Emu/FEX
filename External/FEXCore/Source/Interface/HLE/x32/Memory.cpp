#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x32/Syscalls.h"

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
    static uint32_t MMap(FEXCore::Core::InternalThreadState *Thread, void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
#ifdef MEM_PASSTHROUGH
      if (flags & MAP_FIXED) {
        uint64_t Result = reinterpret_cast<uint64_t>(::mmap(addr, length, prot, flags, fd, offset));
        SYSCALL_ERRNO();
      }
#ifdef MAP_32BIT
      else if (addr == nullptr && offset == 0 && fd == -1) {
        prot &= ~PROT_EXEC;
        uint64_t Result = reinterpret_cast<uint64_t>(::mmap(addr, length, prot, flags | MAP_32BIT, fd, offset));
        SYSCALL_ERRNO();
      }
#endif
      else if (!addr) {
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
      else {
        return ~1U;
      }
#else
      return Thread->CTX->SyscallHandler->HandleMMAP(Thread, addr, length, prot, flags, fd, offset);
#endif
    }

  void RegisterMemory() {
    REGISTER_SYSCALL_IMPL_X32(mmap, MMap);

    REGISTER_SYSCALL_IMPL_X32(mmap2, [](FEXCore::Core::InternalThreadState *Thread, void *addr, size_t length, int prot, int flags, int fd, off_t pgoffset) -> uint32_t {
      return MMap(Thread, addr, length, prot, flags, fd, pgoffset * 0x1000);
    });

    REGISTER_SYSCALL_IMPL_X32(mprotect, [](FEXCore::Core::InternalThreadState *Thread, void *addr, size_t len, int prot) -> uint64_t {
      uint64_t Result = ::mprotect(addr, len, prot);
      SYSCALL_ERRNO();
    });
  }

}
