// SPDX-License-Identifier: MIT
/*
$info$
meta: LinuxSyscalls|syscalls-x86-32 ~ x86-32 specific syscall implementations
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/Types.h"
#include "LinuxSyscalls/x32/Syscalls.h"
#include "LinuxSyscalls/x32/Types.h"
#include "LinuxSyscalls/x64/Syscalls.h"

#include <FEXCore/fextl/vector.h>

#include <algorithm>
#include <cstdint>
#include <sys/epoll.h>
#include <syscall.h>
#include <time.h>
#include <unistd.h>

ARG_TO_STR(FEX::HLE::x32::compat_ptr<FEX::HLE::x32::epoll_event32>, "%lx")
ARG_TO_STR(FEX::HLE::x32::compat_ptr<FEX::HLE::x32::timespec32>, "%lx")

namespace FEXCore::Core {
struct CpuStateFrame;
}

namespace FEX::HLE::x32 {
void RegisterEpoll(FEX::HLE::SyscallHandler* Handler) {
  REGISTER_SYSCALL_IMPL_X32(
    epoll_wait,
    [](FEXCore::Core::CpuStateFrame* Frame, int epfd, compat_ptr<FEX::HLE::x32::epoll_event32> events, int maxevents, int timeout) -> uint64_t {
      fextl::vector<struct epoll_event> Events(std::max(0, maxevents));
      uint64_t Result = ::syscall(SYSCALL_DEF(epoll_pwait), epfd, Events.data(), maxevents, timeout, nullptr, 8);

      if (Result != -1) {
        FaultSafeUserMemAccess::VerifyIsWritable(events, sizeof(FEX::HLE::x32::epoll_event32) * Result);
        for (size_t i = 0; i < Result; ++i) {
          events[i] = Events[i];
        }
      }
      SYSCALL_ERRNO();
    });

  REGISTER_SYSCALL_IMPL_X32(
    epoll_ctl, [](FEXCore::Core::CpuStateFrame* Frame, int epfd, int op, int fd, compat_ptr<FEX::HLE::x32::epoll_event32> event) -> uint64_t {
      struct epoll_event Event;
      struct epoll_event* EventPtr {};
      if (event) {
        FaultSafeUserMemAccess::VerifyIsReadable(event, sizeof(FEX::HLE::x32::epoll_event32));
        Event = *event;
        EventPtr = &Event;
      }
      uint64_t Result = ::syscall(SYSCALL_DEF(epoll_ctl), epfd, op, fd, EventPtr);

      if (Result != -1 && event) {
        FaultSafeUserMemAccess::VerifyIsWritable(event, sizeof(FEX::HLE::x32::epoll_event32));
        *event = Event;
      }
      SYSCALL_ERRNO();
    });

  REGISTER_SYSCALL_IMPL_X32(epoll_pwait,
                            [](FEXCore::Core::CpuStateFrame* Frame, int epfd, compat_ptr<FEX::HLE::x32::epoll_event32> events, int maxevent,
                               int timeout, const uint64_t* sigmask, size_t sigsetsize) -> uint64_t {
                              fextl::vector<struct epoll_event> Events(std::max(0, maxevent));

                              uint64_t Result = ::syscall(SYSCALL_DEF(epoll_pwait), epfd, Events.data(), maxevent, timeout, sigmask, sigsetsize);

                              if (Result != -1) {
                                FaultSafeUserMemAccess::VerifyIsWritable(events, sizeof(FEX::HLE::x32::epoll_event32) * Result);
                                for (size_t i = 0; i < Result; ++i) {
                                  events[i] = Events[i];
                                }
                              }

                              SYSCALL_ERRNO();
                            });

  if (Handler->IsHostKernelVersionAtLeast(5, 11, 0)) {
    REGISTER_SYSCALL_IMPL_X32(epoll_pwait2,
                              [](FEXCore::Core::CpuStateFrame* Frame, int epfd, compat_ptr<FEX::HLE::x32::epoll_event32> events,
                                 int maxevent, compat_ptr<timespec32> timeout, const uint64_t* sigmask, size_t sigsetsize) -> uint64_t {
                                fextl::vector<struct epoll_event> Events(std::max(0, maxevent));

                                struct timespec tp64 {};
                                struct timespec* timed_ptr {};
                                if (timeout) {
                                  tp64 = *timeout;
                                  timed_ptr = &tp64;
                                }

                                uint64_t Result =
                                  ::syscall(SYSCALL_DEF(epoll_pwait2), epfd, Events.data(), maxevent, timed_ptr, sigmask, sigsetsize);

                                if (Result != -1) {
                                  FaultSafeUserMemAccess::VerifyIsWritable(events, sizeof(FEX::HLE::x32::epoll_event32) * Result);
                                  for (size_t i = 0; i < Result; ++i) {
                                    events[i] = Events[i];
                                  }
                                }

                                SYSCALL_ERRNO();
                              });
  } else {
    REGISTER_SYSCALL_IMPL_X32(epoll_pwait2, UnimplementedSyscallSafe);
  }
}
} // namespace FEX::HLE::x32
