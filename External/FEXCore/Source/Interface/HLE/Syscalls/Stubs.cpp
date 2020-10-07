#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"
#include "Interface/HLE/x32/Syscalls.h"

#include "LogManager.h"

#include <sys/time.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/ptrace.h>
#include <grp.h>
#include <sys/fsuid.h>
#include <sys/capability.h>
#include <utime.h>
#include <signal.h>
#include <unistd.h>
#include <sys/timex.h>
#include <sys/resource.h>
#include <linux/aio_abi.h>
#include <mqueue.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

#define SYSCALL_STUB(name) do { ERROR_AND_DIE("Syscall: " #name " stub!"); return -ENOSYS; } while(0)

namespace FEXCore::HLE {

  void RegisterStubs() {

    REGISTER_SYSCALL_IMPL(rt_sigreturn, [](FEXCore::Core::InternalThreadState *Thread) -> uint64_t {
      SYSCALL_STUB(rt_sigreturn);
    });

    REGISTER_SYSCALL_IMPL(ptrace, [](FEXCore::Core::InternalThreadState *Thread, int /*enum __ptrace_request*/ request, pid_t pid, void *addr, void *data) -> uint64_t {
      SYSCALL_STUB(ptrace);
    });

    REGISTER_SYSCALL_IMPL(rt_sigtimedwait, [](FEXCore::Core::InternalThreadState *Thread, sigset_t *set, const struct timespec*, size_t sigsetsize) -> uint64_t {
      SYSCALL_STUB(rt_sigtimedwait);
    });

    REGISTER_SYSCALL_IMPL(rt_sigqueueinfo, [](FEXCore::Core::InternalThreadState *Thread, pid_t pid, int sig, siginfo_t *uinfo) -> uint64_t {
      SYSCALL_STUB(rt_sigqueueinfo);
    });

    REGISTER_SYSCALL_IMPL(modify_ldt, [](FEXCore::Core::InternalThreadState *Thread, int func, void *ptr, unsigned long bytecount) -> uint64_t {
      SYSCALL_STUB(modify_ldt);
    });

    REGISTER_SYSCALL_IMPL(restart_syscall, [](FEXCore::Core::InternalThreadState *Thread) -> uint64_t {
      SYSCALL_STUB(restart_syscall);
    });

    REGISTER_SYSCALL_IMPL(signalfd, [](FEXCore::Core::InternalThreadState *Thread, int fd, const sigset_t *mask, size_t sizemask) -> uint64_t {
      SYSCALL_STUB(signalfd);
    });

    REGISTER_SYSCALL_IMPL(signalfd4, [](FEXCore::Core::InternalThreadState *Thread, int fd, const sigset_t *mask, size_t sizemask, int flags) -> uint64_t {
      SYSCALL_STUB(signalfd4);
    });

    REGISTER_SYSCALL_IMPL(rt_tgsigqueueinfo, [](FEXCore::Core::InternalThreadState *Thread, pid_t tgid, pid_t tid, int sig, siginfo_t *info) -> uint64_t {
      SYSCALL_STUB(rt_tgsigqueueinfo);
    });

    // execute program relative to a directory file descriptor
    REGISTER_SYSCALL_IMPL(execveat, [](FEXCore::Core::InternalThreadState *Thread, int dirfd, const char *pathname, char *const argv[], char *const envp[], int flags) -> uint64_t {
      SYSCALL_STUB(execveat);
    });

    REGISTER_SYSCALL_IMPL(rseq, [](FEXCore::Core::InternalThreadState *Thread,  struct rseq  *rseq, uint32_t rseq_len, int flags, uint32_t sig) -> uint64_t {
      SYSCALL_STUB(rseq);
    });
  }
}
