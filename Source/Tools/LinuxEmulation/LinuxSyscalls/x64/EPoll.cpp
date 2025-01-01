// SPDX-License-Identifier: MIT
/*
$info$
meta: LinuxSyscalls|syscalls-x86-64 ~ x86-64 specific syscall implementations
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/Types.h"
#include "LinuxSyscalls/x64/Syscalls.h"
#include "LinuxSyscalls/x64/Types.h"

#include <FEXCore/fextl/vector.h>

#include <algorithm>
#include <cstdint>
#include <stddef.h>
#include <sys/epoll.h>
#include <syscall.h>
#include <unistd.h>

struct timespec;
namespace FEXCore::Core {
struct CpuStateFrame;
}

namespace FEX::HLE::x64 {
void RegisterEpoll(FEX::HLE::SyscallHandler* Handler) {
  REGISTER_SYSCALL_IMPL_X64(
    epoll_wait, [](FEXCore::Core::CpuStateFrame* Frame, int epfd, FEX::HLE::epoll_event_x86* events, int maxevents, int timeout) -> uint64_t {
      fextl::vector<struct epoll_event> Events(std::max(0, maxevents));
      uint64_t Result = ::syscall(SYSCALL_DEF(epoll_pwait), epfd, Events.data(), maxevents, timeout, nullptr, 8);

      if (Result != -1) {
        FaultSafeUserMemAccess::VerifyIsWritable(events, sizeof(FEX::HLE::epoll_event_x86) * Result);
        for (size_t i = 0; i < Result; ++i) {
          events[i] = Events[i];
        }
      }
      SYSCALL_ERRNO();
    });

  REGISTER_SYSCALL_IMPL_X64(epoll_ctl, [](FEXCore::Core::CpuStateFrame* Frame, int epfd, int op, int fd, FEX::HLE::epoll_event_x86* event) -> uint64_t {
    struct epoll_event Event;
    struct epoll_event* EventPtr {};
    if (event) {
      FaultSafeUserMemAccess::VerifyIsReadable(event, sizeof(FEX::HLE::epoll_event_x86));
      Event = *event;
      EventPtr = &Event;
    }
    uint64_t Result = ::syscall(SYSCALL_DEF(epoll_ctl), epfd, op, fd, EventPtr);
    if (Result != -1 && event) {
      FaultSafeUserMemAccess::VerifyIsWritable(event, sizeof(FEX::HLE::epoll_event_x86));
      *event = Event;
    }
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X64(epoll_pwait,
                            [](FEXCore::Core::CpuStateFrame* Frame, int epfd, FEX::HLE::epoll_event_x86* events, int maxevent, int timeout,
                               const uint64_t* sigmask, size_t sigsetsize) -> uint64_t {
                              fextl::vector<struct epoll_event> Events(std::max(0, maxevent));

                              uint64_t Result = ::syscall(SYSCALL_DEF(epoll_pwait), epfd, Events.data(), maxevent, timeout, sigmask, sigsetsize);

                              if (Result != -1) {
                                FaultSafeUserMemAccess::VerifyIsWritable(events, sizeof(FEX::HLE::epoll_event_x86) * Result);
                                for (size_t i = 0; i < Result; ++i) {
                                  events[i] = Events[i];
                                }
                              }

                              SYSCALL_ERRNO();
                            });

  REGISTER_SYSCALL_IMPL_X64(epoll_pwait2,
                            [](FEXCore::Core::CpuStateFrame* Frame, int epfd, FEX::HLE::epoll_event_x86* events, int maxevent,
                               timespec* timeout, const uint64_t* sigmask, size_t sigsetsize) -> uint64_t {
                              fextl::vector<struct epoll_event> Events(std::max(0, maxevent));

                              uint64_t Result = ::syscall(SYSCALL_DEF(epoll_pwait2), epfd, Events.data(), maxevent, timeout, sigmask, sigsetsize);

                              if (Result != -1) {
                                FaultSafeUserMemAccess::VerifyIsWritable(events, sizeof(FEX::HLE::epoll_event_x86) * Result);
                                for (size_t i = 0; i < Result; ++i) {
                                  events[i] = Events[i];
                                }
                              }

                              SYSCALL_ERRNO();
                            });
}
} // namespace FEX::HLE::x64
