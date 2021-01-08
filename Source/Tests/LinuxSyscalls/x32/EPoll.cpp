#include "Common/MathUtils.h"

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/Utils/LogManager.h>

#include <stdint.h>
#include <sys/epoll.h>
#include <vector>

ARG_TO_STR(FEX::HLE::x32::compat_ptr<FEX::HLE::epoll_event_x86>, "%lx")

namespace FEX::HLE::x32 {
  void RegisterEpoll() {
    REGISTER_SYSCALL_IMPL_X32(epoll_wait, [](FEXCore::Core::InternalThreadState *Thread, int epfd, compat_ptr<epoll_event_x86> events, int maxevents, int timeout) -> uint64_t {
      std::vector<struct epoll_event> Events;
      Events.resize(maxevents);
      uint64_t Result = epoll_wait(epfd, &Events.at(0), maxevents, timeout);

      if (Result != -1) {
        for (size_t i = 0; i < Result; ++i) {
          events[i] = Events[i];
        }
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(epoll_ctl, [](FEXCore::Core::InternalThreadState *Thread, int epfd, int op, int fd, epoll_event_x86 *event) -> uint64_t {
      struct epoll_event Event = *event;
      uint64_t Result = epoll_ctl(epfd, op, fd, &Event);
      if (Result != -1) {
        *event = Event;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(epoll_pwait, [](FEXCore::Core::InternalThreadState *Thread, int epfd, compat_ptr<epoll_event_x86> events, int maxevent, int timeout, const void* sigmask) -> uint64_t {
      std::vector<struct epoll_event> Events;
      Events.resize(maxevent);

      uint64_t Result = epoll_pwait(
        epfd,
        &Events.at(0),
        maxevent,
        timeout,
        reinterpret_cast<const sigset_t*>(sigmask));

      if (Result != -1) {
        for (size_t i = 0; i < Result; ++i) {
          events[i] = Events[i];
        }
      }

      SYSCALL_ERRNO();
    });
  }
}
