/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"

#include <FEXCore/Utils/LogManager.h>

#include <cstring>
#include <linux/kcmp.h>
#include <linux/seccomp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <syslog.h>
#include <sys/random.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/utsname.h>
#include <sys/klog.h>
#include <unistd.h>

#include <git_version.h>

namespace FEX::HLE {
  using cap_user_header_t = void*;
  using cap_user_data_t = void*;

  void RegisterInfo() {
    REGISTER_SYSCALL_IMPL(uname, [](FEXCore::Core::CpuStateFrame *Frame, struct utsname *buf) -> uint64_t {
      struct utsname Local{};
      if (::uname(&Local) == 0) {
        memcpy(buf->nodename, Local.nodename, sizeof(Local.nodename));
        static_assert(sizeof(Local.nodename) <= sizeof(buf->nodename));
        memcpy(buf->domainname, Local.domainname, sizeof(Local.domainname));
        static_assert(sizeof(Local.domainname) <= sizeof(buf->domainname));
      }
      else {
        strcpy(buf->nodename, "FEXCore");
        LogMan::Msg::E("Couldn't determine host nodename. Defaulting to '%s'", buf->nodename);
      }
      strcpy(buf->sysname, "Linux");
      uint32_t GuestVersion = FEX::HLE::_SyscallHandler->GetGuestKernelVersion();
      snprintf(buf->release, sizeof(buf->release), "%d.%d.%d",
        FEX::HLE::SyscallHandler::KernelMajor(GuestVersion),
        FEX::HLE::SyscallHandler::KernelMinor(GuestVersion),
        FEX::HLE::SyscallHandler::KernelPatch(GuestVersion));

      const char version[] = "#" GIT_DESCRIBE_STRING " SMP " __DATE__ " " __TIME__;
      strcpy(buf->version, version);
      static_assert(sizeof(version) <= sizeof(buf->version), "uname version define became too large!");
      // Tell the guest that we are a 64bit kernel
      strcpy(buf->machine, "x86_64");
      return 0;
    });

    REGISTER_SYSCALL_IMPL_PASS(syslog, [](FEXCore::Core::CpuStateFrame *Frame, int type, char *bufp, int len) -> uint64_t {
      uint64_t Result = ::klogctl(type, bufp, len);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(getrandom, [](FEXCore::Core::CpuStateFrame *Frame, void *buf, size_t buflen, unsigned int flags) -> uint64_t {
      uint64_t Result = ::getrandom(buf, buflen, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(capget, [](FEXCore::Core::CpuStateFrame *Frame, cap_user_header_t hdrp, cap_user_data_t datap) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(capget), hdrp, datap);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(capset, [](FEXCore::Core::CpuStateFrame *Frame, cap_user_header_t hdrp, const cap_user_data_t datap) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(capset), hdrp, datap);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(getcpu, [](FEXCore::Core::CpuStateFrame *Frame, unsigned *cpu, unsigned *node, struct getcpu_cache *tcache) -> uint64_t {
      uint32_t LocalCPU{};
      uint32_t LocalNode{};
      // tcache is ignored
      uint64_t Result = ::syscall(SYSCALL_DEF(getcpu), cpu ? &LocalCPU : nullptr, node ? &LocalNode : nullptr, nullptr);
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
    REGISTER_SYSCALL_IMPL_PASS(kcmp, [](FEXCore::Core::CpuStateFrame *Frame, pid_t pid1, pid_t pid2, int type, unsigned long idx1, unsigned long idx2) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(kcmp), pid1, pid2, type, idx1, idx2);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(seccomp, [](FEXCore::Core::CpuStateFrame *Frame, unsigned int operation, unsigned int flags, void *args) -> uint64_t {
      // FEX doesn't support seccomp
      return -EINVAL;
    });
  }
}
