/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Types.h"

#include "Tests/LinuxSyscalls/x64/Syscalls.h"

#include <mqueue.h>
#include <stdint.h>
#include <syscall.h>
#include <time.h>
#include <unistd.h>

ARG_TO_STR(FEX::HLE::x32::compat_ptr<FEX::HLE::x32::mq_attr32>, "%lx")
ARG_TO_STR(FEX::HLE::x32::compat_ptr<FEX::HLE::x32::sigevent32>, "%lx")

namespace FEX::HLE::x32 {
  void RegisterMsg() {
    REGISTER_SYSCALL_IMPL_X32(mq_timedsend, [](FEXCore::Core::CpuStateFrame *Frame, mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned int msg_prio, const struct timespec32 *abs_timeout) -> uint64_t {
      struct timespec tp64{};
      struct timespec *timed_ptr{};
      if (abs_timeout) {
        tp64 = *abs_timeout;
        timed_ptr = &tp64;
      }

      uint64_t Result = ::syscall(SYSCALL_DEF(mq_timedsend), mqdes, msg_ptr, msg_len, msg_prio, timed_ptr);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(mq_timedreceive, [](FEXCore::Core::CpuStateFrame *Frame, mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned int *msg_prio, const struct timespec32 *abs_timeout) -> uint64_t {
      struct timespec tp64{};
      struct timespec *timed_ptr{};
      if (abs_timeout) {
        tp64 = *abs_timeout;
        timed_ptr = &tp64;
      }

      uint64_t Result = ::syscall(SYSCALL_DEF(mq_timedreceive), mqdes, msg_ptr, msg_len, msg_prio, timed_ptr);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL(mq_timedsend_time64, mq_timedsend, [](FEXCore::Core::CpuStateFrame *Frame, mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned int msg_prio, const struct timespec *abs_timeout) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(mq_timedsend), mqdes, msg_ptr, msg_len, msg_prio, abs_timeout);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL(mq_timedreceive_time64, mq_timedreceive, [](FEXCore::Core::CpuStateFrame *Frame, mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned int *msg_prio, const struct timespec *abs_timeout) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(mq_timedreceive), mqdes, msg_ptr, msg_len, msg_prio, abs_timeout);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(mq_open, [](FEXCore::Core::CpuStateFrame *Frame, const char *name, int oflag, mode_t mode, compat_ptr<FEX::HLE::x32::mq_attr32> attr) -> uint64_t {
      mq_attr HostAttr{};
      mq_attr *HostAttr_p{};
      if ((oflag & O_CREAT) && attr) {
        // attr is optional unless O_CREAT is set
        // Then attr can be valid or nullptr
        HostAttr = *attr;
        HostAttr_p = &HostAttr;
      }
      uint64_t Result = ::syscall(SYSCALL_DEF(mq_open), name, oflag, mode, HostAttr_p);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(mq_notify, [](FEXCore::Core::CpuStateFrame *Frame, mqd_t mqdes, const compat_ptr<FEX::HLE::x32::sigevent32> sevp) -> uint64_t {
      sigevent Host = *sevp;
      uint64_t Result = ::syscall(SYSCALL_DEF(mq_notify), mqdes, &Host);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(mq_getsetattr, [](FEXCore::Core::CpuStateFrame *Frame, mqd_t mqdes, compat_ptr<FEX::HLE::x32::mq_attr32> newattr, compat_ptr<FEX::HLE::x32::mq_attr32> oldattr) -> uint64_t {
      mq_attr HostNew{};
      mq_attr *HostNew_p{};

      mq_attr HostOld{};
      mq_attr *HostOld_p{};

      if (newattr) {
        HostNew = *newattr;
        HostNew_p = &HostNew;
      }

      if (oldattr) {
        HostOld_p = &HostOld;
      }

      uint64_t Result = ::syscall(SYSCALL_DEF(mq_getsetattr), mqdes, HostNew_p, HostOld_p);

      if (Result != 1 && oldattr) {
        *oldattr = HostOld;
      }

      SYSCALL_ERRNO();
    });
  }
}
