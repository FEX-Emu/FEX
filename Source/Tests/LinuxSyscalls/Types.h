#pragma once

#include <FEXCore/Utils/CompilerDefs.h>

#include <sys/epoll.h>
#include <type_traits>

namespace FEX::HLE {
using key_serial_t = int32_t;
using kernel_timer_t = int32_t;

struct FEX_PACKED epoll_event_x86 {
  uint32_t events;
  epoll_data_t data;

  epoll_event_x86() = delete;

  operator struct epoll_event() const {
    epoll_event event{};
    event.events = events;
    event.data = data;
    return event;
  }

  epoll_event_x86(struct epoll_event event) {
    events = event.events;
    data = event.data;
  }
};
static_assert(std::is_trivial<epoll_event_x86>::value, "Needs to be trivial");
static_assert(sizeof(epoll_event_x86) == 12, "Incorrect size");


}
