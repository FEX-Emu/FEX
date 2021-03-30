/*
$info$
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include <FEXCore/Core/Context.h>

#include <cstring>
#include <sys/utsname.h>
#include <sys/resource.h>
#include <sys/sysinfo.h>

namespace FEX::HLE::x64 {
  void RegisterInfo() {
    REGISTER_SYSCALL_IMPL_X64(sysinfo, [](FEXCore::Core::CpuStateFrame *Frame, struct sysinfo *info) -> uint64_t {
      uint64_t Result = ::sysinfo(info);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(getrusage, [](FEXCore::Core::CpuStateFrame *Frame, int who, struct rusage *usage) -> uint64_t {
      uint64_t Result = ::getrusage(who, usage);
      SYSCALL_ERRNO();
    });
  }
}
