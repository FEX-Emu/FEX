#include "Interface/Context/Context.h"
#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"

#include <sys/utsname.h>

#ifndef FEXCORE_VERSION
#define FEXCORE_VERSION "1"
#endif

namespace FEXCore::HLE::x64 {
  void RegisterInfo() {
    REGISTER_SYSCALL_IMPL_X64(uname, [](FEXCore::Core::InternalThreadState *Thread, struct utsname *buf) -> uint64_t {
      strcpy(buf->sysname, "Linux");
      strcpy(buf->nodename, "FEXCore");
      strcpy(buf->release, "5.0.0");
      strcpy(buf->version, "#" FEXCORE_VERSION);
      strcpy(buf->machine, "x86_64");
      return 0;
    });
  }
}
