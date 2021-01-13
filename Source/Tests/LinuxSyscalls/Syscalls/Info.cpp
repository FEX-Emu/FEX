#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"

#include <cstring>
#include <linux/kcmp.h>
#include <linux/seccomp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <syslog.h>
#include <sys/capability.h>
#include <sys/random.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/utsname.h>
#include <sys/klog.h>
#include <unistd.h>

namespace FEX::HLE {
  void RegisterInfo() {
    REGISTER_SYSCALL_IMPL(getrlimit, [](FEXCore::Core::InternalThreadState *Thread, int resource, struct rlimit *rlim) -> uint64_t {
      uint64_t Result = ::getrlimit(resource, rlim);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(setrlimit, [](FEXCore::Core::InternalThreadState *Thread, int resource, const struct rlimit *rlim) -> uint64_t {
      uint64_t Result = ::setrlimit(resource, rlim);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(syslog, [](FEXCore::Core::InternalThreadState *Thread, int type, char *bufp, int len) -> uint64_t {
      uint64_t Result = ::klogctl(type, bufp, len);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(getrandom, [](FEXCore::Core::InternalThreadState *Thread, void *buf, size_t buflen, unsigned int flags) -> uint64_t {
      uint64_t Result = ::getrandom(buf, buflen, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(capget, [](FEXCore::Core::InternalThreadState *Thread, cap_user_header_t hdrp, cap_user_data_t datap) -> uint64_t {
      uint64_t Result = ::capget(hdrp, datap);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(capset, [](FEXCore::Core::InternalThreadState *Thread, cap_user_header_t hdrp, const cap_user_data_t datap) -> uint64_t {
      uint64_t Result = ::capset(hdrp, datap);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(getcpu, [](FEXCore::Core::InternalThreadState *Thread, unsigned *cpu, unsigned *node, struct getcpu_cache *tcache) -> uint64_t {
      uint32_t LocalCPU{};
      uint32_t LocalNode{};
      // tcache is ignored
      uint64_t Result = ::syscall(SYS_getcpu, cpu ? &LocalCPU : nullptr, node ? &LocalNode : nullptr, nullptr);
      if (Result == 0) {
        if (cpu) {
          // Ensure we don't return a number over our number of emulated cores
          *cpu = LocalCPU % FEX::HLE::_SyscallHandler->ThreadsConfig();
        }

        if (node) {
          // Just claim we are part of node zero
          *node = 0;
        }
      }
      SYSCALL_ERRNO();
    });

    //compare  two  processes  to determine if they share a kernel resource
    REGISTER_SYSCALL_IMPL(kcmp, [](FEXCore::Core::InternalThreadState *Thread, pid_t pid1, pid_t pid2, int type, unsigned long idx1, unsigned long idx2) -> uint64_t {
      uint64_t Result = ::syscall(SYS_kcmp, pid1, pid2, type, idx1, idx2);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(seccomp, [](FEXCore::Core::InternalThreadState *Thread, unsigned int operation, unsigned int flags, void *args) -> uint64_t {
      uint64_t Result = ::syscall(SYS_seccomp, operation, flags, args);
      SYSCALL_ERRNO();
    });
  }
}
