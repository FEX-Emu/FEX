/*
$info$
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"

#include <mqueue.h>
#include <stddef.h>
#include <stdint.h>
#include <syscall.h>
#include <unistd.h>

namespace FEXCore::Core {
  struct CpuStateFrame;
}

namespace FEX::HLE::x64 {
  void RegisterMsg() {
    REGISTER_SYSCALL_IMPL_X64_PASS(mq_timedsend, [](FEXCore::Core::CpuStateFrame *Frame, mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned int msg_prio, const struct timespec *abs_timeout) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(mq_timedsend), mqdes, msg_ptr, msg_len, msg_prio, abs_timeout);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(mq_timedreceive, [](FEXCore::Core::CpuStateFrame *Frame, mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned int *msg_prio, const struct timespec *abs_timeout) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(mq_timedreceive), mqdes, msg_ptr, msg_len, msg_prio, abs_timeout);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(mq_open, [](FEXCore::Core::CpuStateFrame *Frame, const char *name, int oflag, mode_t mode, struct mq_attr *attr) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(mq_open), name, oflag, mode, attr);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(mq_notify, [](FEXCore::Core::CpuStateFrame *Frame, mqd_t mqdes, const struct sigevent *sevp) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(mq_notify), mqdes, sevp);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(mq_getsetattr, [](FEXCore::Core::CpuStateFrame *Frame, mqd_t mqdes, struct mq_attr *newattr, struct mq_attr *oldattr) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(mq_getsetattr), mqdes, newattr, oldattr);
      SYSCALL_ERRNO();
    });
  }
}
