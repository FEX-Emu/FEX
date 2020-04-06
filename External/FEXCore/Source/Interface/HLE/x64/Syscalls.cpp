#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"

#include "Interface/HLE/Syscalls/EPoll.h"
#include "Interface/HLE/Syscalls/FD.h"
#include "Interface/HLE/Syscalls/FS.h"
#include "Interface/HLE/Syscalls/Info.h"
#include "Interface/HLE/Syscalls/Ioctl.h"
#include "Interface/HLE/Syscalls/Memory.h"
#include "Interface/HLE/Syscalls/Numa.h"
#include "Interface/HLE/Syscalls/Sched.h"
#include "Interface/HLE/Syscalls/SHM.h"
#include "Interface/HLE/Syscalls/Socket.h"
#include "Interface/HLE/Syscalls/Thread.h"
#include "Interface/HLE/Syscalls/Time.h"
#include "Interface/HLE/Syscalls/Timer.h"

#include "LogManager.h"

namespace {
  uint64_t Unimplemented(FEXCore::Core::InternalThreadState *Thread) {
    LogMan::Msg::A("Unhandled system call");
    return -1;
  }

  uint64_t NopSuccess(FEXCore::Core::InternalThreadState *Thread) {
    return 0;
  }
  uint64_t NopFail(FEXCore::Core::InternalThreadState *Thread) {
    return -1ULL;
  }
}

namespace FEXCore::HLE::x64 {

class x64SyscallHandler final : public FEXCore::SyscallHandler {
public:
  x64SyscallHandler(FEXCore::Context::Context *ctx);

