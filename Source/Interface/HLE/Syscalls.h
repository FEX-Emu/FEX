#pragma once

#include "Interface/HLE/FileManagement.h"
#include <FEXCore/HLE/SyscallHandler.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <unordered_map>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore {

///< Enum containing all support x86-64 linux syscalls that we support
enum Syscalls {
  SYSCALL_READ            = 0,   ///< __NR_read
  SYSCALL_WRITE           = 1,   ///< __NR_write
  SYSCALL_OPEN            = 2,   ///< __NR_open
  SYSCALL_CLOSE           = 3,   ///< __NR_close
  SYSCALL_STAT            = 4,   ///< __NR_stat
  SYSCALL_FSTAT           = 5,   ///< __NR_fstat
  SYSCALL_LSTAT           = 6,   ///< __NR_lstat
  SYSCALL_POLL            = 7,   ///< __NR_poll
  SYSCALL_LSEEK           = 8,   ///< __NR_lseek
  SYSCALL_MMAP            = 9,   ///< __NR_mmap
  SYSCALL_MPROTECT        = 10,  ///< __NR_mprotect
  SYSCALL_MUNMAP          = 11,  ///< __NR_munmap
  SYSCALL_BRK             = 12,  ///< __NR_brk
  SYSCALL_RT_SIGACTION    = 13,  ///< __NR_rt_sigaction
  SYSCALL_RT_SIGPROCMASK  = 14,  ///< __NR_rt_sigprocmask
  SYSCALL_IOCTL           = 16,  ///< __NR_ioctl
  SYSCALL_PREAD64         = 17,  ///< __NR_pread64
  SYSCALL_WRITEV          = 20,  ///< __NR_writev
  SYSCALL_ACCESS          = 21,  ///< __NR_access
  SYSCALL_PIPE            = 22,  ///< __NR_pipe
  SYSCALL_SELECT          = 23,  ///< __NR_select
  SYSCALL_SCHED_YIELD     = 24,  ///< __NR_sched_yield
  SYSCALL_MINCORE         = 27,  ///< __NR_mincore
  SYSCALL_SHMGET          = 29,  ///< __NR_shmget
  SYSCALL_SHMAT           = 30,  ///< __NR_shmat
  SYSCALL_SHMCTL          = 31,  ///< __NR_shmctl
  SYSCALL_DUP2            = 33,  ///< __NR_dup2
  SYSCALL_NANOSLEEP       = 35,  ///< __NR_nanosleep
  SYSCALL_GETITIMER       = 36,  ///< __NR_getitimer
  SYSCALL_ALARM           = 37,  ///< __NR_alarm
  SYSCALL_SETITIMER       = 38,  ///< __NR_setitimer
  SYSCALL_GETPID          = 39,  ///< __NR_getpid
  SYSCALL_SOCKET          = 41,  ///< __NR_socket
  SYSCALL_CONNECT         = 42,  ///< __NR_connect
  SYSCALL_RECVFROM        = 45,  ///< __NR_recvfrom
  SYSCALL_SENDMSG         = 46,  ///< __NR_sendmsg
  SYSCALL_RECVMSG         = 47,  ///< __NR_recvmsg
  SYSCALL_SHUTDOWN        = 48,  ///< __NR_shutdown
  SYSCALL_GETSOCKNAME     = 51,  ///< __NR_getsockname
  SYSCALL_GETPEERNAME     = 52,  ///< __NR_getpeername
  SYSCALL_SETSOCKOPT      = 54,  ///< __NR_setsockopt
  SYSCALL_GETSOCKOPT      = 55,  ///< __NR_getsockopt
  SYSCALL_CLONE           = 56,  ///< __NR_clone
  SYSCALL_EXIT            = 60,  ///< __NR_exit
  SYSCALL_WAIT4           = 61,  ///< __NR_wait4
  SYSCALL_UNAME           = 63,  ///< __NR_uname
  SYSCALL_SHMDT           = 67,  ///< __NR_shmdt
  SYSCALL_FCNTL           = 72,  ///< __NR_fcntl
  SYSCALL_GETCWD          = 79,  ///< __NR_getcwd
  SYSCALL_CHDIR           = 80,  ///< __NR_chdir
  SYSCALL_MKDIR           = 83,  ///< __NR_mkdir
  SYSCALL_RMDIR           = 84,  ///< __NR_rmdir
  SYSCALL_UNLINK          = 87,  ///< __NR_unlink
  SYSCALL_READLINK        = 89,  ///< __NR_readlink
  SYSCALL_UMASK           = 95,  ///< __NR_umask
  SYSCALL_GETTIMEOFDAY    = 96,  ///< __NR_gettimeofday
  SYSCALL_SYSINFO         = 99,  ///< __NR_sysinfo
  SYSCALL_GETUID          = 102, ///< __NR_getuid
  SYSCALL_GETGID          = 104, ///< __NR_getgid
  SYSCALL_SETUID          = 105, ///< __NR_setuid
  SYSCALL_GETEUID         = 107, ///< __NR_geteuid
  SYSCALL_GETEGID         = 108, ///< __NR_getegid
  SYSCALL_SETREGID        = 114, ///< __NR_setregid
  SYSCALL_SETRESUID       = 117, ///< __NR_setresuid
  SYSCALL_SETRESGID       = 119, ///< __NR_setresgid
  SYSCALL_SIGALTSTACK     = 131, ///< __NR_sigaltstack
  SYSCALL_MKNOD           = 133, ///< __NR_mknod
  SYSCALL_STATFS          = 137, ///< __NR_statfs
  SYSCALL_PRCTL           = 157, ///< __NR_prctl
  SYSCALL_ARCH_PRCTL      = 158, ///< __NR_arch_prctl
  SYSCALL_GETTID          = 186, ///< __NR_gettid
  SYSCALL_TIME            = 201, ///< __NR_time
  SYSCALL_FUTEX           = 202, ///< __NR_futex
  SYSCALL_SCHED_GETAFFINITY = 204, ///< __NR_sched_getaffinity
  SYSCALL_GETDENTS64      = 217, ///< __NR_getdents64
  SYSCALL_SET_TID_ADDRESS = 218, ///< __NR_set_tid_address
  SYSCALL_CLOCK_GETTIME   = 228, ///< __NR_clock_gettime
  SYSCALL_CLOCK_GETRES    = 229, ///< __NR_clock_getres
  SYSCALL_EXIT_GROUP      = 231, ///< __NR_exit_group
  SYSCALL_TGKILL          = 234, ///< __NR_tgkill
  SYSCALL_OPENAT          = 257, ///< __NR_openat
  SYSCALL_READLINKAT      = 267, ///< __NR_readlinkat
  SYSCALL_FACCESSAT       = 269, ///< __NR_faccessat
  SYSCALL_SET_ROBUST_LIST = 273, ///< __NR_set_robust_list
  SYSCALL_TIMERFD_CREATE  = 283, ///< __NR_timerfd_create
  SYSCALL_EVENTFD         = 290, ///< __NR_eventfd
  SYSCALL_EPOLL_CREATE1   = 291, ///< __NR_epoll_create1
  SYSCALL_PIPE2           = 293, ///< __NR_pipe2
  SYSCALL_PRLIMIT64       = 302, ///< __NR_prlimit64
  SYSCALL_SENDMMSG        = 307, ///< __NR_sendmmsg
  SYSCALL_GETRANDOM       = 318, ///< __NR_getrandom
  SYSCALL_STATX           = 332, ///< __NR_statx
};

