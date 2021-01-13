#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"

#include <sys/resource.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>

namespace FEX::HLE::x32 {
  struct rlimit32 {
    uint32_t rlim_cur;
    uint32_t rlim_max;
  };

  struct sysinfo32 {
    int32_t uptime;
    uint32_t loads[3];
    uint32_t totalram;
    uint32_t freeram;
    uint32_t sharedram;
    uint32_t bufferram;
    uint32_t totalswap;
    uint32_t freeswap;
    uint16_t procs;
    uint32_t totalhigh;
    uint32_t mem_unit;
    char _pad[12];
  };

  static_assert(sizeof(sysinfo32) == 64, "Needs to be 64bytes");

  void RegisterInfo() {
    REGISTER_SYSCALL_IMPL_X32(uname, [](FEXCore::Core::InternalThreadState *Thread, struct utsname *buf) -> uint64_t {
      strcpy(buf->sysname, "Linux");
      strcpy(buf->nodename, "FEXCore");
      strcpy(buf->release, "5.0.0");
      strcpy(buf->version, "#" FEXCORE_VERSION);
      // Tell the guest that we are a 64bit kernel
      strcpy(buf->machine, "x86_64");
      return 0;
    });

    REGISTER_SYSCALL_IMPL_X32(ugetrlimit, [](FEXCore::Core::InternalThreadState *Thread, int resource, rlimit32 *rlim) -> uint64_t {
      struct rlimit rlim64{};
      uint64_t Result = ::getrlimit(resource, &rlim64);
      rlim->rlim_cur = rlim64.rlim_cur;
      rlim->rlim_max = rlim64.rlim_max;
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(sysinfo, [](FEXCore::Core::InternalThreadState *Thread, struct sysinfo32 *info) -> uint64_t {
      struct sysinfo Host{};
      uint64_t Result = ::sysinfo(&Host);
      if (Result != -1) {
#define Copy(x) info->x = static_cast<decltype(info->x)>(std::min(Host.x, static_cast<decltype(Host.x)>(std::numeric_limits<decltype(info->x)>::max())));
        Copy(uptime);
        info->loads[0] = std::min(Host.loads[0], static_cast<unsigned long>(std::numeric_limits<uint32_t>::max()));
        info->loads[1] = std::min(Host.loads[1], static_cast<unsigned long>(std::numeric_limits<uint32_t>::max()));
        info->loads[2] = std::min(Host.loads[2], static_cast<unsigned long>(std::numeric_limits<uint32_t>::max()));
        Copy(totalram);
        Copy(sharedram);
        Copy(bufferram);
        Copy(totalswap);
        Copy(freeswap);
        Copy(procs);
        Copy(totalhigh);
        Copy(mem_unit);
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(getrusage, [](FEXCore::Core::InternalThreadState *Thread, int who, rusage_32 *usage) -> uint64_t {
      struct rusage usage64 = *usage;
      uint64_t Result = ::getrusage(who, &usage64);
      *usage = usage64;
      SYSCALL_ERRNO();
    });
  }
}
