/*
$info$
meta: LinuxSyscalls|syscalls-x86-32 ~ x86-32 specific syscall implementations
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#include "Common/MathUtils.h"

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/Utils/LogManager.h>

#include <algorithm>
#include <cstdint>
#include <sys/epoll.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <vector>

ARG_TO_STR(FEX::HLE::x32::compat_ptr<FEX::HLE::epoll_event_x86>, "%lx")

namespace FEX::HLE::x32 {
  void RegisterEpoll(FEX::HLE::SyscallHandler *const Handler) {
    REGISTER_SYSCALL_IMPL_X32(epoll_wait, [](FEXCore::Core::CpuStateFrame *Frame, int epfd, compat_ptr<epoll_event_x86> events, int maxevents, int timeout) -> uint64_t {
      std::vector<struct epoll_event> Events(std::max(0, maxevents));
      uint64_t Result = ::syscall(SYS_epoll_pwait, epfd, Events.data(), maxevents, timeout, nullptr);

      if (Result != -1) {
        for (size_t i = 0; i < Result; ++i) {
          events[i] = Events[i];
        }
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(epoll_ctl, [](FEXCore::Core::CpuStateFrame *Frame, int epfd, int op, int fd, epoll_event_x86 *event) -> uint64_t {
      struct epoll_event Event = *event;
      uint64_t Result = ::syscall(SYS_epoll_ctl, epfd, op, fd, &Event);
      if (Result != -1) {
        *event = Event;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(epoll_pwait, [](FEXCore::Core::CpuStateFrame *Frame, int epfd, compat_ptr<epoll_event_x86> events, int maxevent, int timeout, const uint64_t* sigmask, size_t sigsetsize) -> uint64_t {
      std::vector<struct epoll_event> Events(std::max(0, maxevent));

      uint64_t Result = ::syscall(SYS_epoll_pwait,
        epfd,
        Events.data(),
        maxevent,
        timeout,
        sigmask,
        sigsetsize);

      if (Result != -1) {
        for (size_t i = 0; i < Result; ++i) {
          events[i] = Events[i];
        }
      }

      SYSCALL_ERRNO();
    });

    if (Handler->GetHostKernelVersion() >= FEX::HLE::SyscallHandler::KernelVersion(5, 11, 0)) {
#ifndef SYS_epoll_pwait2
#define SYS_epoll_pwait2 354
#endif
      REGISTER_SYSCALL_IMPL_X32(epoll_pwait2, [](FEXCore::Core::CpuStateFrame *Frame, int epfd, compat_ptr<epoll_event_x86> events, int maxevent, compat_ptr<timespec32> timeout, const uint64_t* sigmask, size_t sigsetsize) -> uint64_t {
        std::vector<struct epoll_event> Events(std::max(0, maxevent));

        struct timespec tp64{};
        struct timespec *timed_ptr{};
        if (timeout) {
          tp64 = *timeout;
          timed_ptr = &tp64;
        }

        uint64_t Result = ::syscall(SYS_epoll_pwait2,
          epfd,
          Events.data(),
          maxevent,
          timed_ptr,
          sigmask,
          sigsetsize);

        if (Result != -1) {
          for (size_t i = 0; i < Result; ++i) {
            events[i] = Events[i];
          }
        }

        SYSCALL_ERRNO();
      });
    }
    else {
      REGISTER_SYSCALL_IMPL_X32(epoll_pwait2, UnimplementedSyscallSafe);
    }

  }
}
