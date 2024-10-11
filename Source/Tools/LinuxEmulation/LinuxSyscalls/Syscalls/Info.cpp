// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x64/Syscalls.h"
#include "LinuxSyscalls/x32/Syscalls.h"

#include <FEXCore/IR/IR.h>
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
#include <sys/personality.h>
#include <unistd.h>

#include <git_version.h>

namespace FEX::HLE {
using cap_user_header_t = void*;
using cap_user_data_t = void*;

void RegisterInfo(FEX::HLE::SyscallHandler* Handler) {
  using namespace FEXCore::IR;

  REGISTER_SYSCALL_IMPL_FLAGS(
    uname, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY, [](FEXCore::Core::CpuStateFrame* Frame, struct utsname* buf) -> uint64_t {
      struct utsname Local {};
      if (::uname(&Local) == 0) {
        memcpy(buf->nodename, Local.nodename, sizeof(Local.nodename));
        static_assert(sizeof(Local.nodename) <= sizeof(buf->nodename));
        memcpy(buf->domainname, Local.domainname, sizeof(Local.domainname));
        static_assert(sizeof(Local.domainname) <= sizeof(buf->domainname));
      } else {
        strcpy(buf->nodename, "FEXCore");
        LogMan::Msg::EFmt("Couldn't determine host nodename. Defaulting to '{}'", buf->nodename);
      }
      strcpy(buf->sysname, "Linux");
      uint32_t GuestVersion = FEX::HLE::_SyscallHandler->GetGuestKernelVersion();
      snprintf(buf->release, sizeof(buf->release), "%d.%d.%d", FEX::HLE::SyscallHandler::KernelMajor(GuestVersion),
               FEX::HLE::SyscallHandler::KernelMinor(GuestVersion), FEX::HLE::SyscallHandler::KernelPatch(GuestVersion));

      const char version[] = "#" GIT_DESCRIBE_STRING " SMP " __DATE__ " " __TIME__;
      strcpy(buf->version, version);
      static_assert(sizeof(version) <= sizeof(buf->version), "uname version define became too large!");
      // Tell the guest that we are a 64bit kernel
      strcpy(buf->machine, "x86_64");
      return 0;
    });

  REGISTER_SYSCALL_IMPL_PASS_FLAGS(personality, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   [](FEXCore::Core::CpuStateFrame* Frame, uint32_t persona) -> uint64_t {
                                     auto Thread = FEX::HLE::ThreadManager::GetStateObjectFromCPUState(Frame);

                                     if (persona == ~0U) {
                                       // Special case, only queries the persona.
                                       return Thread->persona;
                                     }

                                     // Mask off `PER_LINUX32` because AArch64 doesn't support it.
                                     uint32_t NewPersona = persona & ~PER_LINUX32;

                                     // This syscall can not physically fail with PER_LINUX32 masked off.
                                     // It also can not fail on a real x86 kernel.
                                     (void)::syscall(SYSCALL_DEF(personality), NewPersona);

                                     // Return the old persona while setting the new one.
                                     auto OldPersona = Thread->persona;
                                     Thread->persona = persona;
                                     return OldPersona;
                                   });

  REGISTER_SYSCALL_IMPL_FLAGS(seccomp, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                              [](FEXCore::Core::CpuStateFrame* Frame, unsigned int operation, unsigned int flags, void* args) -> uint64_t {
                                return FEX::HLE::_SyscallHandler->SeccompEmulator.Handle(Frame, operation, flags, args);
                              });
}
} // namespace FEX::HLE
