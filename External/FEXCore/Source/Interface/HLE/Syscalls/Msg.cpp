#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"
#include "Interface/HLE/x32/Syscalls.h"

#include <mqueue.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {
  void RegisterMsg() {
    REGISTER_SYSCALL_IMPL(msgget, [](FEXCore::Core::InternalThreadState *Thread, key_t key, int msgflg) -> uint64_t {
      uint64_t Result = ::msgget(key, msgflg);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(msgsnd, [](FEXCore::Core::InternalThreadState *Thread, int msqid, const void *msgp, size_t msgsz, int msgflg) -> uint64_t {
      uint64_t Result = ::msgsnd(msqid, msgp, msgsz, msgflg);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(msgrcv, [](FEXCore::Core::InternalThreadState *Thread, int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg) -> uint64_t {
      uint64_t Result = ::msgrcv(msqid, msgp, msgsz, msgtyp, msgflg);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(msgctl, [](FEXCore::Core::InternalThreadState *Thread, int msqid, int cmd, struct msqid_ds *buf) -> uint64_t {
      uint64_t Result = ::msgctl(msqid, cmd, buf);
      SYSCALL_ERRNO();
    });

    // last two parameters are optional
    REGISTER_SYSCALL_IMPL(mq_open, [](FEXCore::Core::InternalThreadState *Thread, const char *name, int oflag, mode_t mode, struct mq_attr *attr) -> uint64_t {
      uint64_t Result = ::mq_open(name, oflag, mode, attr);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(mq_unlink, [](FEXCore::Core::InternalThreadState *Thread, const char *name) -> uint64_t {
      uint64_t Result = ::mq_unlink(name);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(mq_timedsend, [](FEXCore::Core::InternalThreadState *Thread, mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned int msg_prio, const struct timespec *abs_timeout) -> uint64_t {
      uint64_t Result = ::mq_timedsend(mqdes, msg_ptr, msg_len, msg_prio, abs_timeout);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(mq_timedreceive, [](FEXCore::Core::InternalThreadState *Thread, mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned int *msg_prio, const struct timespec *abs_timeout) -> uint64_t {
      uint64_t Result = ::mq_timedreceive(mqdes, msg_ptr, msg_len, msg_prio, abs_timeout);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(mq_notify, [](FEXCore::Core::InternalThreadState *Thread, mqd_t mqdes, const struct sigevent *sevp) -> uint64_t {
      uint64_t Result = ::mq_notify(mqdes, sevp);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(mq_getsetattr, [](FEXCore::Core::InternalThreadState *Thread, mqd_t mqdes, struct mq_attr *newattr, struct mq_attr *oldattr) -> uint64_t {
      uint64_t Result = ::syscall(SYS_mq_getsetattr, mqdes, newattr, oldattr);
      SYSCALL_ERRNO();
    });
  }
}
