/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Types.h"

#include <mqueue.h>
#include <stdint.h>
#include <syscall.h>
#include <time.h>
#include <unistd.h>

namespace FEX::HLE::x32 {
  void RegisterMsg() {
    REGISTER_SYSCALL_IMPL_X32(mq_timedsend, [](FEXCore::Core::CpuStateFrame *Frame, mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned int msg_prio, const struct timespec32 *abs_timeout) -> uint64_t {
      struct timespec tp64{};
      struct timespec *timed_ptr{};
      if (abs_timeout) {
        tp64 = *abs_timeout;
        timed_ptr = &tp64;
      }

      uint64_t Result = ::syscall(SYS_mq_timedsend, mqdes, msg_ptr, msg_len, msg_prio, timed_ptr);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(mq_timedreceive, [](FEXCore::Core::CpuStateFrame *Frame, mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned int *msg_prio, const struct timespec32 *abs_timeout) -> uint64_t {
      struct timespec tp64{};
      struct timespec *timed_ptr{};
      if (abs_timeout) {
        tp64 = *abs_timeout;
        timed_ptr = &tp64;
      }

      uint64_t Result = ::syscall(SYS_mq_timedreceive, mqdes, msg_ptr, msg_len, msg_prio, timed_ptr);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(mq_timedsend_time64, [](FEXCore::Core::CpuStateFrame *Frame, mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned int msg_prio, const struct timespec *abs_timeout) -> uint64_t {
      uint64_t Result = ::syscall(SYS_mq_timedsend, mqdes, msg_ptr, msg_len, msg_prio, abs_timeout);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(mq_timedreceive_time64, [](FEXCore::Core::CpuStateFrame *Frame, mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned int *msg_prio, const struct timespec *abs_timeout) -> uint64_t {
      uint64_t Result = ::syscall(SYS_mq_timedreceive, mqdes, msg_ptr, msg_len, msg_prio, abs_timeout);
      SYSCALL_ERRNO();
    });
  }
}
