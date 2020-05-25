#include "Interface/HLE/Syscalls.h"

#include <cstring>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <syslog.h>
#include <sys/random.h>
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>

#ifndef FEXCORE_VERSION
#define FEXCORE_VERSION "1"
#endif
namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {
  uint64_t Uname(FEXCore::Core::InternalThreadState *Thread, struct utsname *buf) {
    strcpy(buf->sysname, "Linux");
    strcpy(buf->nodename, "FEXCore");
    strcpy(buf->release, "5.0.0");
    strcpy(buf->version, "#" FEXCORE_VERSION);
    strcpy(buf->machine, "x64_64");
    return 0;
  }

  uint64_t Getrusage(FEXCore::Core::InternalThreadState *Thread, int who, struct rusage *usage) {
    uint64_t Result = ::getrusage(who, usage);
    SYSCALL_ERRNO();
  }

  uint64_t Sysinfo(FEXCore::Core::InternalThreadState *Thread, struct sysinfo *info) {
    uint64_t Result = ::sysinfo(info);
    SYSCALL_ERRNO();
  }

  uint64_t Syslog(FEXCore::Core::InternalThreadState *Thread, int priority, const char *format, ...) {
    std::vector<char> MaxString(2048);
    va_list argp;
    va_start(argp, format);
    snprintf(&MaxString.at(0), MaxString.size(), format, argp);
    va_end(argp);
    ::syslog(priority, "%s", &MaxString.at(0));
    return 0;
  }

  uint64_t Getrandom(FEXCore::Core::InternalThreadState *Thread, void *buf, size_t buflen, unsigned int flags) {
    uint64_t Result = ::getrandom(buf, buflen, flags);
    SYSCALL_ERRNO();
  }
}
