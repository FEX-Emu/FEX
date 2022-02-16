/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"

#include <FEXCore/IR/IR.h>

#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace FEX::HLE {
  void RegisterMemory() {
    using namespace FEXCore::IR;

    REGISTER_SYSCALL_IMPL_FLAGS(brk, SYSCALL_FLAG_OPTIMIZETHROUGH | SYSCALL_FLAG_NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, void *addr) -> uint64_t {
      uint64_t Result = FEX::HLE::_SyscallHandler->HandleBRK(Frame, addr);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(msync, SYSCALL_FLAG_OPTIMIZETHROUGH | SYSCALL_FLAG_NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, void *addr, size_t length, int32_t flags) -> uint64_t {
      uint64_t Result = ::msync(addr, length, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(mincore, SYSCALL_FLAG_OPTIMIZETHROUGH | SYSCALL_FLAG_NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, void *addr, size_t length, uint8_t *vec) -> uint64_t {
      uint64_t Result = ::mincore(addr, length, vec);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(madvise, SYSCALL_FLAG_OPTIMIZETHROUGH | SYSCALL_FLAG_NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, void *addr, size_t length, int32_t advice) -> uint64_t {
      uint64_t Result = ::madvise(addr, length, advice);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(mlock, SYSCALL_FLAG_OPTIMIZETHROUGH | SYSCALL_FLAG_NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, const void *addr, size_t len) -> uint64_t {
      uint64_t Result = ::mlock(addr, len);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(munlock, SYSCALL_FLAG_OPTIMIZETHROUGH | SYSCALL_FLAG_NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, const void *addr, size_t len) -> uint64_t {
      uint64_t Result = ::munlock(addr, len);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(mlock2, SYSCALL_FLAG_OPTIMIZETHROUGH | SYSCALL_FLAG_NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, const void *addr, size_t len, int flags) -> uint64_t {
      uint64_t Result = ::mlock2(addr, len, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(remap_file_pages, SYSCALL_FLAG_OPTIMIZETHROUGH | SYSCALL_FLAG_NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, void *addr, size_t size, int prot, size_t pgoff, int flags) -> uint64_t {
      // This syscall is deprecated, not sure when it will end up being removed
      uint64_t Result = ::remap_file_pages(addr, size, prot, pgoff, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(mbind, SYSCALL_FLAG_OPTIMIZETHROUGH | SYSCALL_FLAG_NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, void *addr, unsigned long len, int mode, const unsigned long *nodemask, unsigned long maxnode, unsigned flags) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(mbind), addr, len, mode, nodemask, maxnode, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(get_mempolicy, SYSCALL_FLAG_OPTIMIZETHROUGH | SYSCALL_FLAG_NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int *mode, unsigned long *nodemask, unsigned long maxnode, void *addr, unsigned long flags) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(get_mempolicy), mode, nodemask, maxnode, addr, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(set_mempolicy, SYSCALL_FLAG_OPTIMIZETHROUGH | SYSCALL_FLAG_NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int mode, const unsigned long *nodemask, unsigned long maxnode) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(set_mempolicy), mode, nodemask, maxnode);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(migrate_pages, SYSCALL_FLAG_OPTIMIZETHROUGH | SYSCALL_FLAG_NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int pid, unsigned long maxnode, const unsigned long *old_nodes, const unsigned long *new_nodes) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(migrate_pages), pid, maxnode, old_nodes, new_nodes);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(move_pages, SYSCALL_FLAG_OPTIMIZETHROUGH | SYSCALL_FLAG_NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int pid, unsigned long count, void **pages, const int *nodes, int *status, int flags) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(move_pages), pid, count, pages, nodes, status, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(membarrier, SYSCALL_FLAG_OPTIMIZETHROUGH | SYSCALL_FLAG_NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int cmd, int flags) -> uint64_t {
        uint64_t Result = syscall(SYSCALL_DEF(membarrier), cmd, flags);
        SYSCALL_ERRNO();
    });
  }
}