struct Futex {
  std::mutex Mutex;
  std::condition_variable cv;
  std::atomic<uint32_t> *Addr;
  std::atomic<uint32_t> Waiters{};
  uint32_t Val;
};

// #define DEBUG_STRACE
class SyscallHandler final {
public:
  SyscallHandler(FEXCore::Context::Context *ctx);
  uint64_t HandleSyscall(FEXCore::Core::InternalThreadState *Thread, FEXCore::HLE::SyscallArguments *Args);

  // XXX: This leaks memory.
  // Need to know when to delete futexes
  void EmplaceFutex(uint64_t Addr, Futex *futex) {
    std::scoped_lock<std::mutex> lk(FutexMutex);
    Futexes[Addr] = futex;
  }

  Futex *GetFutex(uint64_t Addr) {
    std::scoped_lock<std::mutex> lk (FutexMutex);
    auto it = Futexes.find(Addr);
    if (it == Futexes.end()) return nullptr;
    return it->second;
  }

  void RemoveFutex(uint64_t Addr) {
    std::scoped_lock<std::mutex> lk (FutexMutex);
    Futexes.erase(Addr);
  }

  void DefaultProgramBreak(FEXCore::Core::InternalThreadState *Thread, uint64_t Addr);

  void SetFilename(std::string const &File) { FM.SetFilename(File); }
  std::string const & GetFilename() const { return FM.GetFilename(); }

private:
  FEXCore::Context::Context *CTX;
  bool const &UnifiedMemory;
  FileManager FM;

  // Futex management
  std::unordered_map<uint64_t, Futex*> Futexes;
  std::mutex FutexMutex;
  std::mutex SyscallMutex;

  // BRK management
  uint64_t DataSpace {};
  uint64_t DataSpaceSize {};
  uint64_t DefaultProgramBreakAddress {};

  // MMap management
  uint64_t LastMMAP = 0xd000'0000;

#ifdef DEBUG_STRACE
  void Strace(FEXCore::HLE::SyscallArguments *Args, uint64_t Ret);
#endif
  template<typename T>
  T GetPointer(uint64_t Offset) {
    return reinterpret_cast<T>(GetPointer(Offset));
  }

  void *GetPointer(uint64_t Offset);
  void *GetPointerSizeCheck(uint64_t Offset, uint64_t Size);
};

}
