/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"

#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace FEX::HLE {
  void RegisterKey() {
    REGISTER_SYSCALL_IMPL(add_key, [](FEXCore::Core::CpuStateFrame *Frame, const char *type, const char *description, const void *payload, size_t plen, int32_t /*key_serial_t*/ keyring) -> uint64_t {
      uint64_t Result = syscall(SYS_add_key, type, description, payload, plen, keyring);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(request_key, [](FEXCore::Core::CpuStateFrame *Frame, const char *type, const char *description, const char *callout_info, int32_t /*key_serial_t*/ dest_keyring) -> uint64_t {
      uint64_t Result = syscall(SYS_request_key, type, description, callout_info, dest_keyring);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(keyctl, [](FEXCore::Core::CpuStateFrame *Frame, int operation, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5) -> uint64_t {
      uint64_t Result = syscall(SYS_keyctl, operation, arg2, arg3, arg4, arg5);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(pkey_mprotect, [](FEXCore::Core::CpuStateFrame *Frame, void *addr, size_t len, int prot, int pkey) -> uint64_t {
      // Added in Linux 4.9
      uint64_t Result = ::pkey_mprotect(addr, len, prot, pkey);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(pkey_alloc, [](FEXCore::Core::CpuStateFrame *Frame, unsigned int flags, unsigned int access_rights) -> uint64_t {
      // Added in Linux 4.9
      uint64_t Result = ::pkey_alloc(flags, access_rights);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(pkey_free, [](FEXCore::Core::CpuStateFrame *Frame, int pkey) -> uint64_t {
      // Added in Linux 4.9
      uint64_t Result = ::pkey_free(pkey);
      SYSCALL_ERRNO();
    });
  }
}
