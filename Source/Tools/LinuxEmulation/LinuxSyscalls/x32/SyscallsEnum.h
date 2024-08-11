// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/
#pragma once

namespace FEX::HLE::x32 {
///< Enum containing all 32bit x86 linux syscalls for the guest kernel version
enum Syscalls_x86 {
  SYSCALL_x86_restart_syscall = 0,
  SYSCALL_x86_exit = 1,
  SYSCALL_x86_fork = 2,
  SYSCALL_x86_read = 3,
  SYSCALL_x86_write = 4,
  SYSCALL_x86_open = 5,
  SYSCALL_x86_close = 6,
  SYSCALL_x86_waitpid = 7,
  SYSCALL_x86_creat = 8,
  SYSCALL_x86_link = 9,
  SYSCALL_x86_unlink = 10,
  SYSCALL_x86_execve = 11,
  SYSCALL_x86_chdir = 12,
  SYSCALL_x86_time = 13,
  SYSCALL_x86_mknod = 14,
  SYSCALL_x86_chmod = 15,
  SYSCALL_x86_lchown = 16,
  // No entrypoint. -ENOSYS
  SYSCALL_x86_break = 17,
  SYSCALL_x86_oldstat = 18,
  SYSCALL_x86_lseek = 19,
  SYSCALL_x86_getpid = 20,
  SYSCALL_x86_mount = 21,
  SYSCALL_x86_umount = 22,
  SYSCALL_x86_setuid = 23,
  SYSCALL_x86_getuid = 24,
  SYSCALL_x86_stime = 25,
  SYSCALL_x86_ptrace = 26,
  SYSCALL_x86_alarm = 27,
  SYSCALL_x86_oldfstat = 28,
  SYSCALL_x86_pause = 29,
  SYSCALL_x86_utime = 30,
  // No entrypoint. -ENOSYS
  SYSCALL_x86_stty = 31,
  // No entrypoint. -ENOSYS
  SYSCALL_x86_gtty = 32,
  SYSCALL_x86_access = 33,
  SYSCALL_x86_nice = 34,
  // No entrypoint. -ENOSYS
  SYSCALL_x86_ftime = 35,
  SYSCALL_x86_sync = 36,
  SYSCALL_x86_kill = 37,
  SYSCALL_x86_rename = 38,
  SYSCALL_x86_mkdir = 39,
  SYSCALL_x86_rmdir = 40,
  SYSCALL_x86_dup = 41,
  SYSCALL_x86_pipe = 42,
  SYSCALL_x86_times = 43,
  // No entrypoint. -ENOSYS
  SYSCALL_x86_prof = 44,
  SYSCALL_x86_brk = 45,
  SYSCALL_x86_setgid = 46,
  SYSCALL_x86_getgid = 47,
  SYSCALL_x86_signal = 48,
  SYSCALL_x86_geteuid = 49,
  SYSCALL_x86_getegid = 50,
  SYSCALL_x86_acct = 51,
  SYSCALL_x86_umount2 = 52,
  // No entrypoint. -ENOSYS
  SYSCALL_x86_lock = 53,
  SYSCALL_x86_ioctl = 54,
  SYSCALL_x86_fcntl = 55,
  // No entrypoint. -ENOSYS
  SYSCALL_x86_mpx = 56,
  SYSCALL_x86_setpgid = 57,
  // No entrypoint. -ENOSYS
  SYSCALL_x86_ulimit = 58,
  SYSCALL_x86_oldolduname = 59,
  SYSCALL_x86_umask = 60,
  SYSCALL_x86_chroot = 61,
  SYSCALL_x86_ustat = 62,
  SYSCALL_x86_dup2 = 63,
  SYSCALL_x86_getppid = 64,
  SYSCALL_x86_getpgrp = 65,
  SYSCALL_x86_setsid = 66,
  SYSCALL_x86_sigaction = 67,
  SYSCALL_x86_sgetmask = 68,
  SYSCALL_x86_ssetmask = 69,
  SYSCALL_x86_setreuid = 70,
  SYSCALL_x86_setregid = 71,
  SYSCALL_x86_sigsuspend = 72,
  SYSCALL_x86_sigpending = 73,
  SYSCALL_x86_sethostname = 74,
  SYSCALL_x86_setrlimit = 75,
  SYSCALL_x86_getrlimit = 76,
  SYSCALL_x86_getrusage = 77,
  SYSCALL_x86_gettimeofday = 78,
  SYSCALL_x86_settimeofday = 79,
  SYSCALL_x86_getgroups = 80,
  SYSCALL_x86_setgroups = 81,
  SYSCALL_x86_select = 82,
  SYSCALL_x86_symlink = 83,
  SYSCALL_x86_oldlstat = 84,
  SYSCALL_x86_readlink = 85,
  SYSCALL_x86_uselib = 86,
  SYSCALL_x86_swapon = 87,
  SYSCALL_x86_reboot = 88,
  SYSCALL_x86_readdir = 89,
  SYSCALL_x86_mmap = 90,
  SYSCALL_x86_munmap = 91,
  SYSCALL_x86_truncate = 92,
  SYSCALL_x86_ftruncate = 93,
  SYSCALL_x86_fchmod = 94,
  SYSCALL_x86_fchown = 95,
  SYSCALL_x86_getpriority = 96,
  SYSCALL_x86_setpriority = 97,
  // No entrypoint. -ENOSYS
  SYSCALL_x86_profil = 98,
  SYSCALL_x86_statfs = 99,
  SYSCALL_x86_fstatfs = 100,
  SYSCALL_x86_ioperm = 101,
  SYSCALL_x86_socketcall = 102,
  SYSCALL_x86_syslog = 103,
  SYSCALL_x86_setitimer = 104,
  SYSCALL_x86_getitimer = 105,
  SYSCALL_x86_stat = 106,
  SYSCALL_x86_lstat = 107,
  SYSCALL_x86_fstat = 108,
  SYSCALL_x86_olduname = 109,
  SYSCALL_x86_iopl = 110,
  SYSCALL_x86_vhangup = 111,
  // No entrypoint. -ENOSYS
  SYSCALL_x86_idle = 112,
  SYSCALL_x86_vm86old = 113,
  SYSCALL_x86_wait4 = 114,
  SYSCALL_x86_swapoff = 115,
  SYSCALL_x86_sysinfo = 116,
  SYSCALL_x86_ipc = 117,
  SYSCALL_x86_fsync = 118,
  SYSCALL_x86_sigreturn = 119,
  SYSCALL_x86_clone = 120,
  SYSCALL_x86_setdomainname = 121,
  SYSCALL_x86_uname = 122,
  SYSCALL_x86_modify_ldt = 123,
  SYSCALL_x86_adjtimex = 124,
  SYSCALL_x86_mprotect = 125,
  SYSCALL_x86_sigprocmask = 126,
  // No entrypoint. -ENOSYS
  SYSCALL_x86_create_module = 127,
  SYSCALL_x86_init_module = 128,
  SYSCALL_x86_delete_module = 129,
  // No entrypoint. -ENOSYS
  SYSCALL_x86_get_kernel_syms = 130,
  SYSCALL_x86_quotactl = 131,
  SYSCALL_x86_getpgid = 132,
  SYSCALL_x86_fchdir = 133,
  SYSCALL_x86_bdflush = 134,
  SYSCALL_x86_sysfs = 135,
  SYSCALL_x86_personality = 136,
  // No entrypoint. -ENOSYS
  SYSCALL_x86_afs_syscall = 137,
  SYSCALL_x86_setfsuid = 138,
  SYSCALL_x86_setfsgid = 139,
  SYSCALL_x86__llseek = 140,
  SYSCALL_x86_getdents = 141,
  SYSCALL_x86__newselect = 142,
  SYSCALL_x86_flock = 143,
  SYSCALL_x86_msync = 144,
  SYSCALL_x86_readv = 145,
  SYSCALL_x86_writev = 146,
  SYSCALL_x86_getsid = 147,
  SYSCALL_x86_fdatasync = 148,
  SYSCALL_x86__sysctl = 149,
  SYSCALL_x86_mlock = 150,
  SYSCALL_x86_munlock = 151,
  SYSCALL_x86_mlockall = 152,
  SYSCALL_x86_munlockall = 153,
  SYSCALL_x86_sched_setparam = 154,
  SYSCALL_x86_sched_getparam = 155,
  SYSCALL_x86_sched_setscheduler = 156,
  SYSCALL_x86_sched_getscheduler = 157,
  SYSCALL_x86_sched_yield = 158,
  SYSCALL_x86_sched_get_priority_max = 159,
  SYSCALL_x86_sched_get_priority_min = 160,
  SYSCALL_x86_sched_rr_get_interval = 161,
  SYSCALL_x86_nanosleep = 162,
  SYSCALL_x86_mremap = 163,
  SYSCALL_x86_setresuid = 164,
  SYSCALL_x86_getresuid = 165,
  SYSCALL_x86_vm86 = 166,
  // No entrypoint. -ENOSYS
  SYSCALL_x86_query_module = 167,
  SYSCALL_x86_poll = 168,
  // No entrypoint. -ENOSYS
  SYSCALL_x86_nfsservctl = 169,
  SYSCALL_x86_setresgid = 170,
  SYSCALL_x86_getresgid = 171,
  SYSCALL_x86_prctl = 172,
  SYSCALL_x86_rt_sigreturn = 173,
  SYSCALL_x86_rt_sigaction = 174,
  SYSCALL_x86_rt_sigprocmask = 175,
  SYSCALL_x86_rt_sigpending = 176,
  SYSCALL_x86_rt_sigtimedwait = 177,
  SYSCALL_x86_rt_sigqueueinfo = 178,
  SYSCALL_x86_rt_sigsuspend = 179,
  SYSCALL_x86_pread_64 = 180,
  SYSCALL_x86_pwrite_64 = 181,
  SYSCALL_x86_chown = 182,
  SYSCALL_x86_getcwd = 183,
  SYSCALL_x86_capget = 184,
  SYSCALL_x86_capset = 185,
  SYSCALL_x86_sigaltstack = 186,
  SYSCALL_x86_sendfile = 187,
  // No entrypoint. -ENOSYS
  SYSCALL_x86_getpmsg = 188,
  // No entrypoint. -ENOSYS
  SYSCALL_x86_putpmsg = 189,
  SYSCALL_x86_vfork = 190,
  SYSCALL_x86_ugetrlimit = 191,
  SYSCALL_x86_mmap2 = 192,
  SYSCALL_x86_truncate64 = 193,
  SYSCALL_x86_ftruncate64 = 194,
  SYSCALL_x86_stat64 = 195,
  SYSCALL_x86_lstat64 = 196,
  SYSCALL_x86_fstat64 = 197,
  SYSCALL_x86_lchown32 = 198,
  SYSCALL_x86_getuid32 = 199,
  SYSCALL_x86_getgid32 = 200,
  SYSCALL_x86_geteuid32 = 201,
  SYSCALL_x86_getegid32 = 202,
  SYSCALL_x86_setreuid32 = 203,
  SYSCALL_x86_setregid32 = 204,
  SYSCALL_x86_getgroups32 = 205,
  SYSCALL_x86_setgroups32 = 206,
  SYSCALL_x86_fchown32 = 207,
  SYSCALL_x86_setresuid32 = 208,
  SYSCALL_x86_getresuid32 = 209,
  SYSCALL_x86_setresgid32 = 210,
  SYSCALL_x86_getresgid32 = 211,
  SYSCALL_x86_chown32 = 212,
  SYSCALL_x86_setuid32 = 213,
  SYSCALL_x86_setgid32 = 214,
  SYSCALL_x86_setfsuid32 = 215,
  SYSCALL_x86_setfsgid32 = 216,
  SYSCALL_x86_pivot_root = 217,
  SYSCALL_x86_mincore = 218,
  SYSCALL_x86_madvise = 219,
  SYSCALL_x86_getdents64 = 220,
  SYSCALL_x86_fcntl64 = 221,
  SYSCALL_x86_gettid = 224,
  SYSCALL_x86_readahead = 225,
  SYSCALL_x86_setxattr = 226,
  SYSCALL_x86_lsetxattr = 227,
  SYSCALL_x86_fsetxattr = 228,
  SYSCALL_x86_getxattr = 229,
  SYSCALL_x86_lgetxattr = 230,
  SYSCALL_x86_fgetxattr = 231,
  SYSCALL_x86_listxattr = 232,
  SYSCALL_x86_llistxattr = 233,
  SYSCALL_x86_flistxattr = 234,
  SYSCALL_x86_removexattr = 235,
  SYSCALL_x86_lremovexattr = 236,
  SYSCALL_x86_fremovexattr = 237,
  SYSCALL_x86_tkill = 238,
  SYSCALL_x86_sendfile64 = 239,
  SYSCALL_x86_futex = 240,
  SYSCALL_x86_sched_setaffinity = 241,
  SYSCALL_x86_sched_getaffinity = 242,
  SYSCALL_x86_set_thread_area = 243,
  SYSCALL_x86_get_thread_area = 244,
  SYSCALL_x86_io_setup = 245,
  SYSCALL_x86_io_destroy = 246,
  SYSCALL_x86_io_getevents = 247,
  SYSCALL_x86_io_submit = 248,
  SYSCALL_x86_io_cancel = 249,
  SYSCALL_x86_fadvise64 = 250,
  SYSCALL_x86_exit_group = 252,
  // No entrypoint. -ENOSYS
  SYSCALL_x86_lookup_dcookie = 253,
  SYSCALL_x86_epoll_create = 254,
  SYSCALL_x86_epoll_ctl = 255,
  SYSCALL_x86_epoll_wait = 256,
  SYSCALL_x86_remap_file_pages = 257,
  SYSCALL_x86_set_tid_address = 258,
  SYSCALL_x86_timer_create = 259,
  SYSCALL_x86_timer_settime = 260,
  SYSCALL_x86_timer_gettime = 261,
  SYSCALL_x86_timer_getoverrun = 262,
  SYSCALL_x86_timer_delete = 263,
  SYSCALL_x86_clock_settime = 264,
  SYSCALL_x86_clock_gettime = 265,
  SYSCALL_x86_clock_getres = 266,
  SYSCALL_x86_clock_nanosleep = 267,
  SYSCALL_x86_statfs64 = 268,
  SYSCALL_x86_fstatfs64 = 269,
  SYSCALL_x86_tgkill = 270,
  SYSCALL_x86_utimes = 271,
  SYSCALL_x86_fadvise64_64 = 272,
  // No entrypoint. -ENOSYS
  SYSCALL_x86_vserver = 273,
  SYSCALL_x86_mbind = 274,
  SYSCALL_x86_get_mempolicy = 275,
  SYSCALL_x86_set_mempolicy = 276,
  SYSCALL_x86_mq_open = 277,
  SYSCALL_x86_mq_unlink = 278,
  SYSCALL_x86_mq_timedsend = 279,
  SYSCALL_x86_mq_timedreceive = 280,
  SYSCALL_x86_mq_notify = 281,
  SYSCALL_x86_mq_getsetattr = 282,
  SYSCALL_x86_kexec_load = 283,
  SYSCALL_x86_waitid = 284,
  SYSCALL_x86_add_key = 286,
  SYSCALL_x86_request_key = 287,
  SYSCALL_x86_keyctl = 288,
  SYSCALL_x86_ioprio_set = 289,
  SYSCALL_x86_ioprio_get = 290,
  SYSCALL_x86_inotify_init = 291,
  SYSCALL_x86_inotify_add_watch = 292,
  SYSCALL_x86_inotify_rm_watch = 293,
  SYSCALL_x86_migrate_pages = 294,
  SYSCALL_x86_openat = 295,
  SYSCALL_x86_mkdirat = 296,
  SYSCALL_x86_mknodat = 297,
  SYSCALL_x86_fchownat = 298,
  SYSCALL_x86_futimesat = 299,
  SYSCALL_x86_fstatat_64 = 300,
  SYSCALL_x86_unlinkat = 301,
  SYSCALL_x86_renameat = 302,
  SYSCALL_x86_linkat = 303,
  SYSCALL_x86_symlinkat = 304,
  SYSCALL_x86_readlinkat = 305,
  SYSCALL_x86_fchmodat = 306,
  SYSCALL_x86_faccessat = 307,
  SYSCALL_x86_pselect6 = 308,
  SYSCALL_x86_ppoll = 309,
  SYSCALL_x86_unshare = 310,
  SYSCALL_x86_set_robust_list = 311,
  SYSCALL_x86_get_robust_list = 312,
  SYSCALL_x86_splice = 313,
  SYSCALL_x86_sync_file_range = 314,
  SYSCALL_x86_tee = 315,
  SYSCALL_x86_vmsplice = 316,
  SYSCALL_x86_move_pages = 317,
  SYSCALL_x86_getcpu = 318,
  SYSCALL_x86_epoll_pwait = 319,
  SYSCALL_x86_utimensat = 320,
  SYSCALL_x86_signalfd = 321,
  SYSCALL_x86_timerfd_create = 322,
  SYSCALL_x86_eventfd = 323,
  SYSCALL_x86_fallocate = 324,
  SYSCALL_x86_timerfd_settime = 325,
  SYSCALL_x86_timerfd_gettime = 326,
  SYSCALL_x86_signalfd4 = 327,
  SYSCALL_x86_eventfd2 = 328,
  SYSCALL_x86_epoll_create1 = 329,
  SYSCALL_x86_dup3 = 330,
  SYSCALL_x86_pipe2 = 331,
  SYSCALL_x86_inotify_init1 = 332,
  SYSCALL_x86_preadv = 333,
  SYSCALL_x86_pwritev = 334,
  SYSCALL_x86_rt_tgsigqueueinfo = 335,
  SYSCALL_x86_perf_event_open = 336,
  SYSCALL_x86_recvmmsg = 337,
  SYSCALL_x86_fanotify_init = 338,
  SYSCALL_x86_fanotify_mark = 339,
  SYSCALL_x86_prlimit_64 = 340,
  SYSCALL_x86_name_to_handle_at = 341,
  SYSCALL_x86_open_by_handle_at = 342,
  SYSCALL_x86_clock_adjtime = 343,
  SYSCALL_x86_syncfs = 344,
  SYSCALL_x86_sendmmsg = 345,
  SYSCALL_x86_setns = 346,
  SYSCALL_x86_process_vm_readv = 347,
  SYSCALL_x86_process_vm_writev = 348,
  SYSCALL_x86_kcmp = 349,
  SYSCALL_x86_finit_module = 350,
  SYSCALL_x86_sched_setattr = 351,
  SYSCALL_x86_sched_getattr = 352,
  SYSCALL_x86_renameat2 = 353,
  SYSCALL_x86_seccomp = 354,
  SYSCALL_x86_getrandom = 355,
  SYSCALL_x86_memfd_create = 356,
  SYSCALL_x86_bpf = 357,
  SYSCALL_x86_execveat = 358,
  SYSCALL_x86_socket = 359,
  SYSCALL_x86_socketpair = 360,
  SYSCALL_x86_bind = 361,
  SYSCALL_x86_connect = 362,
  SYSCALL_x86_listen = 363,
  SYSCALL_x86_accept4 = 364,
  SYSCALL_x86_getsockopt = 365,
  SYSCALL_x86_setsockopt = 366,
  SYSCALL_x86_getsockname = 367,
  SYSCALL_x86_getpeername = 368,
  SYSCALL_x86_sendto = 369,
  SYSCALL_x86_sendmsg = 370,
  SYSCALL_x86_recvfrom = 371,
  SYSCALL_x86_recvmsg = 372,
  SYSCALL_x86_shutdown = 373,
  SYSCALL_x86_userfaultfd = 374,
  SYSCALL_x86_membarrier = 375,
  SYSCALL_x86_mlock2 = 376,
  SYSCALL_x86_copy_file_range = 377,
  SYSCALL_x86_preadv2 = 378,
  SYSCALL_x86_pwritev2 = 379,
  SYSCALL_x86_pkey_mprotect = 380,
  SYSCALL_x86_pkey_alloc = 381,
  SYSCALL_x86_pkey_free = 382,
  SYSCALL_x86_statx = 383,
  SYSCALL_x86_arch_prctl = 384,
  SYSCALL_x86_io_pgetevents = 385,
  SYSCALL_x86_rseq = 386,
  SYSCALL_x86_semget = 393,
  SYSCALL_x86_semctl = 394,
  SYSCALL_x86_shmget = 395,
  SYSCALL_x86_shmctl = 396,
  SYSCALL_x86_shmat = 397,
  SYSCALL_x86_shmdt = 398,
  SYSCALL_x86_msgget = 399,
  SYSCALL_x86_msgsnd = 400,
  SYSCALL_x86_msgrcv = 401,
  SYSCALL_x86_msgctl = 402,
  SYSCALL_x86_clock_gettime64 = 403,
  SYSCALL_x86_clock_settime64 = 404,
  SYSCALL_x86_clock_adjtime64 = 405,
  SYSCALL_x86_clock_getres_time64 = 406,
  SYSCALL_x86_clock_nanosleep_time64 = 407,
  SYSCALL_x86_timer_gettime64 = 408,
  SYSCALL_x86_timer_settime64 = 409,
  SYSCALL_x86_timerfd_gettime64 = 410,
  SYSCALL_x86_timerfd_settime64 = 411,
  SYSCALL_x86_utimensat_time64 = 412,
  SYSCALL_x86_pselect6_time64 = 413,
  SYSCALL_x86_ppoll_time64 = 414,
  SYSCALL_x86_io_pgetevents_time64 = 416,
  SYSCALL_x86_recvmmsg_time64 = 417,
  SYSCALL_x86_mq_timedsend_time64 = 418,
  SYSCALL_x86_mq_timedreceive_time64 = 419,
  SYSCALL_x86_semtimedop_time64 = 420,
  SYSCALL_x86_rt_sigtimedwait_time64 = 421,
  SYSCALL_x86_futex_time64 = 422,
  SYSCALL_x86_sched_rr_get_interval_time64 = 423,
  SYSCALL_x86_pidfd_send_signal = 424,
  SYSCALL_x86_io_uring_setup = 425,
  SYSCALL_x86_io_uring_enter = 426,
  SYSCALL_x86_io_uring_register = 427,
  SYSCALL_x86_open_tree = 428,
  SYSCALL_x86_move_mount = 429,
  SYSCALL_x86_fsopen = 430,
  SYSCALL_x86_fsconfig = 431,
  SYSCALL_x86_fsmount = 432,
  SYSCALL_x86_fspick = 433,
  SYSCALL_x86_pidfd_open = 434,
  SYSCALL_x86_clone3 = 435,
  SYSCALL_x86_close_range = 436,
  SYSCALL_x86_openat2 = 437,
  SYSCALL_x86_pidfd_getfd = 438,
  SYSCALL_x86_faccessat2 = 439,
  SYSCALL_x86_process_madvise = 440,
  SYSCALL_x86_epoll_pwait2 = 441,
  SYSCALL_x86_mount_setattr = 442,
  SYSCALL_x86_quotactl_fd = 443,
  SYSCALL_x86_landlock_create_ruleset = 444,
  SYSCALL_x86_landlock_add_rule = 445,
  SYSCALL_x86_landlock_restrict_self = 446,
  SYSCALL_x86_memfd_secret = 447,
  SYSCALL_x86_process_mrelease = 448,
  SYSCALL_x86_futex_waitv = 449,
  SYSCALL_x86_set_mempolicy_home_node = 450,
  SYSCALL_x86_cachestat = 451,
  SYSCALL_x86_fchmodat2 = 452,
  SYSCALL_x86_map_shadow_stack = 453,
  SYSCALL_x86_futex_wake = 454,
  SYSCALL_x86_futex_wait = 455,
  SYSCALL_x86_futex_requeue = 456,
  SYSCALL_x86_statmount = 457,
  SYSCALL_x86_listmount = 458,
  SYSCALL_x86_lsm_get_self_attr = 459,
  SYSCALL_x86_lsm_set_self_attr = 460,
  SYSCALL_x86_lsm_list_modules = 461,
  SYSCALL_x86_mseal = 462,
  SYSCALL_x86_MAX = 512,
};
} // namespace FEX::HLE::x32
