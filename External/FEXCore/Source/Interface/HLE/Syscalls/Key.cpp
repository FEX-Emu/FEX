#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"
#include "Interface/HLE/x32/Syscalls.h"

#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {
  void RegisterKey() {
    REGISTER_SYSCALL_IMPL(add_key, [](FEXCore::Core::InternalThreadState *Thread, const char *type, const char *description, const void *payload, size_t plen, int32_t /*key_serial_t*/ keyring) -> uint64_t {
      uint64_t Result = syscall(SYS_add_key, type, description, payload, plen, keyring);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(request_key, [](FEXCore::Core::InternalThreadState *Thread, const char *type, const char *description, const char *callout_info, int32_t /*key_serial_t*/ dest_keyring) -> uint64_t {
      uint64_t Result = syscall(SYS_request_key, type, description, callout_info, dest_keyring);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(keyctl, [](FEXCore::Core::InternalThreadState *Thread, int operation, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5) -> uint64_t {
      uint64_t Result = syscall(SYS_keyctl, operation, arg2, arg3, arg4, arg5);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(pkey_mprotect, [](FEXCore::Core::InternalThreadState *Thread, void *addr, size_t len, int prot, int pkey) -> uint64_t {
      // Added in Linux 4.9
      uint64_t Result = ::pkey_mprotect(addr, len, prot, pkey);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(pkey_alloc, [](FEXCore::Core::InternalThreadState *Thread, unsigned int flags, unsigned int access_rights) -> uint64_t {
      // Added in Linux 4.9
      uint64_t Result = ::pkey_alloc(flags, access_rights);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(pkey_free, [](FEXCore::Core::InternalThreadState *Thread, int pkey) -> uint64_t {
      // Added in Linux 4.9
      uint64_t Result = ::pkey_free(pkey);
      SYSCALL_ERRNO();
    });
  }
}
