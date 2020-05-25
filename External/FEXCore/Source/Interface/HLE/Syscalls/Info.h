#pragma once
#include <stddef.h>
#include <stdint.h>
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {
  uint64_t Uname(FEXCore::Core::InternalThreadState *Thread, struct utsname *buf);
  uint64_t Getrusage(FEXCore::Core::InternalThreadState *Thread, int who, struct rusage *usage);
  uint64_t Sysinfo(FEXCore::Core::InternalThreadState *Thread, struct sysinfo *info);
  uint64_t Syslog(FEXCore::Core::InternalThreadState *Thread, int priority, const char *format, ...);
  uint64_t Getrandom(FEXCore::Core::InternalThreadState *Thread, void *buf, size_t buflen, unsigned int flags);
}
