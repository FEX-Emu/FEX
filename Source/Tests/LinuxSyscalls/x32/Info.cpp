/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Types.h"

#include <algorithm>
#include <asm/posix_types.h>
#include <bits/types/struct_rusage.h>
#include <limits>
#include <linux/sysinfo.h>
#include <stdint.h>
#include <sys/resource.h>
#include <sys/sysinfo.h>

namespace FEXCore::Core {
  struct CpuStateFrame;
}

ARG_TO_STR(FEX::HLE::x32::compat_ptr<FEX::HLE::x32::rlimit32<true>>, "%lx")
ARG_TO_STR(FEX::HLE::x32::compat_ptr<FEX::HLE::x32::rlimit32<false>>, "%lx")

namespace FEX::HLE::x32 {
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
    uint32_t freehigh;
    uint32_t mem_unit;
    char _pad[8];
  };

  static_assert(sizeof(sysinfo32) == 64, "Needs to be 64bytes");

  void RegisterInfo() {
    REGISTER_SYSCALL_IMPL_X32(getrlimit, [](FEXCore::Core::CpuStateFrame *Frame, int resource, compat_ptr<FEX::HLE::x32::rlimit32<true>> rlim) -> uint64_t {
      struct rlimit rlim64{};
      uint64_t Result = ::getrlimit(resource, &rlim64);
      *rlim = rlim64;
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(ugetrlimit, [](FEXCore::Core::CpuStateFrame *Frame, int resource, compat_ptr<FEX::HLE::x32::rlimit32<false>> rlim) -> uint64_t {
      struct rlimit rlim64{};
      uint64_t Result = ::getrlimit(resource, &rlim64);
      *rlim = rlim64;
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(setrlimit, [](FEXCore::Core::CpuStateFrame *Frame, int resource, const compat_ptr<FEX::HLE::x32::rlimit32<false>> rlim) -> uint64_t {
      struct rlimit rlim64{};
      rlim64 = *rlim;
      uint64_t Result = ::setrlimit(resource, &rlim64);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(sysinfo, [](FEXCore::Core::CpuStateFrame *Frame, struct sysinfo32 *info) -> uint64_t {
      struct sysinfo Host{};
      uint64_t Result = ::sysinfo(&Host);
      if (Result != -1) {
#define Copy(x) info->x = static_cast<decltype(info->x)>(std::min(Host.x, static_cast<decltype(Host.x)>(std::numeric_limits<decltype(info->x)>::max())));
        Copy(uptime);
        Copy(procs);
#define CopyShift(x) info->x = static_cast<decltype(info->x)>(Host.x >> ShiftAmount);

        info->loads[0] = std::min(Host.loads[0], static_cast<unsigned long>(std::numeric_limits<uint32_t>::max()));
        info->loads[1] = std::min(Host.loads[1], static_cast<unsigned long>(std::numeric_limits<uint32_t>::max()));
        info->loads[2] = std::min(Host.loads[2], static_cast<unsigned long>(std::numeric_limits<uint32_t>::max()));

        // If any result can't fit in to a uint32_t then we need to shift the mem_unit and all the members
        // Set the mem_unit to the pagesize
        uint32_t ShiftAmount{};
        if ((Host.totalram >> 32) != 0 ||
            (Host.totalswap >> 32) != 0) {

          while (Host.mem_unit < 4096) {
            Host.mem_unit <<= 1;
            ++ShiftAmount;
          }
        }

        CopyShift(totalram);
        CopyShift(sharedram);
        CopyShift(bufferram);
        CopyShift(totalswap);
        CopyShift(freeswap);
        CopyShift(totalhigh);
        CopyShift(freehigh);
        Copy(mem_unit);
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(getrusage, [](FEXCore::Core::CpuStateFrame *Frame, int who, rusage_32 *usage) -> uint64_t {
      struct rusage usage64 = *usage;
      uint64_t Result = ::getrusage(who, &usage64);
      *usage = usage64;
      SYSCALL_ERRNO();
    });
  }
}
