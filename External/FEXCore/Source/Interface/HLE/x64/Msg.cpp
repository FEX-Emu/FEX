#include "Interface/Context/Context.h"
#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"

#include <mqueue.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE::x64 {
  void RegisterMsg() {
    REGISTER_SYSCALL_IMPL_X64(mq_timedsend, [](FEXCore::Core::InternalThreadState *Thread, mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned int msg_prio, const struct timespec *abs_timeout) -> uint64_t {
      uint64_t Result = ::mq_timedsend(mqdes, msg_ptr, msg_len, msg_prio, abs_timeout);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(mq_timedreceive, [](FEXCore::Core::InternalThreadState *Thread, mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned int *msg_prio, const struct timespec *abs_timeout) -> uint64_t {
      uint64_t Result = ::mq_timedreceive(mqdes, msg_ptr, msg_len, msg_prio, abs_timeout);
      SYSCALL_ERRNO();
    });
  }
}
