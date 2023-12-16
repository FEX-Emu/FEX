// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x64/Syscalls.h"
#include "LinuxSyscalls/x32/Syscalls.h"

#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/IR/IR.h>

#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace FEX::HLE {
  void RegisterMemory(FEX::HLE::SyscallHandler *Handler) {
    using namespace FEXCore::IR;

    REGISTER_SYSCALL_IMPL_FLAGS(brk, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, void *addr) -> uint64_t {
      uint64_t Result = FEX::HLE::_SyscallHandler->HandleBRK(Frame, addr);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(msync, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, void *addr, size_t length, int32_t flags) -> uint64_t {
      uint64_t Result = ::msync(addr, length, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(mincore, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, void *addr, size_t length, uint8_t *vec) -> uint64_t {
      uint64_t Result = ::mincore(addr, length, vec);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_FLAGS(madvise, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, void *addr, size_t length, int32_t advice) -> uint64_t {
      uint64_t Result = ::madvise(addr, length, advice);

      if (Result != -1) {
        FEX::HLE::_SyscallHandler->TrackMadvise(Frame->Thread, (uintptr_t)addr, length, advice);
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(mlock, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, const void *addr, size_t len) -> uint64_t {
      uint64_t Result = ::mlock(addr, len);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(munlock, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, const void *addr, size_t len) -> uint64_t {
      uint64_t Result = ::munlock(addr, len);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(mlock2, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, const void *addr, size_t len, int flags) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(mlock2), addr, len, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(remap_file_pages, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, void *addr, size_t size, int prot, size_t pgoff, int flags) -> uint64_t {
      // This syscall is deprecated, not sure when it will end up being removed
      uint64_t Result = ::syscall(SYSCALL_DEF(remap_file_pages), addr, size, prot, pgoff, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(mbind, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, void *addr, unsigned long len, int mode, const unsigned long *nodemask, unsigned long maxnode, unsigned flags) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(mbind), addr, len, mode, nodemask, maxnode, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(get_mempolicy, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int *mode, unsigned long *nodemask, unsigned long maxnode, void *addr, unsigned long flags) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(get_mempolicy), mode, nodemask, maxnode, addr, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(set_mempolicy, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int mode, const unsigned long *nodemask, unsigned long maxnode) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(set_mempolicy), mode, nodemask, maxnode);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(migrate_pages, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int pid, unsigned long maxnode, const unsigned long *old_nodes, const unsigned long *new_nodes) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(migrate_pages), pid, maxnode, old_nodes, new_nodes);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(move_pages, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int pid, unsigned long count, void **pages, const int *nodes, int *status, int flags) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(move_pages), pid, count, pages, nodes, status, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(membarrier, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int cmd, int flags) -> uint64_t {
        uint64_t Result = syscall(SYSCALL_DEF(membarrier), cmd, flags);
        SYSCALL_ERRNO();
    });

    if (Handler->IsHostKernelVersionAtLeast(5, 17, 0)) {
      REGISTER_SYSCALL_IMPL_PASS_FLAGS(set_mempolicy_home_node, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
        [](FEXCore::Core::CpuStateFrame *Frame, uint64_t start, uint64_t len, uint64_t home_node, uint64_t flags) -> uint64_t {
        uint64_t Result = ::syscall(SYSCALL_DEF(set_mempolicy_home_node), start, len, home_node, flags);
        SYSCALL_ERRNO();
      });
    }
    else {
      REGISTER_SYSCALL_IMPL(set_mempolicy_home_node, UnimplementedSyscallSafe);
    }
  }
}