  // In the case that the syscall doesn't hit the optimized path then we still need to go here
  uint64_t HandleSyscall(FEXCore::Core::InternalThreadState *Thread, FEXCore::HLE::SyscallArguments *Args) override;

#ifdef DEBUG_STRACE
  void Strace(FEXCore::HLE::SyscallArguments *Args, uint64_t Ret) override;
#endif

private:
  void RegisterSyscallHandlers();
};

#ifdef DEBUG_STRACE
void x64SyscallHandler::Strace(FEXCore::HLE::SyscallArguments *Args, uint64_t Ret) {
  switch (Args->Argument[0]) {
  case SYSCALL_BRK: LogMan::Msg::D("brk(%p)                               = 0x%lx", Args->Argument[1], Ret); break;
  case SYSCALL_ACCESS:
    LogMan::Msg::D("access(\"%s\", %d) = %ld", reinterpret_cast<char const*>(Args->Argument[1]), Args->Argument[2], Ret);
    break;
  case SYSCALL_PIPE:
    LogMan::Msg::D("pipe({...}) = %ld", Ret);
    break;
  case SYSCALL_SELECT:
    LogMan::Msg::D("select(%ld, 0x%lx, 0x%lx, 0x%lx, 0x%lx) = %ld",
      Args->Argument[1],
      Args->Argument[2],
      Args->Argument[3],
      Args->Argument[4],
      Args->Argument[5],
      Ret);
    break;
  case SYSCALL_SCHED_YIELD:
    LogMan::Msg::D("sched_yield() = %ld",
      Ret);
    break;
  case SYSCALL_MREMAP:
    LogMan::Msg::D("mremap(0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx) = %ld",
      Args->Argument[1],
      Args->Argument[2],
      Args->Argument[3],
      Args->Argument[4],
      Args->Argument[5],
      Ret);
    break;
  case SYSCALL_MINCORE:
    LogMan::Msg::D("mincore(0x%lx, 0x%lx, 0x%lx) = %ld",
      Args->Argument[1],
      Args->Argument[2],
      Args->Argument[3],
      Ret);
    break;
  case SYSCALL_MADVISE:
    LogMan::Msg::D("madvise(0x%lx, 0x%lx, 0x%lx) = %ld",
      Args->Argument[1],
      Args->Argument[2],
      Args->Argument[3],
      Ret);
    break;
  case SYSCALL_SHMGET:
    LogMan::Msg::D("shmget(0x%lx, 0x%lx, 0x%lx) = %ld",
      Args->Argument[1],
      Args->Argument[2],
      Args->Argument[3],
      Ret);
    break;
  case SYSCALL_SHMAT:
    LogMan::Msg::D("shmat(0x%lx, 0x%lx, 0x%lx) = %lx",
      Args->Argument[1],
      Args->Argument[2],
      Args->Argument[3],
      Ret);
    break;
  case SYSCALL_SHMCTL:
    LogMan::Msg::D("shmctl(0x%lx, 0x%lx, 0x%lx) = %ld",
      Args->Argument[1],
      Args->Argument[2],
      Args->Argument[3],
      Ret);
    break;
  case SYSCALL_DUP:
    LogMan::Msg::D("dup(0x%lx) = %ld",
      Args->Argument[1],
      Ret);
    break;
  case SYSCALL_DUP2:
    LogMan::Msg::D("dup2(0x%lx, 0x%lx) = %ld",
      Args->Argument[1],
      Args->Argument[2],
      Ret);
    break;
  case SYSCALL_NANOSLEEP:
    LogMan::Msg::D("nanosleep(0x%ld, 0x%lx) = %ld",
      Args->Argument[1],
      Args->Argument[2],
      Ret);
    break;
  case SYSCALL_GETITIMER:
    LogMan::Msg::D("getitimer(0x%ld, 0x%lx) = %ld",
      Args->Argument[1],
      Args->Argument[2],
      Ret);
    break;
  case SYSCALL_ALARM:
    LogMan::Msg::D("alarm(%ld) = %ld",
      Args->Argument[1],
      Ret);
    break;
  case SYSCALL_SETITIMER:
    LogMan::Msg::D("setitimer(0x%ld, 0x%lx) = %ld",
      Args->Argument[1],
      Args->Argument[2],
      Ret);
    break;
  case SYSCALL_OPENAT:
    LogMan::Msg::D("openat(%ld, \"%s\", %d) = %ld", Args->Argument[1], reinterpret_cast<char const*>(Args->Argument[2]), Args->Argument[3], Ret);
    break;
  case SYSCALL_NEWFSTATAT:
    LogMan::Msg::D("newfstatat(%ld, \"%s\", %p, 0x%lx) = %ld",
      Args->Argument[1],
      reinterpret_cast<char const*>(Args->Argument[2]),
      Args->Argument[3],
      Args->Argument[4],
      Ret);
    break;
  case SYSCALL_READLINKAT:
    LogMan::Msg::D("readlinkat(\"%s\") = %ld", reinterpret_cast<char const*>(Args->Argument[2]), Ret);
    break;
  case SYSCALL_FACCESSAT:
    LogMan::Msg::D("faccessat(\"%s\", %d) = %ld", reinterpret_cast<char const*>(Args->Argument[2]), Args->Argument[3], Ret);
    break;
  case SYSCALL_STAT:
    LogMan::Msg::D("stat(\"%s\", {...}) = %ld", reinterpret_cast<char const*>(Args->Argument[1]), Ret);
    break;
  case SYSCALL_FSTAT:
    LogMan::Msg::D("fstat(%ld, {...}) = %ld", Args->Argument[1], Ret);
    break;
  case SYSCALL_LSTAT:
    LogMan::Msg::D("lstat(\"%s\", {...}) = %ld", reinterpret_cast<char const*>(Args->Argument[1]), Ret);
    break;
  case SYSCALL_POLL:
    LogMan::Msg::D("poll(0x%lx, %ld, %ld) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret);
    break;
  case SYSCALL_MMAP:
    LogMan::Msg::D("mmap(%p, 0x%lx, %ld, %lx, %d, %p) = %p", Args->Argument[1], Args->Argument[2], Args->Argument[3], Args->Argument[4], static_cast<int32_t>(Args->Argument[5]), Args->Argument[6], Ret);
    break;
  case SYSCALL_CLOSE:
    LogMan::Msg::D("close(%ld) = %ld", Args->Argument[1], Ret);
    break;
  case SYSCALL_READ:
    LogMan::Msg::D("read(%ld, {...}, %ld) = %ld", Args->Argument[1], Args->Argument[3], Ret);
    break;
  case SYSCALL_SCHED_SETSCHEDULER:
    LogMan::Msg::D("sched_setscheduler(%ld, %p, %p) = %ld",
      Args->Argument[1],
      Args->Argument[2],
      Args->Argument[3],
      Ret);
    break;
  case SYSCALL_PRCTL:
    LogMan::Msg::D("arch_prctl(%ld, %p, %p, %p, %p) = %ld",
      Args->Argument[1], Args->Argument[2],
      Args->Argument[3], Args->Argument[4],
      Args->Argument[5],
      Ret);
    break;
  case SYSCALL_ARCH_PRCTL:
    LogMan::Msg::D("arch_prctl(0x%lx, %p) = %ld", Args->Argument[1], Args->Argument[2], Ret);
    break;
  case SYSCALL_MPROTECT:
    LogMan::Msg::D("mprotect(%p, %ld, %ld) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret);
    break;
  case SYSCALL_MUNMAP:
    LogMan::Msg::D("munmap(%p, %ld) = %ld", Args->Argument[1], Args->Argument[2], Ret);
    break;
  case SYSCALL_UNAME:
    LogMan::Msg::D("uname ({...}) = %ld", Ret);
    break;
  case SYSCALL_SHMDT:
    LogMan::Msg::D("shmdt(%p)= %ld", Args->Argument[1], Ret);
    break;
  case SYSCALL_FCNTL:
    LogMan::Msg::D("fcntl (%ld, %ld, 0x%lx) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret);
    break;
  case SYSCALL_FTRUNCATE:
    LogMan::Msg::D("ftruncate(%ld, 0x%lx) = %ld", Args->Argument[1], Args->Argument[2], Ret);
    break;
  case SYSCALL_UMASK:
    LogMan::Msg::D("umask(0x%lx) = %ld", Args->Argument[1], Ret);
    break;
  case SYSCALL_GETTIMEOFDAY:
    LogMan::Msg::D("gettimeofday(0x%lx, 0x%lx) = %ld", Args->Argument[1], Args->Argument[2], Ret);
    break;
  case SYSCALL_SYSINFO:
    LogMan::Msg::D("sysinfo(0x%lx) = %ld", Args->Argument[1], Ret);
    break;
  case SYSCALL_GETCWD:
    LogMan::Msg::D("getcwd(\"%s\", %ld) = %ld", reinterpret_cast<char const*>(Args->Argument[1]), Args->Argument[2], Ret);
    break;
  case SYSCALL_CHDIR:
    LogMan::Msg::D("chdir(\"%s\") = %ld", reinterpret_cast<char const*>(Args->Argument[1]), Ret);
    break;
  case SYSCALL_RENAME:
    LogMan::Msg::D("rename(\"%s\", \"%s\") = %ld", reinterpret_cast<char const*>(Args->Argument[1]), reinterpret_cast<char const*>(Args->Argument[2]), Ret);
    break;
  case SYSCALL_MKDIR:
    LogMan::Msg::D("mkdir(\"%s\", 0x%lx) = %ld", reinterpret_cast<char const*>(Args->Argument[1]), Args->Argument[2], Ret);
    break;
  case SYSCALL_RMDIR:
    LogMan::Msg::D("rmdir(\"%s\", 0x%lx) = %ld", reinterpret_cast<char const*>(Args->Argument[1]), Args->Argument[2], Ret);
    break;
  case SYSCALL_LINK:
    LogMan::Msg::D("link(\"%s\", \"%s\") = %ld", reinterpret_cast<char const*>(Args->Argument[1]), reinterpret_cast<char const*>(Args->Argument[2]), Ret);
    break;
  case SYSCALL_UNLINK:
    LogMan::Msg::D("unlink(\"%s\") = %ld", reinterpret_cast<char const*>(Args->Argument[1]), Ret);
    break;
  case SYSCALL_READLINK:
    LogMan::Msg::D("readlink(\"%s\") = %ld", reinterpret_cast<char const*>(Args->Argument[1]), Ret);
    break;
  case SYSCALL_CHMOD:
    LogMan::Msg::D("chmod(\"%s\", 0x%lx) = %ld", reinterpret_cast<char const*>(Args->Argument[1]), Args->Argument[2], Ret);
    break;
  case SYSCALL_EXIT:
    LogMan::Msg::D("exit(0x%lx)", Args->Argument[1]);
    break;
  case SYSCALL_WRITE:
    LogMan::Msg::D("write(%ld, %p, %ld) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret);
    break;
  case SYSCALL_WRITEV:
    LogMan::Msg::D("writev(%ld, %p, %ld) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret);
    break;
  case SYSCALL_EXIT_GROUP:
    LogMan::Msg::D("exit_group(0x%lx)", Args->Argument[1]);
    break;
  case SYSCALL_EPOLL_CTL:
    LogMan::Msg::D("epoll_ctl(%ld, %ld, %ld, %p) = %ld",
      Args->Argument[1],
      Args->Argument[2],
      Args->Argument[3],
      Args->Argument[4],
      Ret);
    break;
  case SYSCALL_GETDENTS64:
    LogMan::Msg::D("getdents(%ld, 0x%lx, %ld) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret);
    break;
  case SYSCALL_FADVISE64:
    LogMan::Msg::D("fadvise64(%ld, 0x%lx, %ld, %lx) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Args->Argument[4], Ret);
    break;
  case SYSCALL_SET_TID_ADDRESS:
    LogMan::Msg::D("set_tid_address(%p) = %ld", Args->Argument[1], Ret);
    break;
  case SYSCALL_PPOLL:
    LogMan::Msg::D("ppoll(%p, %d, %p, %p, %d) = %ld",
      Args->Argument[1],
      Args->Argument[2],
      Args->Argument[3],
      Args->Argument[4],
      Args->Argument[5],
      Ret);
    break;
  case SYSCALL_SET_ROBUST_LIST:
    LogMan::Msg::D("set_robust_list(%p, %ld) = %ld", Args->Argument[1], Args->Argument[2], Ret);
    break;
  case SYSCALL_EPOLL_PWAIT:
    LogMan::Msg::D("epoll_pwait(%ld, %p, %ld, %ld) = %ld",
      Args->Argument[1],
      Args->Argument[2],
      Args->Argument[3],
      Args->Argument[4],
      Ret);
    break;
  case SYSCALL_TIMERFD_CREATE:
    LogMan::Msg::D("timerfd_create(%lx, %lx) = %ld", Args->Argument[1], Args->Argument[2], Ret);
    break;
  case SYSCALL_EVENTFD:
    LogMan::Msg::D("eventfd(%lx, %ld) = %ld", Args->Argument[1], Args->Argument[2], Ret);
    break;
  case SYSCALL_EPOLL_CREATE1:
    LogMan::Msg::D("epoll_create1(%ld) = %ld", Args->Argument[1], Ret);
    break;
  case SYSCALL_PIPE2:
    LogMan::Msg::D("pipe2({...}, %d) = %ld", Args->Argument[2], Ret);
    break;
  case SYSCALL_RT_SIGACTION:
    LogMan::Msg::D("rt_sigaction(%ld, %p, %p) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret);
    break;
  case SYSCALL_RT_SIGPROCMASK:
    LogMan::Msg::D("rt_sigprocmask(%ld, %p, %p, %ld) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Args->Argument[4], Ret);
    break;
  case SYSCALL_PRLIMIT64:
    LogMan::Msg::D("prlimit64(%ld, %ld, %p, %p) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Args->Argument[4], Ret);
    break;
  case SYSCALL_SENDMMSG:
    LogMan::Msg::D("sendmmsg(%ld, 0x%lx, %ld, %ld) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Args->Argument[4], Ret);
    break;
  case SYSCALL_GETPID:
    LogMan::Msg::D("getpid() = %ld", Ret);
    break;
  case SYSCALL_SOCKET:
    LogMan::Msg::D("socket(%ld, %ld, %ld) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret);
    break;
  case SYSCALL_CONNECT:
    LogMan::Msg::D("connect(%ld, 0x%lx, %ld) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret);
    break;
  case SYSCALL_SENDTO:
    LogMan::Msg::D("sendto(%ld, 0x%lx, %ld, %lx, 0x%lx, %ld) = %ld",
        Args->Argument[1],
        Args->Argument[2],
        Args->Argument[3],
        Args->Argument[4],
        Args->Argument[5],
        Args->Argument[6],
        Ret);
    break;
  case SYSCALL_RECVFROM:
    LogMan::Msg::D("recvfrom(%ld, 0x%lx, %ld, %ld, 0x%lx, 0x%lx) = %ld",
        Args->Argument[1],
        Args->Argument[2],
        Args->Argument[3],
        Args->Argument[4],
        Args->Argument[5],
        Args->Argument[6],
        Ret);
    break;
  case SYSCALL_SENDMSG:
    LogMan::Msg::D("sendmsg(%ld, 0x%lx, %ld) = %ld",
        Args->Argument[1],
        Args->Argument[2],
        Args->Argument[3],
        Ret);
    break;
  case SYSCALL_RECVMSG:
    LogMan::Msg::D("recvmsg(%ld, 0x%lx, %ld) = %ld",
        Args->Argument[1],
        Args->Argument[2],
        Args->Argument[3],
        Ret);
    break;
  case SYSCALL_SHUTDOWN:
    LogMan::Msg::D("shutdown(%ld, %ld) = %ld", Args->Argument[1], Args->Argument[2], Ret);
    break;
  case SYSCALL_BIND:
    LogMan::Msg::D("bind(%ld, %p, %ld) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret);
    break;
  case SYSCALL_LISTEN:
    LogMan::Msg::D("listen(%ld, %ld) = %ld", Args->Argument[1], Args->Argument[2], Ret);
    break;
  case SYSCALL_GETSOCKNAME:
    LogMan::Msg::D("getsockname(%ld, 0x%lx, 0x%lx) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret);
    break;
  case SYSCALL_GETPEERNAME:
    LogMan::Msg::D("getpeername(%ld, 0x%lx, 0x%lx) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret);
    break;
  case SYSCALL_SETSOCKOPT:
    LogMan::Msg::D("setsockopt(%ld, %ld, %ld, 0x%lx, %ld) = %ld",
        Args->Argument[1],
        Args->Argument[2],
        Args->Argument[3],
        Args->Argument[4],
        Args->Argument[5],
        Ret);
    break;
  case SYSCALL_GETSOCKOPT:
    LogMan::Msg::D("getsockopt(%ld, %ld, %ld, 0x%lx, 0x%lx) = %ld",
        Args->Argument[1],
        Args->Argument[2],
        Args->Argument[3],
        Args->Argument[4],
        Args->Argument[5],
        Ret);
    break;
  case SYSCALL_CLONE:
    LogMan::Msg::I("clone(%lx,\n\t%lx,\n\t%lx,\n\t%lx,\n\t%lx,\n\t%lx,\n\t%lx)",
        Args->Argument[1],
        Args->Argument[2],
        Args->Argument[3],
        Args->Argument[4],
        Args->Argument[5],
        Args->Argument[6]);
    break;
  case SYSCALL_EXECVE:
    LogMan::Msg::I("execve('%s', %p, %p) = %ld",
        reinterpret_cast<char const *>(Args->Argument[1]),
        Args->Argument[2],
        Args->Argument[3],
        Ret);
    break;
  case SYSCALL_GETUID:
    LogMan::Msg::D("getuid() = %ld", Ret);
    break;
  case SYSCALL_SYSLOG:
    LogMan::Msg::D("syslog(%ld, %p, 0x%lx) = %ld",
      Args->Argument[1],
      Args->Argument[2],
      Args->Argument[3],
      Ret);
    break;
  case SYSCALL_GETGID:
    LogMan::Msg::D("getgid() = %ld", Ret);
    break;
  case SYSCALL_SETUID:
    LogMan::Msg::D("setuid(%ld) = %ld", Args->Argument[1], Ret);
    break;
  case SYSCALL_GETEUID:
    LogMan::Msg::D("geteuid() = %ld", Ret);
    break;
  case SYSCALL_GETEGID:
    LogMan::Msg::D("getegid() = %ld", Ret);
    break;
  case SYSCALL_SETREGID:
    LogMan::Msg::D("setregid(%ld, %ld) = %ld", Args->Argument[1], Args->Argument[2], Ret);
    break;
  case SYSCALL_SETRESUID:
    LogMan::Msg::D("setresuid(%ld, %ld, %ld) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret);
    break;
  case SYSCALL_GETRESUID:
    LogMan::Msg::D("getresuid(%ld, %ld, %ld) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret);
    break;
  case SYSCALL_GETRESGID:
    LogMan::Msg::D("getresgid(%ld, %ld, %ld) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret);
    break;
  case SYSCALL_SETRESGID:
    LogMan::Msg::D("setresgid(%ld, %ld, %ld) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret);
    break;
  case SYSCALL_SIGALTSTACK:
    LogMan::Msg::D("sigaltstack(%lx, %lx) = %ld", Args->Argument[1], Args->Argument[2], Ret);
    break;
  case SYSCALL_MKNOD:
    LogMan::Msg::D("mknod(\"%s\", %ld, %ld) = %ld", reinterpret_cast<char const*>(Args->Argument[1]),
      Args->Argument[2], Args->Argument[3],
      Ret);
    break;
  case SYSCALL_STATFS:
    LogMan::Msg::D("statfs(\"%s\", {...}) = %ld", reinterpret_cast<char const*>(Args->Argument[1]), Ret);
    break;
  case SYSCALL_FSTATFS:
    LogMan::Msg::D("fstatfs(%ld, {...}) = %ld", Args->Argument[1], Ret);
    break;
  case SYSCALL_GETPRIORITY:
    LogMan::Msg::D("getpriority(%ld, %ld) = %ld", Args->Argument[1], Args->Argument[2], Ret);
    break;
  case SYSCALL_SETPRIORITY:
    LogMan::Msg::D("setpriority(%ld, %ld, %ld) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret);
    break;
  case SYSCALL_GETTID:
    LogMan::Msg::D("gettid() = %ld", Ret);
    break;
  case SYSCALL_TGKILL:
    LogMan::Msg::D("tgkill(%ld, %ld) = %ld", Args->Argument[1], Args->Argument[2], Ret);
    break;
  case SYSCALL_GET_MEMPOLICY:
    LogMan::Msg::D("get_mempolicy(%p, %p, %ld, %p, 0x%lx) = %ld",
      Args->Argument[1],
      Args->Argument[2],
      Args->Argument[3],
      Args->Argument[4],
      Args->Argument[5],
      Ret);
    break;
  case SYSCALL_LSEEK:
    LogMan::Msg::D("lseek(%ld, %ld, %ld) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret);
    break;
  case SYSCALL_IOCTL:
    LogMan::Msg::D("ioctl(%ld, 0x%lx, %p) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret);
    break;
  case SYSCALL_PREAD64:
    LogMan::Msg::D("pread64(%ld, 0x%lx, %ld, %ld) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Args->Argument[4], Ret);
    break;
  case SYSCALL_TIME:
    LogMan::Msg::D("time(%p) = %ld", Args->Argument[1], Ret);
    break;
  case SYSCALL_FUTEX:
    LogMan::Msg::D("futex(%p, %ld, %ld, 0x%lx, 0x%lx, %ld) = %ld",
      Args->Argument[1],
      Args->Argument[2],
      Args->Argument[3],
      Args->Argument[4],
      Args->Argument[5],
      Args->Argument[6],
      Ret);
    break;
  case SYSCALL_SCHED_SETAFFINITY:
    LogMan::Msg::D("sched_setaffinity(%ld, %ld, 0x%lx) = %ld",
      Args->Argument[1],
      Args->Argument[2],
      Args->Argument[3],
      Ret);
    break;
  case SYSCALL_SCHED_GETAFFINITY:
    LogMan::Msg::D("sched_getaffinity(%ld, %ld, 0x%lx) = %ld",
      Args->Argument[1],
      Args->Argument[2],
      Args->Argument[3],
      Ret);
    break;
  case SYSCALL_CLOCK_GETTIME:
    LogMan::Msg::D("clock_gettime(%ld, %p) = %ld", Args->Argument[1], Args->Argument[2], Ret);
    break;
  case SYSCALL_CLOCK_GETRES:
    LogMan::Msg::D("clock_getres(%ld, %p) = %ld", Args->Argument[1], Args->Argument[2], Ret);
    break;
  case SYSCALL_GETRANDOM:
    LogMan::Msg::D("getrandom(0x%lx, 0x%lx, 0x%lx) = %ld",
        Args->Argument[1],
        Args->Argument[2],
        Args->Argument[3],
        Ret);
    break;
  case SYSCALL_MEMFD_CREATE:
    LogMan::Msg::D("memfd_create(\"%s\", 0x%lx) = %ld",
        reinterpret_cast<char const*>(Args->Argument[1]),
        Args->Argument[2],
        Ret);
    break;
  case SYSCALL_STATX:
    LogMan::Msg::D("statx(%ld, \"%s\", 0x%lx, 0x%lx, {...}) = %ld",
      Args->Argument[1],
      reinterpret_cast<char const*>(Args->Argument[2]),
      Args->Argument[3], Args->Argument[4],
      Ret);
    break;
  default: LogMan::Msg::D("Unknown strace: %d", Args->Argument[0]);
  }
}
#endif

x64SyscallHandler::x64SyscallHandler(FEXCore::Context::Context *ctx)
  : SyscallHandler {ctx} {
  RegisterSyscallHandlers();
}

void x64SyscallHandler::RegisterSyscallHandlers() {
  Definitions.resize(FEXCore::HLE::x64::SYSCALL_MAX);
  auto cvt = [](auto in) {
    union {
      decltype(in) val;
      void *raw;
    } raw;
    raw.val = in;
    return raw.raw;
  };

  // Clear all definitions
  for (auto &Def : Definitions) {
    Def.NumArgs = 0;
    Def.Ptr = cvt(&Unimplemented);
  }

  const std::vector<std::tuple<uint16_t, void*, uint8_t>> Syscalls = {
    {SYSCALL_READ,               cvt(&FEXCore::HLE::Read),              3},
    {SYSCALL_WRITE,              cvt(&FEXCore::HLE::Write),             3},
    {SYSCALL_OPEN,               cvt(&FEXCore::HLE::Open),              3},
    {SYSCALL_CLOSE,              cvt(&FEXCore::HLE::Close),             1},
    {SYSCALL_STAT,               cvt(&FEXCore::HLE::Stat),              2},
    {SYSCALL_FSTAT,              cvt(&FEXCore::HLE::Fstat),             2},
    {SYSCALL_LSTAT,              cvt(&FEXCore::HLE::Lstat),             2},
    {SYSCALL_POLL,               cvt(&FEXCore::HLE::Poll),              3},
    {SYSCALL_LSEEK,              cvt(&FEXCore::HLE::Lseek),             3},
    {SYSCALL_MMAP,               cvt(&FEXCore::HLE::Mmap),              6},
    {SYSCALL_MPROTECT,           cvt(&FEXCore::HLE::Mprotect),          3},
    {SYSCALL_MUNMAP,             cvt(&FEXCore::HLE::Munmap),            2},
    {SYSCALL_BRK,                cvt(&FEXCore::HLE::Brk),               1},
    {SYSCALL_RT_SIGACTION,       cvt(&NopSuccess),                      0},
    {SYSCALL_RT_SIGPROCMASK,     cvt(&NopSuccess),                      0},
    {SYSCALL_IOCTL,              cvt(&FEXCore::HLE::Ioctl),             3},
    {SYSCALL_PREAD64,            cvt(&FEXCore::HLE::PRead64),           4},
    {SYSCALL_PWRITE64,           cvt(&FEXCore::HLE::PWrite64),          4},
    {SYSCALL_READV,              cvt(&FEXCore::HLE::Readv),             3},
    {SYSCALL_WRITEV,             cvt(&FEXCore::HLE::Writev),            3},
    {SYSCALL_ACCESS,             cvt(&FEXCore::HLE::Access),            2},
    {SYSCALL_PIPE,               cvt(&FEXCore::HLE::Pipe),              1},
    {SYSCALL_SELECT,             cvt(&FEXCore::HLE::Select),            5},
    {SYSCALL_SCHED_YIELD,        cvt(&FEXCore::HLE::Sched_Yield),       0},
    {SYSCALL_MREMAP,             cvt(&FEXCore::HLE::Mremap),            5},
    {SYSCALL_MINCORE,            cvt(&FEXCore::HLE::Mincore),           3},
    {SYSCALL_MADVISE,            cvt(&FEXCore::HLE::Madvise),           3},
    {SYSCALL_SHMGET,             cvt(&FEXCore::HLE::Shmget),            3},
    {SYSCALL_SHMAT,              cvt(&FEXCore::HLE::Shmat),             3},
    {SYSCALL_SHMCTL,             cvt(&FEXCore::HLE::Shmctl),            3},
    {SYSCALL_DUP,                cvt(&FEXCore::HLE::Dup),               1},
    {SYSCALL_DUP2,               cvt(&FEXCore::HLE::Dup2),              2},
    {SYSCALL_PAUSE,              cvt(&NopFail),                         0},
    {SYSCALL_NANOSLEEP,          cvt(&FEXCore::HLE::Nanosleep),         2},
    {SYSCALL_GETITIMER,          cvt(&NopSuccess),                      2},
    {SYSCALL_ALARM,              cvt(&NopSuccess),                      1},
    {SYSCALL_SETITIMER,          cvt(&NopSuccess),                      3},
    {SYSCALL_GETPID,             cvt(&FEXCore::HLE::Getpid),            0},
    {SYSCALL_SOCKET,             cvt(&FEXCore::HLE::Socket),            3},
    {SYSCALL_CONNECT,            cvt(&FEXCore::HLE::Connect),           3},
    {SYSCALL_SENDTO,             cvt(&FEXCore::HLE::Sendto),            6},
    {SYSCALL_RECVFROM,           cvt(&FEXCore::HLE::Recvfrom),          6},
    {SYSCALL_SENDMSG,            cvt(&FEXCore::HLE::Sendmsg),           3},
    {SYSCALL_RECVMSG,            cvt(&FEXCore::HLE::Recvmsg),           3},
    {SYSCALL_SHUTDOWN,           cvt(&FEXCore::HLE::Shutdown),          2},
    {SYSCALL_BIND,               cvt(&FEXCore::HLE::Bind),              3},
    {SYSCALL_LISTEN,             cvt(&FEXCore::HLE::Listen),            2},
    {SYSCALL_GETSOCKNAME,        cvt(&FEXCore::HLE::GetSockName),       3},
    {SYSCALL_GETPEERNAME,        cvt(&FEXCore::HLE::GetPeerName),       3},
    {SYSCALL_SETSOCKOPT,         cvt(&FEXCore::HLE::SetSockOpt),        5},
    {SYSCALL_GETSOCKOPT,         cvt(&FEXCore::HLE::GetSockOpt),        5},
    {SYSCALL_CLONE,              cvt(&FEXCore::HLE::Clone),             5},
    {SYSCALL_EXECVE,             cvt(&FEXCore::HLE::Execve),            3},
    {SYSCALL_EXIT,               cvt(&FEXCore::HLE::Exit),              1},
    {SYSCALL_WAIT4,              cvt(&FEXCore::HLE::Wait4),             4},
    {SYSCALL_UNAME,              cvt(&FEXCore::HLE::Uname),             1},
    {SYSCALL_SHMDT,              cvt(&FEXCore::HLE::Shmdt),             1},
    {SYSCALL_FCNTL,              cvt(&FEXCore::HLE::Fcntl),             3},
    {SYSCALL_FTRUNCATE,          cvt(&FEXCore::HLE::Ftruncate),         2},
    {SYSCALL_GETCWD,             cvt(&FEXCore::HLE::Getcwd),            2},
    {SYSCALL_CHDIR,              cvt(&FEXCore::HLE::Chdir),             1},
    {SYSCALL_RENAME,             cvt(&FEXCore::HLE::Rename),            2},
    {SYSCALL_MKDIR,              cvt(&FEXCore::HLE::Mkdir),             2},
    {SYSCALL_RMDIR,              cvt(&FEXCore::HLE::Rmdir),             1},
    {SYSCALL_LINK,               cvt(&FEXCore::HLE::Link),              2},
    {SYSCALL_UNLINK,             cvt(&FEXCore::HLE::Unlink),            1},
    {SYSCALL_READLINK,           cvt(&FEXCore::HLE::Readlink),          3},
    {SYSCALL_CHMOD,              cvt(&FEXCore::HLE::Chmod),             2},
    {SYSCALL_UMASK,              cvt(&FEXCore::HLE::Umask),             1},
    {SYSCALL_GETTIMEOFDAY,       cvt(&FEXCore::HLE::Gettimeofday),      2},
    {SYSCALL_SYSINFO,            cvt(&FEXCore::HLE::Sysinfo),           1},
    {SYSCALL_GETUID,             cvt(&FEXCore::HLE::Getuid),            0},
    {SYSCALL_SYSLOG,             cvt(&FEXCore::HLE::Syslog),            6}, // XXX: Verify ABI on vaarg
    {SYSCALL_GETGID,             cvt(&FEXCore::HLE::Getgid),            0},
    {SYSCALL_SETUID,             cvt(&FEXCore::HLE::Setuid),            1},
    {SYSCALL_GETEUID,            cvt(&FEXCore::HLE::Geteuid),           0},
    {SYSCALL_GETEGID,            cvt(&FEXCore::HLE::Getegid),           0},
    {SYSCALL_SETREGID,           cvt(&FEXCore::HLE::Setregid),          2},
    {SYSCALL_SETRESUID,          cvt(&FEXCore::HLE::Setresuid),         3},
    {SYSCALL_GETRESUID,          cvt(&FEXCore::HLE::Getresuid),         3},
    {SYSCALL_SETRESGID,          cvt(&FEXCore::HLE::Setresgid),         3},
    {SYSCALL_GETRESGID,          cvt(&FEXCore::HLE::Getresgid),         3},
    {SYSCALL_SIGALTSTACK,        cvt(&NopSuccess),                      2},
    {SYSCALL_MKNOD,              cvt(&FEXCore::HLE::Mknod),             3},
    {SYSCALL_STATFS,             cvt(&FEXCore::HLE::Statfs),            2},
    {SYSCALL_FSTATFS,            cvt(&FEXCore::HLE::FStatfs),           2},
    {SYSCALL_GETPRIORITY,        cvt(&FEXCore::HLE::Getpriority),       2},
    {SYSCALL_SETPRIORITY,        cvt(&FEXCore::HLE::Setpriority),       3},
    {SYSCALL_SCHED_SETSCHEDULER, cvt(&NopSuccess),                      3},
    {SYSCALL_PRCTL,              cvt(&FEXCore::HLE::Prctl),             5},
    {SYSCALL_ARCH_PRCTL,         cvt(&FEXCore::HLE::Arch_Prctl),        2},
    {SYSCALL_GETTID,             cvt(&FEXCore::HLE::Gettid),            0},
    {SYSCALL_TIME,               cvt(&FEXCore::HLE::Time),              1},
    {SYSCALL_FUTEX,              cvt(&FEXCore::HLE::Futex),             6},
    {SYSCALL_SCHED_SETAFFINITY,  cvt(&FEXCore::HLE::Sched_Setaffinity), 3},
    {SYSCALL_SCHED_GETAFFINITY,  cvt(&FEXCore::HLE::Sched_Getaffinity), 3},
    {SYSCALL_GETDENTS64,         cvt(&FEXCore::HLE::Getdents64),        3},
    {SYSCALL_FADVISE64,          cvt(&FEXCore::HLE::Fadvise64),         4},
    {SYSCALL_TIMER_CREATE,       cvt(&FEXCore::HLE::Timer_Create),      3},
    {SYSCALL_TIMER_SETTIME,      cvt(&FEXCore::HLE::Timer_Settime),     4},
    {SYSCALL_TIMER_GETTIME,      cvt(&FEXCore::HLE::Timer_Gettime),     2},
    {SYSCALL_TIMER_GETOVERRUN,   cvt(&FEXCore::HLE::Timer_Getoverrun),  1},
    {SYSCALL_TIMER_DELETE,       cvt(&FEXCore::HLE::Timer_Delete),      1},
    {SYSCALL_SET_TID_ADDRESS,    cvt(&FEXCore::HLE::Set_tid_address),   1},
    {SYSCALL_CLOCK_GETTIME,      cvt(&FEXCore::HLE::Clock_gettime),     2},
    {SYSCALL_CLOCK_GETRES,       cvt(&FEXCore::HLE::Clock_getres),      2},
    {SYSCALL_EXIT_GROUP,         cvt(&FEXCore::HLE::Exit_group),        1},
    {SYSCALL_EPOLL_CTL,          cvt(&FEXCore::HLE::EPoll_Ctl),         4},
    {SYSCALL_TGKILL,             cvt(&NopSuccess),                      3},
    {SYSCALL_GET_MEMPOLICY,      cvt(&FEXCore::HLE::Get_mempolicy),     5},
    {SYSCALL_INOTIFY_INIT,       cvt(&FEXCore::HLE::Inotify_init),      0},
    {SYSCALL_INOTIFY_ADD_WATCH,  cvt(&FEXCore::HLE::Inotify_add_watch), 3},
    {SYSCALL_INOTIFY_RM_WATCH,   cvt(&FEXCore::HLE::Inotify_rm_watch),  2},
    {SYSCALL_OPENAT,             cvt(&FEXCore::HLE::Openat),            4},
    {SYSCALL_NEWFSTATAT,         cvt(&FEXCore::HLE::NewFStatat),        4},
    {SYSCALL_READLINKAT,         cvt(&FEXCore::HLE::Readlinkat),        4},
    {SYSCALL_FACCESSAT,          cvt(&FEXCore::HLE::FAccessat),         4},
    {SYSCALL_PPOLL,              cvt(&FEXCore::HLE::Ppoll),             5},
    {SYSCALL_SET_ROBUST_LIST,    cvt(&FEXCore::HLE::Set_robust_list),   2},
    {SYSCALL_EPOLL_PWAIT,        cvt(&FEXCore::HLE::EPoll_Pwait),       5},
    {SYSCALL_TIMERFD_CREATE,     cvt(&FEXCore::HLE::Timerfd_Create),    2},
    {SYSCALL_EVENTFD,            cvt(&FEXCore::HLE::Eventfd),           2},
    {SYSCALL_EPOLL_CREATE1,      cvt(&FEXCore::HLE::EPoll_Create1),     1},
    {SYSCALL_PIPE2,              cvt(&FEXCore::HLE::Pipe2),             2},
    {SYSCALL_PRLIMIT64,          cvt(&FEXCore::HLE::Prlimit64),         4},
    {SYSCALL_SENDMMSG,           cvt(&FEXCore::HLE::Sendmmsg),          4},
    {SYSCALL_GETRANDOM,          cvt(&FEXCore::HLE::Getrandom),         3},
    {SYSCALL_MEMFD_CREATE,       cvt(&FEXCore::HLE::Memfd_Create),      2},
    {SYSCALL_STATX,              cvt(&FEXCore::HLE::Statx),             5},
  };

  // Set all the new definitions
  for (auto &Syscall : Syscalls) {
    auto &Def = Definitions.at(std::get<0>(Syscall));
    Def.Ptr = std::get<1>(Syscall);
    Def.NumArgs = std::get<2>(Syscall);
  }
}

uint64_t x64SyscallHandler::HandleSyscall(FEXCore::Core::InternalThreadState *Thread, FEXCore::HLE::SyscallArguments *Args) {
  auto &Def = Definitions[Args->Argument[0]];
  switch (Def.NumArgs) {
  case 0: return std::invoke(Def.Ptr0, Thread);
  case 1: return std::invoke(Def.Ptr1, Thread, Args->Argument[1]);
  case 2: return std::invoke(Def.Ptr2, Thread, Args->Argument[1], Args->Argument[2]);
  case 3: return std::invoke(Def.Ptr3, Thread, Args->Argument[1], Args->Argument[2], Args->Argument[3]);
  case 4: return std::invoke(Def.Ptr4, Thread, Args->Argument[1], Args->Argument[2], Args->Argument[3], Args->Argument[4]);
  case 5: return std::invoke(Def.Ptr5, Thread, Args->Argument[1], Args->Argument[2], Args->Argument[3], Args->Argument[4], Args->Argument[5]);
  case 6: return std::invoke(Def.Ptr6, Thread, Args->Argument[1], Args->Argument[2], Args->Argument[3], Args->Argument[4], Args->Argument[5], Args->Argument[6]);
  default: break;
  }

  LogMan::Msg::A("Unhandled syscall");
  return -1;
}

FEXCore::SyscallHandler *CreateHandler(FEXCore::Context::Context *ctx) {
  return new x64SyscallHandler(ctx);
}

}
