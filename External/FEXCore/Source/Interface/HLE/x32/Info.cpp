#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x32/Syscalls.h"

#include "Interface/Context/Context.h"
#include "Interface/Core/InternalThreadState.h"

#include <sys/utsname.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

#ifndef FEXCORE_VERSION
#define FEXCORE_VERSION "1"
#endif

namespace FEXCore::HLE::x32 {
  void RegisterInfo() {
    REGISTER_SYSCALL_IMPL_X32(uname, [](FEXCore::Core::InternalThreadState *Thread, struct utsname *buf) -> uint64_t {
      strcpy(buf->sysname, "Linux");
      strcpy(buf->nodename, "FEXCore");
      strcpy(buf->release, "5.0.0");
      strcpy(buf->version, "#" FEXCORE_VERSION);
      strcpy(buf->machine, "i386");
      return 0;
    });
  }
}
