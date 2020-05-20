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
class SyscallHandler;
}

namespace FEXCore::HLE::x64 {

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
  SYSCALL_PWRITE64        = 18,  ///< __NR_pwrite64
  SYSCALL_READV           = 19,  ///< __NR_readv
  SYSCALL_WRITEV          = 20,  ///< __NR_writev
  SYSCALL_ACCESS          = 21,  ///< __NR_access
  SYSCALL_PIPE            = 22,  ///< __NR_pipe
  SYSCALL_SELECT          = 23,  ///< __NR_select
  SYSCALL_SCHED_YIELD     = 24,  ///< __NR_sched_yield
  SYSCALL_MREMAP          = 25,  ///< __NR_mremap
  SYSCALL_MSYNC           = 26,  ///< __NR_msync
  SYSCALL_MINCORE         = 27,  ///< __NR_mincore
  SYSCALL_MADVISE         = 28,  ///< __NR_madvise
  SYSCALL_SHMGET          = 29,  ///< __NR_shmget
  SYSCALL_SHMAT           = 30,  ///< __NR_shmat
  SYSCALL_SHMCTL          = 31,  ///< __NR_shmctl
  SYSCALL_DUP             = 32,  ///< __NR_dup
  SYSCALL_DUP2            = 33,  ///< __NR_dup2
  SYSCALL_PAUSE           = 34,  ///< __NR_pause
  SYSCALL_NANOSLEEP       = 35,  ///< __NR_nanosleep
  SYSCALL_GETITIMER       = 36,  ///< __NR_getitimer
  SYSCALL_ALARM           = 37,  ///< __NR_alarm
  SYSCALL_SETITIMER       = 38,  ///< __NR_setitimer
  SYSCALL_GETPID          = 39,  ///< __NR_getpid
  SYSCALL_SOCKET          = 41,  ///< __NR_socket
  SYSCALL_CONNECT         = 42,  ///< __NR_connect
  SYSCALL_SENDTO          = 44,  ///< __NR_sendto
  SYSCALL_RECVFROM        = 45,  ///< __NR_recvfrom
  SYSCALL_SENDMSG         = 46,  ///< __NR_sendmsg
  SYSCALL_RECVMSG         = 47,  ///< __NR_recvmsg
  SYSCALL_SHUTDOWN        = 48,  ///< __NR_shutdown
  SYSCALL_BIND            = 49,  ///< __NR_bind
  SYSCALL_LISTEN          = 50,  ///< __NR_listen
  SYSCALL_GETSOCKNAME     = 51,  ///< __NR_getsockname
  SYSCALL_GETPEERNAME     = 52,  ///< __NR_getpeername
  SYSCALL_SOCKETPAIR      = 53,  ///< __NR_socketpair
  SYSCALL_SETSOCKOPT      = 54,  ///< __NR_setsockopt
  SYSCALL_GETSOCKOPT      = 55,  ///< __NR_getsockopt
  SYSCALL_CLONE           = 56,  ///< __NR_clone
  SYSCALL_EXECVE          = 59,  ///< __NR_execve
  SYSCALL_EXIT            = 60,  ///< __NR_exit
  SYSCALL_WAIT4           = 61,  ///< __NR_wait4
  SYSCALL_KILL            = 62,  ///< __NR_kill
  SYSCALL_UNAME           = 63,  ///< __NR_uname
  SYSCALL_SEMGET          = 64,  ///< __NR_semget
  SYSCALL_SEMOP           = 65,  ///< __NR_semop
  SYSCALL_SEMCTL          = 66,  ///< __NR_semctl
  SYSCALL_SHMDT           = 67,  ///< __NR_shmdt
  SYSCALL_FCNTL           = 72,  ///< __NR_fcntl
  SYSCALL_FLOCK           = 73,  ///< __NR_flock
  SYSCALL_FSYNC           = 74,  ///< __NR_fsync
  SYSCALL_FDATASYNC       = 75,  ///< __NR_fdatasync
  SYSCALL_FTRUNCATE       = 77,  ///< __NR_ftruncate
  SYSCALL_GETDENTS        = 78,  ///< __NR_getdents
  SYSCALL_GETCWD          = 79,  ///< __NR_getcwd
  SYSCALL_CHDIR           = 80,  ///< __NR_chdir
  SYSCALL_RENAME          = 82,  ///< __NR_rename
  SYSCALL_MKDIR           = 83,  ///< __NR_mkdir
  SYSCALL_RMDIR           = 84,  ///< __NR_rmdir
  SYSCALL_LINK            = 86,  ///< __NR_link
  SYSCALL_UNLINK          = 87,  ///< __NR_unlink
  SYSCALL_READLINK        = 89,  ///< __NR_readlink
  SYSCALL_CHMOD           = 90,  ///< __NR_chmod
  SYSCALL_FCHMOD          = 91,  ///< __NR_fchmod
  SYSCALL_UMASK           = 95,  ///< __NR_umask
  SYSCALL_GETTIMEOFDAY    = 96,  ///< __NR_gettimeofday
  SYSCALL_GETRUSAGE       = 98,  ///< __NR_getrusage
  SYSCALL_SYSINFO         = 99,  ///< __NR_sysinfo
  SYSCALL_PTRACE          = 101, ///< __NR_ptrace
  SYSCALL_GETUID          = 102, ///< __NR_getuid
  SYSCALL_SYSLOG          = 103, ///< __NR_syslog
  SYSCALL_GETGID          = 104, ///< __NR_getgid
  SYSCALL_SETUID          = 105, ///< __NR_setuid
  SYSCALL_GETEUID         = 107, ///< __NR_geteuid
  SYSCALL_GETEGID         = 108, ///< __NR_getegid
  SYSCALL_GETPPID         = 110, ///< __NR_getppid
  SYSCALL_GETPGRP         = 111, ///< __NR_getpgrp
  SYSCALL_SETSID          = 112, ///< __NR_setsid
  SYSCALL_SETREUID        = 113, ///< __NR_setreuid
  SYSCALL_SETREGID        = 114, ///< __NR_setregid
  SYSCALL_SETRESUID       = 117, ///< __NR_setresuid
  SYSCALL_GETRESUID       = 118, ///< __NR_getresuid
  SYSCALL_SETRESGID       = 119, ///< __NR_setresgid
  SYSCALL_GETRESGID       = 120, ///< __NR_getresgid
  SYSCALL_SIGALTSTACK     = 131, ///< __NR_sigaltstack
  SYSCALL_MKNOD           = 133, ///< __NR_mknod
  SYSCALL_PERSONALITY     = 135, ///< __NR_personality
  SYSCALL_STATFS          = 137, ///< __NR_statfs
  SYSCALL_FSTATFS         = 138, ///< __NR_fstatfs
  SYSCALL_GETPRIORITY     = 140, ///< __NR_getpriority
  SYSCALL_SETPRIORITY     = 141, ///< __NR_getpriority
  SYSCALL_SCHED_SETPARAM  = 142, ///< __NR_sched_setparam
  SYSCALL_SCHED_GETPARAM  = 143, ///< __NR_sched_getparam
  SYSCALL_SCHED_SETSCHEDULER = 144, ///< __NR_sched_setscheduler
  SYSCALL_SCHED_GETSCHEDULER = 145, ///< __NR_sched_getscheduler
  SYSCALL_SCHED_GET_PRIORITY_MAX = 146, ///< __NR_sched_get_priority_max
  SYSCALL_SCHED_GET_PRIORITY_MIN = 147, ///< __NR_sched_get_priority_min
  SYSCALL_SCHED_RR_GET_INTERVAL  = 148, ///< __NR_sched_rr_get_interval
  SYSCALL_MLOCK           = 149, ///< __NR_mlock
  SYSCALL_MUNLOCK         = 150, ///< __NR_munlock
  SYSCALL_MLOCKALL        = 151, ///< __NR_mlockall
  SYSCALL_MUNLOCKALL      = 152, ///< __NR_munlockall
  SYSCALL_PRCTL           = 157, ///< __NR_prctl
  SYSCALL_ARCH_PRCTL      = 158, ///< __NR_arch_prctl
  SYSCALL_GETTID          = 186, ///< __NR_gettid
  SYSCALL_TIME            = 201, ///< __NR_time
  SYSCALL_FUTEX           = 202, ///< __NR_futex
  SYSCALL_SCHED_SETAFFINITY = 203, ///< __NR_sched_setaffinity
  SYSCALL_SCHED_GETAFFINITY = 204, ///< __NR_sched_getaffinity
  SYSCALL_EPOLL_CREATE    = 213, ///< __NR_epoll_create
  SYSCALL_EPOLL_CTL_OLD   = 214, ///< __NR_epoll_ctl_old
  SYSCALL_EPOLL_WAIT_OLD  = 215, ///< __NR_epoll_wait_old
  SYSCALL_GETDENTS64      = 217, ///< __NR_getdents64
  SYSCALL_SET_TID_ADDRESS = 218, ///< __NR_set_tid_address
  SYSCALL_SEMTIMEDOP      = 220, ///< __NR_semtimedop
  SYSCALL_FADVISE64       = 221, ///< __NR_fadvise64
  SYSCALL_TIMER_CREATE    = 222, ///< __NR_timer_create
  SYSCALL_TIMER_SETTIME   = 223, ///< __NR_timer_settime
  SYSCALL_TIMER_GETTIME   = 224, ///< __NR_timer_gettime
  SYSCALL_TIMER_GETOVERRUN= 225, ///< __NR_timer_getoverrun
  SYSCALL_TIMER_DELETE    = 226, ///< __NR_timer_delete
  SYSCALL_CLOCK_GETTIME   = 228, ///< __NR_clock_gettime
  SYSCALL_CLOCK_GETRES    = 229, ///< __NR_clock_getres
  SYSCALL_CLOCK_NANOSLEEP = 230, ///< __NR_clock_nanosleep
  SYSCALL_EXIT_GROUP      = 231, ///< __NR_exit_group
  SYSCALL_EPOLL_WAIT      = 232, ///< __NR_epoll_wait
  SYSCALL_EPOLL_CTL       = 233, ///< __NR_epoll_ctl
  SYSCALL_TGKILL          = 234, ///< __NR_tgkill
  SYSCALL_GET_MEMPOLICY   = 239, ///< __NR_get_mempolicy
  SYSCALL_INOTIFY_INIT      = 253, ///< __NR_inotify_init
  SYSCALL_INOTIFY_ADD_WATCH = 254, ///< __NR_inotify_add_watch
  SYSCALL_INOTIFY_RM_WATCH  = 255, ///< __NR_inotify_rm_watch
  SYSCALL_OPENAT          = 257, ///< __NR_openat
  SYSCALL_NEWFSTATAT      = 262, ///< __NR_newfstatat
  SYSCALL_READLINKAT      = 267, ///< __NR_readlinkat
  SYSCALL_FACCESSAT       = 269, ///< __NR_faccessat
  SYSCALL_PPOLL           = 271, ///< __NR_ppoll
  SYSCALL_SET_ROBUST_LIST = 273, ///< __NR_set_robust_list
  SYSCALL_GET_ROBUST_LIST = 274, ///< __NR_get_robust_list
  SYSCALL_EPOLL_PWAIT     = 281, ///< __NR_epoll_pwait
  SYSCALL_TIMERFD_CREATE  = 283, ///< __NR_timerfd_create
  SYSCALL_EVENTFD         = 290, ///< __NR_eventfd
  SYSCALL_EPOLL_CREATE1   = 291, ///< __NR_epoll_create1
  SYSCALL_DUP3            = 292, ///< __NR_dup3
  SYSCALL_PIPE2           = 293, ///< __NR_pipe2
  SYSCALL_INOTIFY_INIT1   = 294, ///< __NR_inotify_init1
  SYSCALL_PRLIMIT64       = 302, ///< __NR_prlimit64
  SYSCALL_SENDMMSG        = 307, ///< __NR_sendmmsg
  SYSCALL_SCHED_SETATTR   = 314, ///< __NR_sched_setattr
  SYSCALL_SCHED_GETATTR   = 315, ///< __NR_sched_getattr
  SYSCALL_GETRANDOM       = 318, ///< __NR_getrandom
  SYSCALL_MEMFD_CREATE    = 319, ///< __NR_memfd_create
  SYSCALL_MLOCK2          = 325, ///< __NR_mlock2
  SYSCALL_STATX           = 332, ///< __NR_statx
  SYSCALL_MAX             = 512,
};

FEXCore::SyscallHandler *CreateHandler(FEXCore::Context::Context *ctx);

}
