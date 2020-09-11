#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"

#include <cstring>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <syslog.h>
#include <sys/capability.h>
#include <sys/random.h>
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <sys/klog.h>

#ifndef FEXCORE_VERSION
#define FEXCORE_VERSION "1"
#endif
namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {

  void RegisterInfo() {
    REGISTER_SYSCALL_IMPL(uname, [](FEXCore::Core::InternalThreadState *Thread, struct utsname *buf) -> uint64_t {
      strcpy(buf->sysname, "Linux");
      strcpy(buf->nodename, "FEXCore");
      strcpy(buf->release, "5.0.0");
      strcpy(buf->version, "#" FEXCORE_VERSION);
      strcpy(buf->machine, "x86_64");
      return 0;
    });

    REGISTER_SYSCALL_IMPL(getrusage, [](FEXCore::Core::InternalThreadState *Thread, int who, struct rusage *usage) -> uint64_t {
      uint64_t Result = ::getrusage(who, usage);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(sysinfo, [](FEXCore::Core::InternalThreadState *Thread, struct sysinfo *info) -> uint64_t {
      uint64_t Result = ::sysinfo(info);
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
  }
}
