// SPDX-License-Identifier: MIT
/*
$info$
meta: LinuxSyscalls|syscalls-shared ~ Syscall implementations shared between x86 and x86-64
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x64/Syscalls.h"
#include "LinuxSyscalls/x32/Syscalls.h"

#include <FEXCore/IR/IR.h>

#include <stdint.h>
#include <sys/epoll.h>

namespace FEX::HLE {
#ifdef _M_ARM_64
template<int syscall_num>
requires (syscall_num != -1)
uint64_t SyscallPassthrough0(FEXCore::Core::CpuStateFrame* Frame) {
  register uint64_t x0 asm("x0");
  register int x8 asm("x8") = syscall_num;
  __asm volatile(R"(
    svc #0;
  )"
                 : "=r"(x0)
                 : "r"(x8)
                 : "memory");
  return x0;
}

template<int syscall_num>
requires (syscall_num != -1)
uint64_t SyscallPassthrough1(FEXCore::Core::CpuStateFrame* Frame, uint64_t arg1) {
  register uint64_t x0 asm("x0") = arg1;
  register int x8 asm("x8") = syscall_num;
  __asm volatile(R"(
    svc #0;
  )"
                 : "=r"(x0)
                 : "r"(x8), "r"(x0)
                 : "memory");
  return x0;
}

template<int syscall_num>
requires (syscall_num != -1)
uint64_t SyscallPassthrough2(FEXCore::Core::CpuStateFrame* Frame, uint64_t arg1, uint64_t arg2) {
  register uint64_t x0 asm("x0") = arg1;
  register uint64_t x1 asm("x1") = arg2;
  register int x8 asm("x8") = syscall_num;
  __asm volatile(R"(
    svc #0;
  )"
                 : "=r"(x0)
                 : "r"(x8), "r"(x0), "r"(x1)
                 : "memory");
  return x0;
}

template<int syscall_num>
requires (syscall_num != -1)
uint64_t SyscallPassthrough3(FEXCore::Core::CpuStateFrame* Frame, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
  register uint64_t x0 asm("x0") = arg1;
  register uint64_t x1 asm("x1") = arg2;
  register uint64_t x2 asm("x2") = arg3;
  register int x8 asm("x8") = syscall_num;
  __asm volatile(R"(
    svc #0;
  )"
                 : "=r"(x0)
                 : "r"(x8), "r"(x0), "r"(x1), "r"(x2)
                 : "memory");
  return x0;
}

template<int syscall_num>
requires (syscall_num != -1)
uint64_t SyscallPassthrough4(FEXCore::Core::CpuStateFrame* Frame, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4) {
  register uint64_t x0 asm("x0") = arg1;
  register uint64_t x1 asm("x1") = arg2;
  register uint64_t x2 asm("x2") = arg3;
  register uint64_t x3 asm("x3") = arg4;
  register int x8 asm("x8") = syscall_num;
  __asm volatile(R"(
    svc #0;
  )"
                 : "=r"(x0)
                 : "r"(x8), "r"(x0), "r"(x1), "r"(x2), "r"(x3)
                 : "memory");
  return x0;
}

template<int syscall_num>
requires (syscall_num != -1)
uint64_t SyscallPassthrough5(FEXCore::Core::CpuStateFrame* Frame, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5) {
  register uint64_t x0 asm("x0") = arg1;
  register uint64_t x1 asm("x1") = arg2;
  register uint64_t x2 asm("x2") = arg3;
  register uint64_t x3 asm("x3") = arg4;
  register uint64_t x4 asm("x4") = arg5;
  register int x8 asm("x8") = syscall_num;
  __asm volatile(R"(
    svc #0;
  )"
                 : "=r"(x0)
                 : "r"(x8), "r"(x0), "r"(x1), "r"(x2), "r"(x3), "r"(x4)
                 : "memory");
  return x0;
}

template<int syscall_num>
requires (syscall_num != -1)
uint64_t SyscallPassthrough6(FEXCore::Core::CpuStateFrame* Frame, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5,
                             uint64_t arg6) {
  register uint64_t x0 asm("x0") = arg1;
  register uint64_t x1 asm("x1") = arg2;
  register uint64_t x2 asm("x2") = arg3;
  register uint64_t x3 asm("x3") = arg4;
  register uint64_t x4 asm("x4") = arg5;
  register uint64_t x5 asm("x5") = arg6;
  register int x8 asm("x8") = syscall_num;
  __asm volatile(R"(
    svc #0;
  )"
                 : "=r"(x0)
                 : "r"(x8), "r"(x0), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5)
                 : "memory");
  return x0;
}

template<int syscall_num>
requires (syscall_num != -1)
uint64_t SyscallPassthrough7(FEXCore::Core::CpuStateFrame* Frame, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5,
                             uint64_t arg6, uint64_t arg7) {
  register uint64_t x0 asm("x0") = arg1;
  register uint64_t x1 asm("x1") = arg2;
  register uint64_t x2 asm("x2") = arg3;
  register uint64_t x3 asm("x3") = arg4;
  register uint64_t x4 asm("x4") = arg5;
  register uint64_t x5 asm("x5") = arg6;
  register uint64_t x6 asm("x6") = arg7;
  register int x8 asm("x8") = syscall_num;
  __asm volatile(R"(
    svc #0;
  )"
                 : "=r"(x0)
                 : "r"(x8), "r"(x0), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5), "r"(x6)
                 : "memory");
  return x0;
}
#else
template<int syscall_num>
requires (syscall_num != -1)
uint64_t SyscallPassthrough0(FEXCore::Core::CpuStateFrame* Frame) {
  uint64_t Result = ::syscall(syscall_num);
  SYSCALL_ERRNO();
}

template<int syscall_num>
requires (syscall_num != -1)
uint64_t SyscallPassthrough1(FEXCore::Core::CpuStateFrame* Frame, uint64_t arg1) {
  uint64_t Result = ::syscall(syscall_num, arg1);
  SYSCALL_ERRNO();
}

template<int syscall_num>
requires (syscall_num != -1)
uint64_t SyscallPassthrough2(FEXCore::Core::CpuStateFrame* Frame, uint64_t arg1, uint64_t arg2) {
  uint64_t Result = ::syscall(syscall_num, arg1, arg2);
  SYSCALL_ERRNO();
}

template<int syscall_num>
requires (syscall_num != -1)
uint64_t SyscallPassthrough3(FEXCore::Core::CpuStateFrame* Frame, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
  uint64_t Result = ::syscall(syscall_num, arg1, arg2, arg3);
  SYSCALL_ERRNO();
}

template<int syscall_num>
requires (syscall_num != -1)
uint64_t SyscallPassthrough4(FEXCore::Core::CpuStateFrame* Frame, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4) {
  uint64_t Result = ::syscall(syscall_num, arg1, arg2, arg3, arg4);
  SYSCALL_ERRNO();
}

template<int syscall_num>
requires (syscall_num != -1)
uint64_t SyscallPassthrough5(FEXCore::Core::CpuStateFrame* Frame, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5) {
  uint64_t Result = ::syscall(syscall_num, arg1, arg2, arg3, arg4, arg5);
  SYSCALL_ERRNO();
}

template<int syscall_num>
requires (syscall_num != -1)
uint64_t SyscallPassthrough6(FEXCore::Core::CpuStateFrame* Frame, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5,
                             uint64_t arg6) {
  uint64_t Result = ::syscall(syscall_num, arg1, arg2, arg3, arg4, arg5, arg6);
  SYSCALL_ERRNO();
}

template<int syscall_num>
requires (syscall_num != -1)
uint64_t SyscallPassthrough7(FEXCore::Core::CpuStateFrame* Frame, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5,
                             uint64_t arg6, uint64_t arg7) {
  uint64_t Result = ::syscall(syscall_num, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
  SYSCALL_ERRNO();
}
#endif

void RegisterCommon(FEX::HLE::SyscallHandler* Handler) {
  using namespace FEXCore::IR;
  REGISTER_SYSCALL_IMPL(read, SyscallPassthrough3<SYSCALL_DEF(read)>);
  REGISTER_SYSCALL_IMPL(write, SyscallPassthrough3<SYSCALL_DEF(write)>);
  REGISTER_SYSCALL_IMPL(lseek, SyscallPassthrough3<SYSCALL_DEF(lseek)>);
  REGISTER_SYSCALL_IMPL(sched_yield, SyscallPassthrough0<SYSCALL_DEF(sched_yield)>);
  REGISTER_SYSCALL_IMPL(msync, SyscallPassthrough3<SYSCALL_DEF(msync)>);
  REGISTER_SYSCALL_IMPL(mincore, SyscallPassthrough3<SYSCALL_DEF(mincore)>);
  REGISTER_SYSCALL_IMPL(shmget, SyscallPassthrough3<SYSCALL_DEF(shmget)>);
  REGISTER_SYSCALL_IMPL(shmctl, SyscallPassthrough3<SYSCALL_DEF(shmctl)>);
  REGISTER_SYSCALL_IMPL(getpid, SyscallPassthrough0<SYSCALL_DEF(getpid)>);
  REGISTER_SYSCALL_IMPL(socket, SyscallPassthrough3<SYSCALL_DEF(socket)>);
  REGISTER_SYSCALL_IMPL(connect, SyscallPassthrough3<SYSCALL_DEF(connect)>);
  REGISTER_SYSCALL_IMPL(sendto, SyscallPassthrough6<SYSCALL_DEF(sendto)>);
  REGISTER_SYSCALL_IMPL(recvfrom, SyscallPassthrough6<SYSCALL_DEF(recvfrom)>);
  REGISTER_SYSCALL_IMPL(shutdown, SyscallPassthrough2<SYSCALL_DEF(shutdown)>);
  REGISTER_SYSCALL_IMPL(bind, SyscallPassthrough3<SYSCALL_DEF(bind)>);
  REGISTER_SYSCALL_IMPL(listen, SyscallPassthrough2<SYSCALL_DEF(listen)>);
  REGISTER_SYSCALL_IMPL(getsockname, SyscallPassthrough3<SYSCALL_DEF(getsockname)>);
  REGISTER_SYSCALL_IMPL(getpeername, SyscallPassthrough3<SYSCALL_DEF(getpeername)>);
  REGISTER_SYSCALL_IMPL(socketpair, SyscallPassthrough4<SYSCALL_DEF(socketpair)>);
  REGISTER_SYSCALL_IMPL(kill, SyscallPassthrough2<SYSCALL_DEF(kill)>);
  REGISTER_SYSCALL_IMPL(semget, SyscallPassthrough3<SYSCALL_DEF(semget)>);
  REGISTER_SYSCALL_IMPL(msgget, SyscallPassthrough2<SYSCALL_DEF(msgget)>);
  REGISTER_SYSCALL_IMPL(msgsnd, SyscallPassthrough4<SYSCALL_DEF(msgsnd)>);
  REGISTER_SYSCALL_IMPL(msgrcv, SyscallPassthrough5<SYSCALL_DEF(msgrcv)>);
  REGISTER_SYSCALL_IMPL(msgctl, SyscallPassthrough3<SYSCALL_DEF(msgctl)>);
  REGISTER_SYSCALL_IMPL(flock, SyscallPassthrough2<SYSCALL_DEF(flock)>);
  REGISTER_SYSCALL_IMPL(fsync, SyscallPassthrough1<SYSCALL_DEF(fsync)>);
  REGISTER_SYSCALL_IMPL(fdatasync, SyscallPassthrough1<SYSCALL_DEF(fdatasync)>);
  REGISTER_SYSCALL_IMPL(truncate, SyscallPassthrough2<SYSCALL_DEF(truncate)>);
  REGISTER_SYSCALL_IMPL(ftruncate, SyscallPassthrough2<SYSCALL_DEF(ftruncate)>);
  REGISTER_SYSCALL_IMPL(getcwd, SyscallPassthrough2<SYSCALL_DEF(getcwd)>);
  REGISTER_SYSCALL_IMPL(chdir, SyscallPassthrough1<SYSCALL_DEF(chdir)>);
  REGISTER_SYSCALL_IMPL(fchdir, SyscallPassthrough1<SYSCALL_DEF(fchdir)>);
  REGISTER_SYSCALL_IMPL(fchmod, SyscallPassthrough2<SYSCALL_DEF(fchmod)>);
  REGISTER_SYSCALL_IMPL(fchown, SyscallPassthrough3<SYSCALL_DEF(fchown)>);
  REGISTER_SYSCALL_IMPL(umask, SyscallPassthrough1<SYSCALL_DEF(umask)>);
  REGISTER_SYSCALL_IMPL(getuid, SyscallPassthrough0<SYSCALL_DEF(getuid)>);
  REGISTER_SYSCALL_IMPL(syslog, SyscallPassthrough3<SYSCALL_DEF(syslog)>);
  REGISTER_SYSCALL_IMPL(getgid, SyscallPassthrough0<SYSCALL_DEF(getgid)>);
  REGISTER_SYSCALL_IMPL(setuid, SyscallPassthrough1<SYSCALL_DEF(setuid)>);
  REGISTER_SYSCALL_IMPL(setgid, SyscallPassthrough1<SYSCALL_DEF(setgid)>);
  REGISTER_SYSCALL_IMPL(geteuid, SyscallPassthrough0<SYSCALL_DEF(geteuid)>);
  REGISTER_SYSCALL_IMPL(getegid, SyscallPassthrough0<SYSCALL_DEF(getegid)>);
  REGISTER_SYSCALL_IMPL(setpgid, SyscallPassthrough2<SYSCALL_DEF(setpgid)>);
  REGISTER_SYSCALL_IMPL(getppid, SyscallPassthrough0<SYSCALL_DEF(getppid)>);
  REGISTER_SYSCALL_IMPL(setsid, SyscallPassthrough0<SYSCALL_DEF(setsid)>);
  REGISTER_SYSCALL_IMPL(setreuid, SyscallPassthrough2<SYSCALL_DEF(setreuid)>);
  REGISTER_SYSCALL_IMPL(setregid, SyscallPassthrough2<SYSCALL_DEF(setregid)>);
  REGISTER_SYSCALL_IMPL(getgroups, SyscallPassthrough2<SYSCALL_DEF(getgroups)>);
  REGISTER_SYSCALL_IMPL(setgroups, SyscallPassthrough2<SYSCALL_DEF(setgroups)>);
  REGISTER_SYSCALL_IMPL(setresuid, SyscallPassthrough3<SYSCALL_DEF(setresuid)>);
  REGISTER_SYSCALL_IMPL(getresuid, SyscallPassthrough3<SYSCALL_DEF(getresuid)>);
  REGISTER_SYSCALL_IMPL(setresgid, SyscallPassthrough3<SYSCALL_DEF(setresgid)>);
  REGISTER_SYSCALL_IMPL(getresgid, SyscallPassthrough3<SYSCALL_DEF(getresgid)>);
  REGISTER_SYSCALL_IMPL(getpgid, SyscallPassthrough1<SYSCALL_DEF(getpgid)>);
  REGISTER_SYSCALL_IMPL(setfsuid, SyscallPassthrough1<SYSCALL_DEF(setfsuid)>);
  REGISTER_SYSCALL_IMPL(setfsgid, SyscallPassthrough1<SYSCALL_DEF(setfsgid)>);
  REGISTER_SYSCALL_IMPL(getsid, SyscallPassthrough1<SYSCALL_DEF(getsid)>);
  REGISTER_SYSCALL_IMPL(capget, SyscallPassthrough2<SYSCALL_DEF(capget)>);
  REGISTER_SYSCALL_IMPL(capset, SyscallPassthrough2<SYSCALL_DEF(capset)>);
  REGISTER_SYSCALL_IMPL(getpriority, SyscallPassthrough2<SYSCALL_DEF(getpriority)>);
  REGISTER_SYSCALL_IMPL(setpriority, SyscallPassthrough3<SYSCALL_DEF(setpriority)>);
  REGISTER_SYSCALL_IMPL(sched_setparam, SyscallPassthrough2<SYSCALL_DEF(sched_setparam)>);
  REGISTER_SYSCALL_IMPL(sched_getparam, SyscallPassthrough2<SYSCALL_DEF(sched_getparam)>);
  REGISTER_SYSCALL_IMPL(sched_setscheduler, SyscallPassthrough3<SYSCALL_DEF(sched_setscheduler)>);
  REGISTER_SYSCALL_IMPL(sched_getscheduler, SyscallPassthrough1<SYSCALL_DEF(sched_getscheduler)>);
  REGISTER_SYSCALL_IMPL(sched_get_priority_max, SyscallPassthrough1<SYSCALL_DEF(sched_get_priority_max)>);
  REGISTER_SYSCALL_IMPL(sched_get_priority_min, SyscallPassthrough1<SYSCALL_DEF(sched_get_priority_min)>);
  REGISTER_SYSCALL_IMPL(mlock, SyscallPassthrough2<SYSCALL_DEF(mlock)>);
  REGISTER_SYSCALL_IMPL(munlock, SyscallPassthrough2<SYSCALL_DEF(munlock)>);
  REGISTER_SYSCALL_IMPL(pivot_root, SyscallPassthrough2<SYSCALL_DEF(pivot_root)>);
  REGISTER_SYSCALL_IMPL(chroot, SyscallPassthrough1<SYSCALL_DEF(chroot)>);
  REGISTER_SYSCALL_IMPL(sync, SyscallPassthrough0<SYSCALL_DEF(sync)>);
  REGISTER_SYSCALL_IMPL(acct, SyscallPassthrough1<SYSCALL_DEF(acct)>);
  REGISTER_SYSCALL_IMPL(mount, SyscallPassthrough5<SYSCALL_DEF(mount)>);
  REGISTER_SYSCALL_IMPL(umount2, SyscallPassthrough2<SYSCALL_DEF(umount2)>);
  REGISTER_SYSCALL_IMPL(swapon, SyscallPassthrough2<SYSCALL_DEF(swapon)>);
  REGISTER_SYSCALL_IMPL(swapoff, SyscallPassthrough1<SYSCALL_DEF(swapoff)>);
  REGISTER_SYSCALL_IMPL(gettid, SyscallPassthrough0<SYSCALL_DEF(gettid)>);
  REGISTER_SYSCALL_IMPL(fsetxattr, SyscallPassthrough5<SYSCALL_DEF(fsetxattr)>);
  REGISTER_SYSCALL_IMPL(fgetxattr, SyscallPassthrough4<SYSCALL_DEF(fgetxattr)>);
  REGISTER_SYSCALL_IMPL(flistxattr, SyscallPassthrough3<SYSCALL_DEF(flistxattr)>);
  REGISTER_SYSCALL_IMPL(fremovexattr, SyscallPassthrough2<SYSCALL_DEF(fremovexattr)>);
  REGISTER_SYSCALL_IMPL(tkill, SyscallPassthrough2<SYSCALL_DEF(tkill)>);
  REGISTER_SYSCALL_IMPL(sched_setaffinity, SyscallPassthrough3<SYSCALL_DEF(sched_setaffinity)>);
  REGISTER_SYSCALL_IMPL(sched_getaffinity, SyscallPassthrough3<SYSCALL_DEF(sched_getaffinity)>);
  REGISTER_SYSCALL_IMPL(io_setup, SyscallPassthrough2<SYSCALL_DEF(io_setup)>);
  REGISTER_SYSCALL_IMPL(io_destroy, SyscallPassthrough1<SYSCALL_DEF(io_destroy)>);
  REGISTER_SYSCALL_IMPL(io_submit, SyscallPassthrough3<SYSCALL_DEF(io_submit)>);
  REGISTER_SYSCALL_IMPL(io_cancel, SyscallPassthrough3<SYSCALL_DEF(io_cancel)>);
  REGISTER_SYSCALL_IMPL(remap_file_pages, SyscallPassthrough5<SYSCALL_DEF(remap_file_pages)>);
  REGISTER_SYSCALL_IMPL(timer_getoverrun, SyscallPassthrough1<SYSCALL_DEF(timer_getoverrun)>);
  REGISTER_SYSCALL_IMPL(timer_delete, SyscallPassthrough1<SYSCALL_DEF(timer_delete)>);
  REGISTER_SYSCALL_IMPL(tgkill, SyscallPassthrough3<SYSCALL_DEF(tgkill)>);
  REGISTER_SYSCALL_IMPL(mbind, SyscallPassthrough6<SYSCALL_DEF(mbind)>);
  REGISTER_SYSCALL_IMPL(set_mempolicy, SyscallPassthrough3<SYSCALL_DEF(set_mempolicy)>);
  REGISTER_SYSCALL_IMPL(get_mempolicy, SyscallPassthrough5<SYSCALL_DEF(get_mempolicy)>);
  REGISTER_SYSCALL_IMPL(mq_unlink, SyscallPassthrough1<SYSCALL_DEF(mq_unlink)>);
  REGISTER_SYSCALL_IMPL(add_key, SyscallPassthrough5<SYSCALL_DEF(add_key)>);
  REGISTER_SYSCALL_IMPL(request_key, SyscallPassthrough4<SYSCALL_DEF(request_key)>);
  REGISTER_SYSCALL_IMPL(keyctl, SyscallPassthrough5<SYSCALL_DEF(keyctl)>);
  REGISTER_SYSCALL_IMPL(ioprio_set, SyscallPassthrough2<SYSCALL_DEF(ioprio_set)>);
  REGISTER_SYSCALL_IMPL(ioprio_get, SyscallPassthrough3<SYSCALL_DEF(ioprio_get)>);
  REGISTER_SYSCALL_IMPL(inotify_add_watch, SyscallPassthrough3<SYSCALL_DEF(inotify_add_watch)>);
  REGISTER_SYSCALL_IMPL(inotify_rm_watch, SyscallPassthrough2<SYSCALL_DEF(inotify_rm_watch)>);
  REGISTER_SYSCALL_IMPL(migrate_pages, SyscallPassthrough4<SYSCALL_DEF(migrate_pages)>);
  REGISTER_SYSCALL_IMPL(mkdirat, SyscallPassthrough3<SYSCALL_DEF(mkdirat)>);
  REGISTER_SYSCALL_IMPL(mknodat, SyscallPassthrough4<SYSCALL_DEF(mknodat)>);
  REGISTER_SYSCALL_IMPL(fchownat, SyscallPassthrough5<SYSCALL_DEF(fchownat)>);
  REGISTER_SYSCALL_IMPL(unlinkat, SyscallPassthrough3<SYSCALL_DEF(unlinkat)>);
  REGISTER_SYSCALL_IMPL(renameat, SyscallPassthrough4<SYSCALL_DEF(renameat)>);
  REGISTER_SYSCALL_IMPL(linkat, SyscallPassthrough5<SYSCALL_DEF(linkat)>);
  REGISTER_SYSCALL_IMPL(symlinkat, SyscallPassthrough3<SYSCALL_DEF(symlinkat)>);
  REGISTER_SYSCALL_IMPL(fchmodat, SyscallPassthrough3<SYSCALL_DEF(fchmodat)>);
  REGISTER_SYSCALL_IMPL(unshare, SyscallPassthrough1<SYSCALL_DEF(unshare)>);
  REGISTER_SYSCALL_IMPL(splice, SyscallPassthrough6<SYSCALL_DEF(splice)>);
  REGISTER_SYSCALL_IMPL(tee, SyscallPassthrough4<SYSCALL_DEF(tee)>);
  REGISTER_SYSCALL_IMPL(move_pages, SyscallPassthrough6<SYSCALL_DEF(move_pages)>);
  REGISTER_SYSCALL_IMPL(timerfd_create, SyscallPassthrough2<SYSCALL_DEF(timerfd_create)>);
  REGISTER_SYSCALL_IMPL(accept4, SyscallPassthrough4<SYSCALL_DEF(accept4)>);
  REGISTER_SYSCALL_IMPL(eventfd2, SyscallPassthrough2<SYSCALL_DEF(eventfd2)>);
  REGISTER_SYSCALL_IMPL(epoll_create1, SyscallPassthrough1<SYSCALL_DEF(epoll_create1)>);
  REGISTER_SYSCALL_IMPL(inotify_init1, SyscallPassthrough1<SYSCALL_DEF(inotify_init1)>);
  REGISTER_SYSCALL_IMPL(fanotify_init, SyscallPassthrough2<SYSCALL_DEF(fanotify_init)>);
  REGISTER_SYSCALL_IMPL(fanotify_mark, SyscallPassthrough5<SYSCALL_DEF(fanotify_mark)>);
  REGISTER_SYSCALL_IMPL(prlimit_64, SyscallPassthrough4<SYSCALL_DEF(prlimit_64)>);
  REGISTER_SYSCALL_IMPL(name_to_handle_at, SyscallPassthrough5<SYSCALL_DEF(name_to_handle_at)>);
  REGISTER_SYSCALL_IMPL(open_by_handle_at, SyscallPassthrough3<SYSCALL_DEF(open_by_handle_at)>);
  REGISTER_SYSCALL_IMPL(syncfs, SyscallPassthrough1<SYSCALL_DEF(syncfs)>);
  REGISTER_SYSCALL_IMPL(setns, SyscallPassthrough2<SYSCALL_DEF(setns)>);
  REGISTER_SYSCALL_IMPL(getcpu, SyscallPassthrough3<SYSCALL_DEF(getcpu)>);
  REGISTER_SYSCALL_IMPL(kcmp, SyscallPassthrough5<SYSCALL_DEF(kcmp)>);
  REGISTER_SYSCALL_IMPL(sched_setattr, SyscallPassthrough3<SYSCALL_DEF(sched_setattr)>);
  REGISTER_SYSCALL_IMPL(sched_getattr, SyscallPassthrough4<SYSCALL_DEF(sched_getattr)>);
  REGISTER_SYSCALL_IMPL(renameat2, SyscallPassthrough5<SYSCALL_DEF(renameat2)>);
  REGISTER_SYSCALL_IMPL(getrandom, SyscallPassthrough3<SYSCALL_DEF(getrandom)>);
  REGISTER_SYSCALL_IMPL(memfd_create, SyscallPassthrough2<SYSCALL_DEF(memfd_create)>);
  REGISTER_SYSCALL_IMPL(membarrier, SyscallPassthrough2<SYSCALL_DEF(membarrier)>);
  REGISTER_SYSCALL_IMPL(mlock2, SyscallPassthrough3<SYSCALL_DEF(mlock2)>);
  REGISTER_SYSCALL_IMPL(copy_file_range, SyscallPassthrough6<SYSCALL_DEF(copy_file_range)>);
  REGISTER_SYSCALL_IMPL(pkey_mprotect, SyscallPassthrough4<SYSCALL_DEF(pkey_mprotect)>);
  REGISTER_SYSCALL_IMPL(pkey_alloc, SyscallPassthrough2<SYSCALL_DEF(pkey_alloc)>);
  REGISTER_SYSCALL_IMPL(pkey_free, SyscallPassthrough1<SYSCALL_DEF(pkey_free)>);
  REGISTER_SYSCALL_IMPL(io_uring_setup, SyscallPassthrough2<SYSCALL_DEF(io_uring_setup)>);
  REGISTER_SYSCALL_IMPL(io_uring_enter, SyscallPassthrough6<SYSCALL_DEF(io_uring_enter)>);
  REGISTER_SYSCALL_IMPL(io_uring_register, SyscallPassthrough4<SYSCALL_DEF(io_uring_register)>);
  REGISTER_SYSCALL_IMPL(open_tree, SyscallPassthrough3<SYSCALL_DEF(open_tree)>);
  REGISTER_SYSCALL_IMPL(move_mount, SyscallPassthrough5<SYSCALL_DEF(move_mount)>);
  REGISTER_SYSCALL_IMPL(fsopen, SyscallPassthrough3<SYSCALL_DEF(fsopen)>);
  REGISTER_SYSCALL_IMPL(fsconfig, SyscallPassthrough5<SYSCALL_DEF(fsconfig)>);
  REGISTER_SYSCALL_IMPL(fsmount, SyscallPassthrough3<SYSCALL_DEF(fsmount)>);
  REGISTER_SYSCALL_IMPL(fspick, SyscallPassthrough3<SYSCALL_DEF(fspick)>);
  REGISTER_SYSCALL_IMPL(pidfd_open, SyscallPassthrough2<SYSCALL_DEF(pidfd_open)>);
  REGISTER_SYSCALL_IMPL(pidfd_getfd, SyscallPassthrough3<SYSCALL_DEF(pidfd_getfd)>);
  REGISTER_SYSCALL_IMPL(mount_setattr, SyscallPassthrough5<SYSCALL_DEF(mount_setattr)>);
  REGISTER_SYSCALL_IMPL(quotactl_fd, SyscallPassthrough4<SYSCALL_DEF(quotactl_fd)>);
  REGISTER_SYSCALL_IMPL(landlock_create_ruleset, SyscallPassthrough3<SYSCALL_DEF(landlock_create_ruleset)>);
  REGISTER_SYSCALL_IMPL(landlock_add_rule, SyscallPassthrough4<SYSCALL_DEF(landlock_add_rule)>);
  REGISTER_SYSCALL_IMPL(landlock_restrict_self, SyscallPassthrough2<SYSCALL_DEF(landlock_restrict_self)>);
  REGISTER_SYSCALL_IMPL(memfd_secret, SyscallPassthrough1<SYSCALL_DEF(memfd_secret)>);
  REGISTER_SYSCALL_IMPL(process_mrelease, SyscallPassthrough2<SYSCALL_DEF(process_mrelease)>);
  if (Handler->IsHostKernelVersionAtLeast(5, 16, 0)) {
    REGISTER_SYSCALL_IMPL(futex_waitv, SyscallPassthrough5<SYSCALL_DEF(futex_waitv)>);
  } else {
    REGISTER_SYSCALL_IMPL(futex_waitv, UnimplementedSyscallSafe);
  }
  if (Handler->IsHostKernelVersionAtLeast(5, 17, 0)) {
    REGISTER_SYSCALL_IMPL(set_mempolicy_home_node, SyscallPassthrough4<SYSCALL_DEF(set_mempolicy_home_node)>);
  } else {
    REGISTER_SYSCALL_IMPL(set_mempolicy_home_node, UnimplementedSyscallSafe);
  }

  if (Handler->IsHostKernelVersionAtLeast(6, 8, 0)) {
    REGISTER_SYSCALL_IMPL(futex_wake, SyscallPassthrough4<SYSCALL_DEF(futex_wake)>);
    REGISTER_SYSCALL_IMPL(futex_wait, SyscallPassthrough6<SYSCALL_DEF(futex_wait)>);
    REGISTER_SYSCALL_IMPL(futex_requeue, SyscallPassthrough4<SYSCALL_DEF(futex_requeue)>);
    REGISTER_SYSCALL_IMPL(statmount, SyscallPassthrough4<SYSCALL_DEF(statmount)>);
    REGISTER_SYSCALL_IMPL(listmount, SyscallPassthrough4<SYSCALL_DEF(listmount)>);
    REGISTER_SYSCALL_IMPL(lsm_get_self_attr, SyscallPassthrough4<SYSCALL_DEF(lsm_get_self_attr)>);
    REGISTER_SYSCALL_IMPL(lsm_set_self_attr, SyscallPassthrough4<SYSCALL_DEF(lsm_set_self_attr)>);
    REGISTER_SYSCALL_IMPL(lsm_list_modules, SyscallPassthrough3<SYSCALL_DEF(lsm_list_modules)>);
  } else {
    REGISTER_SYSCALL_IMPL(futex_wake, UnimplementedSyscallSafe);
    REGISTER_SYSCALL_IMPL(futex_wait, UnimplementedSyscallSafe);
    REGISTER_SYSCALL_IMPL(futex_requeue, UnimplementedSyscallSafe);
    REGISTER_SYSCALL_IMPL(statmount, UnimplementedSyscallSafe);
    REGISTER_SYSCALL_IMPL(listmount, UnimplementedSyscallSafe);
    REGISTER_SYSCALL_IMPL(lsm_get_self_attr, UnimplementedSyscallSafe);
    REGISTER_SYSCALL_IMPL(lsm_set_self_attr, UnimplementedSyscallSafe);
    REGISTER_SYSCALL_IMPL(lsm_list_modules, UnimplementedSyscallSafe);
  }
  if (Handler->IsHostKernelVersionAtLeast(6, 10, 0)) {
    REGISTER_SYSCALL_IMPL(mseal, SyscallPassthrough3<SYSCALL_DEF(mseal)>);
  } else {
    REGISTER_SYSCALL_IMPL(mseal, UnimplementedSyscallSafe);
  }
}

namespace x64 {
  void RegisterPassthrough(FEX::HLE::SyscallHandler* Handler) {
    using namespace FEXCore::IR;
    RegisterCommon(Handler);
    REGISTER_SYSCALL_IMPL_X64(ioctl, SyscallPassthrough3<SYSCALL_DEF(ioctl)>);
    REGISTER_SYSCALL_IMPL_X64(pread_64, SyscallPassthrough4<SYSCALL_DEF(pread_64)>);
    REGISTER_SYSCALL_IMPL_X64(pwrite_64, SyscallPassthrough4<SYSCALL_DEF(pwrite_64)>);
    REGISTER_SYSCALL_IMPL_X64(readv, SyscallPassthrough3<SYSCALL_DEF(readv)>);
    REGISTER_SYSCALL_IMPL_X64(writev, SyscallPassthrough3<SYSCALL_DEF(writev)>);
    REGISTER_SYSCALL_IMPL_X64(dup, SyscallPassthrough1<SYSCALL_DEF(dup)>);
    REGISTER_SYSCALL_IMPL_X64(nanosleep, SyscallPassthrough2<SYSCALL_DEF(nanosleep)>);
    REGISTER_SYSCALL_IMPL_X64(getitimer, SyscallPassthrough2<SYSCALL_DEF(getitimer)>);
    REGISTER_SYSCALL_IMPL_X64(setitimer, SyscallPassthrough3<SYSCALL_DEF(setitimer)>);
    REGISTER_SYSCALL_IMPL_X64(sendfile, SyscallPassthrough4<SYSCALL_DEF(sendfile)>);
    REGISTER_SYSCALL_IMPL_X64(accept, SyscallPassthrough3<SYSCALL_DEF(accept)>);
    REGISTER_SYSCALL_IMPL_X64(sendmsg, SyscallPassthrough3<SYSCALL_DEF(sendmsg)>);
    REGISTER_SYSCALL_IMPL_X64(recvmsg, SyscallPassthrough3<SYSCALL_DEF(recvmsg)>);
    REGISTER_SYSCALL_IMPL_X64(setsockopt, SyscallPassthrough5<SYSCALL_DEF(setsockopt)>);
    REGISTER_SYSCALL_IMPL_X64(getsockopt, SyscallPassthrough5<SYSCALL_DEF(getsockopt)>);
    REGISTER_SYSCALL_IMPL_X64(wait4, SyscallPassthrough4<SYSCALL_DEF(wait4)>);
    REGISTER_SYSCALL_IMPL_X64(semop, SyscallPassthrough3<SYSCALL_DEF(semop)>);
    REGISTER_SYSCALL_IMPL_X64(gettimeofday, SyscallPassthrough2<SYSCALL_DEF(gettimeofday)>);
    REGISTER_SYSCALL_IMPL_X64(getrlimit, SyscallPassthrough2<SYSCALL_DEF(getrlimit)>);
    REGISTER_SYSCALL_IMPL_X64(getrusage, SyscallPassthrough2<SYSCALL_DEF(getrusage)>);
    REGISTER_SYSCALL_IMPL_X64(sysinfo, SyscallPassthrough1<SYSCALL_DEF(sysinfo)>);
    REGISTER_SYSCALL_IMPL_X64(times, SyscallPassthrough1<SYSCALL_DEF(times)>);
    REGISTER_SYSCALL_IMPL_X64(rt_sigqueueinfo, SyscallPassthrough3<SYSCALL_DEF(rt_sigqueueinfo)>);
    REGISTER_SYSCALL_IMPL_X64(fstatfs, SyscallPassthrough2<SYSCALL_DEF(fstatfs)>);
    REGISTER_SYSCALL_IMPL_X64(sched_rr_get_interval, SyscallPassthrough2<SYSCALL_DEF(sched_rr_get_interval)>);
    REGISTER_SYSCALL_IMPL_X64(mlockall, SyscallPassthrough1<SYSCALL_DEF(mlockall)>);
    REGISTER_SYSCALL_IMPL_X64(munlockall, SyscallPassthrough0<SYSCALL_DEF(munlockall)>);
    REGISTER_SYSCALL_IMPL_X64(adjtimex, SyscallPassthrough1<SYSCALL_DEF(adjtimex)>);
    REGISTER_SYSCALL_IMPL_X64(setrlimit, SyscallPassthrough2<SYSCALL_DEF(setrlimit)>);
    REGISTER_SYSCALL_IMPL_X64(settimeofday, SyscallPassthrough2<SYSCALL_DEF(settimeofday)>);
    REGISTER_SYSCALL_IMPL_X64(readahead, SyscallPassthrough3<SYSCALL_DEF(readahead)>);
    REGISTER_SYSCALL_IMPL_X64(futex, SyscallPassthrough6<SYSCALL_DEF(futex)>);
    REGISTER_SYSCALL_IMPL_X64(io_getevents, SyscallPassthrough5<SYSCALL_DEF(io_getevents)>);
    REGISTER_SYSCALL_IMPL_X64(semtimedop, SyscallPassthrough4<SYSCALL_DEF(semtimedop)>);
    REGISTER_SYSCALL_IMPL_X64(timer_create, SyscallPassthrough3<SYSCALL_DEF(timer_create)>);
    REGISTER_SYSCALL_IMPL_X64(timer_settime, SyscallPassthrough4<SYSCALL_DEF(timer_settime)>);
    REGISTER_SYSCALL_IMPL_X64(timer_gettime, SyscallPassthrough2<SYSCALL_DEF(timer_gettime)>);
    REGISTER_SYSCALL_IMPL_X64(clock_settime, SyscallPassthrough2<SYSCALL_DEF(clock_settime)>);
    REGISTER_SYSCALL_IMPL_X64(clock_gettime, SyscallPassthrough2<SYSCALL_DEF(clock_gettime)>);
    REGISTER_SYSCALL_IMPL_X64(clock_getres, SyscallPassthrough2<SYSCALL_DEF(clock_getres)>);
    REGISTER_SYSCALL_IMPL_X64(clock_nanosleep, SyscallPassthrough4<SYSCALL_DEF(clock_nanosleep)>);
    REGISTER_SYSCALL_IMPL_X64(mq_open, SyscallPassthrough4<SYSCALL_DEF(mq_open)>);
    REGISTER_SYSCALL_IMPL_X64(mq_timedsend, SyscallPassthrough5<SYSCALL_DEF(mq_timedsend)>);
    REGISTER_SYSCALL_IMPL_X64(mq_timedreceive, SyscallPassthrough5<SYSCALL_DEF(mq_timedreceive)>);
    REGISTER_SYSCALL_IMPL_X64(mq_notify, SyscallPassthrough2<SYSCALL_DEF(mq_notify)>);
    REGISTER_SYSCALL_IMPL_X64(mq_getsetattr, SyscallPassthrough3<SYSCALL_DEF(mq_getsetattr)>);
    REGISTER_SYSCALL_IMPL_X64(waitid, SyscallPassthrough5<SYSCALL_DEF(waitid)>);
    REGISTER_SYSCALL_IMPL_X64(pselect6, SyscallPassthrough6<SYSCALL_DEF(pselect6)>);
    REGISTER_SYSCALL_IMPL_X64(ppoll, SyscallPassthrough5<SYSCALL_DEF(ppoll)>);
    REGISTER_SYSCALL_IMPL_X64(set_robust_list, SyscallPassthrough2<SYSCALL_DEF(set_robust_list)>);
    REGISTER_SYSCALL_IMPL_X64(get_robust_list, SyscallPassthrough3<SYSCALL_DEF(get_robust_list)>);
    REGISTER_SYSCALL_IMPL_X64(sync_file_range, SyscallPassthrough4<SYSCALL_DEF(sync_file_range)>);
    REGISTER_SYSCALL_IMPL_X64(vmsplice, SyscallPassthrough4<SYSCALL_DEF(vmsplice)>);
    REGISTER_SYSCALL_IMPL_X64(utimensat, SyscallPassthrough4<SYSCALL_DEF(utimensat)>);
    REGISTER_SYSCALL_IMPL_X64(fallocate, SyscallPassthrough4<SYSCALL_DEF(fallocate)>);
    REGISTER_SYSCALL_IMPL_X64(timerfd_settime, SyscallPassthrough4<SYSCALL_DEF(timerfd_settime)>);
    REGISTER_SYSCALL_IMPL_X64(timerfd_gettime, SyscallPassthrough2<SYSCALL_DEF(timerfd_gettime)>);
    REGISTER_SYSCALL_IMPL_X64(preadv, SyscallPassthrough5<SYSCALL_DEF(preadv)>);
    REGISTER_SYSCALL_IMPL_X64(pwritev, SyscallPassthrough5<SYSCALL_DEF(pwritev)>);
    REGISTER_SYSCALL_IMPL_X64(rt_tgsigqueueinfo, SyscallPassthrough4<SYSCALL_DEF(rt_tgsigqueueinfo)>);
    REGISTER_SYSCALL_IMPL_X64(recvmmsg, SyscallPassthrough5<SYSCALL_DEF(recvmmsg)>);
    REGISTER_SYSCALL_IMPL_X64(clock_adjtime, SyscallPassthrough2<SYSCALL_DEF(clock_adjtime)>);
    REGISTER_SYSCALL_IMPL_X64(sendmmsg, SyscallPassthrough4<SYSCALL_DEF(sendmmsg)>);
    REGISTER_SYSCALL_IMPL_X64(process_vm_readv, SyscallPassthrough6<SYSCALL_DEF(process_vm_readv)>);
    REGISTER_SYSCALL_IMPL_X64(process_vm_writev, SyscallPassthrough6<SYSCALL_DEF(process_vm_writev)>);
    REGISTER_SYSCALL_IMPL_X64(preadv2, SyscallPassthrough6<SYSCALL_DEF(preadv2)>);
    REGISTER_SYSCALL_IMPL_X64(pwritev2, SyscallPassthrough6<SYSCALL_DEF(pwritev2)>);
    REGISTER_SYSCALL_IMPL_X64(io_pgetevents, SyscallPassthrough6<SYSCALL_DEF(io_pgetevents)>);
    REGISTER_SYSCALL_IMPL_X64(pidfd_send_signal, SyscallPassthrough4<SYSCALL_DEF(pidfd_send_signal)>);
    REGISTER_SYSCALL_IMPL_X64(process_madvise, SyscallPassthrough5<SYSCALL_DEF(process_madvise)>);
    REGISTER_SYSCALL_IMPL_X64(fadvise64, SyscallPassthrough4<SYSCALL_DEF(fadvise64)>);
    if (Handler->IsHostKernelVersionAtLeast(6, 5, 0)) {
      REGISTER_SYSCALL_IMPL_X64(cachestat, SyscallPassthrough4<SYSCALL_DEF(cachestat)>);
    } else {
      REGISTER_SYSCALL_IMPL_X64(cachestat, UnimplementedSyscallSafe);
    }
    if (Handler->IsHostKernelVersionAtLeast(6, 6, 0)) {
      REGISTER_SYSCALL_IMPL_X64(fchmodat2, SyscallPassthrough4<SYSCALL_DEF(fchmodat2)>);
    } else {
      REGISTER_SYSCALL_IMPL_X64(fchmodat2, UnimplementedSyscallSafe);
    }
  }
} // namespace x64

namespace x32 {
  void RegisterPassthrough(FEX::HLE::SyscallHandler* Handler) {
    using namespace FEXCore::IR;
    RegisterCommon(Handler);
    REGISTER_SYSCALL_IMPL_X32(getuid32, SyscallPassthrough0<SYSCALL_DEF(getuid)>);
    REGISTER_SYSCALL_IMPL_X32(getgid32, SyscallPassthrough0<SYSCALL_DEF(getgid)>);
    REGISTER_SYSCALL_IMPL_X32(geteuid32, SyscallPassthrough0<SYSCALL_DEF(geteuid)>);
    REGISTER_SYSCALL_IMPL_X32(getegid32, SyscallPassthrough0<SYSCALL_DEF(getegid)>);
    REGISTER_SYSCALL_IMPL_X32(setreuid32, SyscallPassthrough2<SYSCALL_DEF(setreuid)>);
    REGISTER_SYSCALL_IMPL_X32(setregid32, SyscallPassthrough2<SYSCALL_DEF(setregid)>);
    REGISTER_SYSCALL_IMPL_X32(getgroups32, SyscallPassthrough2<SYSCALL_DEF(getgroups)>);
    REGISTER_SYSCALL_IMPL_X32(setgroups32, SyscallPassthrough2<SYSCALL_DEF(setgroups)>);
    REGISTER_SYSCALL_IMPL_X32(fchown32, SyscallPassthrough3<SYSCALL_DEF(fchown)>);
    REGISTER_SYSCALL_IMPL_X32(setresuid32, SyscallPassthrough3<SYSCALL_DEF(setresuid)>);
    REGISTER_SYSCALL_IMPL_X32(getresuid32, SyscallPassthrough3<SYSCALL_DEF(getresuid)>);
    REGISTER_SYSCALL_IMPL_X32(setresgid32, SyscallPassthrough3<SYSCALL_DEF(setresgid)>);
    REGISTER_SYSCALL_IMPL_X32(getresgid32, SyscallPassthrough3<SYSCALL_DEF(getresgid)>);
    REGISTER_SYSCALL_IMPL_X32(setuid32, SyscallPassthrough1<SYSCALL_DEF(setuid)>);
    REGISTER_SYSCALL_IMPL_X32(setgid32, SyscallPassthrough1<SYSCALL_DEF(setgid)>);
    REGISTER_SYSCALL_IMPL_X32(setfsuid32, SyscallPassthrough1<SYSCALL_DEF(setfsuid)>);
    REGISTER_SYSCALL_IMPL_X32(setfsgid32, SyscallPassthrough1<SYSCALL_DEF(setfsgid)>);
    REGISTER_SYSCALL_IMPL_X32(sendfile64, SyscallPassthrough4<SYSCALL_DEF(sendfile)>);
    REGISTER_SYSCALL_IMPL_X32(clock_gettime64, SyscallPassthrough2<SYSCALL_DEF(clock_gettime)>);
    REGISTER_SYSCALL_IMPL_X32(clock_settime64, SyscallPassthrough2<SYSCALL_DEF(clock_settime)>);
    REGISTER_SYSCALL_IMPL_X32(clock_adjtime64, SyscallPassthrough2<SYSCALL_DEF(clock_adjtime)>);
    REGISTER_SYSCALL_IMPL_X32(clock_getres_time64, SyscallPassthrough2<SYSCALL_DEF(clock_getres)>);
    REGISTER_SYSCALL_IMPL_X32(clock_nanosleep_time64, SyscallPassthrough4<SYSCALL_DEF(clock_nanosleep)>);
    REGISTER_SYSCALL_IMPL_X32(timer_gettime64, SyscallPassthrough2<SYSCALL_DEF(timer_gettime)>);
    REGISTER_SYSCALL_IMPL_X32(timer_settime64, SyscallPassthrough4<SYSCALL_DEF(timer_settime)>);
    REGISTER_SYSCALL_IMPL_X32(timerfd_gettime64, SyscallPassthrough2<SYSCALL_DEF(timerfd_gettime)>);
    REGISTER_SYSCALL_IMPL_X32(timerfd_settime64, SyscallPassthrough4<SYSCALL_DEF(timerfd_settime)>);
    REGISTER_SYSCALL_IMPL_X32(utimensat_time64, SyscallPassthrough4<SYSCALL_DEF(utimensat)>);
    REGISTER_SYSCALL_IMPL_X32(ppoll_time64, SyscallPassthrough5<SYSCALL_DEF(ppoll)>);
    REGISTER_SYSCALL_IMPL_X32(io_pgetevents_time64, SyscallPassthrough6<SYSCALL_DEF(io_pgetevents)>);
    REGISTER_SYSCALL_IMPL_X32(mq_timedsend_time64, SyscallPassthrough5<SYSCALL_DEF(mq_timedsend)>);
    REGISTER_SYSCALL_IMPL_X32(mq_timedreceive_time64, SyscallPassthrough5<SYSCALL_DEF(mq_timedreceive)>);
    REGISTER_SYSCALL_IMPL_X32(semtimedop_time64, SyscallPassthrough4<SYSCALL_DEF(semtimedop)>);
    REGISTER_SYSCALL_IMPL_X32(futex_time64, SyscallPassthrough6<SYSCALL_DEF(futex)>);
    REGISTER_SYSCALL_IMPL_X32(sched_rr_get_interval_time64, SyscallPassthrough2<SYSCALL_DEF(sched_rr_get_interval)>);
  }
} // namespace x32
} // namespace FEX::HLE
