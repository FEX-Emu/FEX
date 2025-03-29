// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Utils/CompilerDefs.h>

#include <algorithm>
#include <signal.h>
#include <sys/epoll.h>
#include <type_traits>

namespace FEX::HLE {
using key_serial_t = int32_t;
using kernel_timer_t = int32_t;
using mqd_t = int32_t;

#ifndef GETPID
#define GETPID 11
#endif

#ifndef GETVAL
#define GETVAL 12
#endif

#ifndef GETALL
#define GETALL 13
#endif

#ifndef GETNCNT
#define GETNCNT 14
#endif

#ifndef GETZCNT
#define GETZCNT 15
#endif

#ifndef SETVAL
#define SETVAL 16
#endif

#ifndef SETALL
#define SETALL 17
#endif

#ifndef SEM_STAT
#define SEM_STAT 18
#endif

#ifndef SEM_INFO
#define SEM_INFO 19
#endif

#ifndef SEM_STAT_ANY
#define SEM_STAT_ANY 20
#endif

struct FEX_PACKED epoll_event_x86 {
  uint32_t events;
  epoll_data_t data;

  epoll_event_x86() = delete;

  operator struct epoll_event() const {
    epoll_event event {};
    event.events = events;
    event.data = data;
    return event;
  }

  epoll_event_x86(struct epoll_event event) {
    events = event.events;
    data = event.data;
  }
};
static_assert(std::is_trivially_copyable_v<epoll_event_x86>);
static_assert(sizeof(epoll_event_x86) == 12);

// This directly matches the Linux `struct seminfo` structure
// Due to the way this definition cyclic depends inside of includes, redefine it
// This works around some terrible compile errors on some platforms
struct fex_seminfo {
  int32_t semmap;
  int32_t semmni;
  int32_t semmns;
  int32_t semmnu;
  int32_t semmsl;
  int32_t semopm;
  int32_t semume;
  int32_t semusz;
  int32_t semvmx;
  int32_t semaem;
};

struct FEX_PACKED GuestSAMask {
  uint64_t Val;
};

struct FEX_PACKED GuestSigAction {
  union {
    void (*handler)(int);
    void (*sigaction)(int, siginfo_t*, void*);
  } sigaction_handler;

  uint64_t sa_flags;
  void (*restorer)(void);
  GuestSAMask sa_mask;
};

} // namespace FEX::HLE
