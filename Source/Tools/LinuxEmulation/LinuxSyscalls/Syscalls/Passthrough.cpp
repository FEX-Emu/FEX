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
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(read, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY, SyscallPassthrough3<SYSCALL_DEF(read)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(write, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(write)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(lseek, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(lseek)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(sched_yield, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough0<SYSCALL_DEF(sched_yield)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(msync, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(msync)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(mincore, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(mincore)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(shmget, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(shmget)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(shmctl, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(shmctl)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(getpid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough0<SYSCALL_DEF(getpid)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(socket, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(socket)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(connect, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(connect)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(sendto, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough6<SYSCALL_DEF(sendto)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(recvfrom, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough6<SYSCALL_DEF(recvfrom)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(shutdown, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(shutdown)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(bind, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY, SyscallPassthrough3<SYSCALL_DEF(bind)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(listen, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(listen)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(getsockname, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(getsockname)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(getpeername, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(getpeername)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(socketpair, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough4<SYSCALL_DEF(socketpair)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(kill, SyscallFlags::DEFAULT, SyscallPassthrough2<SYSCALL_DEF(kill)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(semget, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(semget)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(msgget, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(msgget)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(msgsnd, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough4<SYSCALL_DEF(msgsnd)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(msgrcv, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough5<SYSCALL_DEF(msgrcv)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(msgctl, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(msgctl)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(flock, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(flock)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(fsync, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough1<SYSCALL_DEF(fsync)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(fdatasync, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough1<SYSCALL_DEF(fdatasync)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(truncate, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(truncate)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(ftruncate, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(ftruncate)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(getcwd, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(getcwd)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(chdir, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough1<SYSCALL_DEF(chdir)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(fchdir, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough1<SYSCALL_DEF(fchdir)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(fchmod, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(fchmod)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(fchown, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(fchown)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(umask, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough1<SYSCALL_DEF(umask)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(getuid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough0<SYSCALL_DEF(getuid)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(syslog, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(syslog)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(getgid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough0<SYSCALL_DEF(getgid)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(setuid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough1<SYSCALL_DEF(setuid)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(setgid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough1<SYSCALL_DEF(setgid)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(geteuid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough0<SYSCALL_DEF(geteuid)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(getegid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough0<SYSCALL_DEF(getegid)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(setpgid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(setpgid)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(getppid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough0<SYSCALL_DEF(getppid)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(setsid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough0<SYSCALL_DEF(setsid)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(setreuid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(setreuid)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(setregid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(setregid)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(getgroups, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(getgroups)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(setgroups, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(setgroups)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(setresuid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(setresuid)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(getresuid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(getresuid)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(setresgid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(setresgid)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(getresgid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(getresgid)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(getpgid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough1<SYSCALL_DEF(getpgid)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(setfsuid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough1<SYSCALL_DEF(setfsuid)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(setfsgid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough1<SYSCALL_DEF(setfsgid)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(getsid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough1<SYSCALL_DEF(getsid)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(capget, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(capget)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(capset, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(capset)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(getpriority, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(getpriority)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(setpriority, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(setpriority)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(sched_setparam, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(sched_setparam)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(sched_getparam, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(sched_getparam)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(sched_setscheduler, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(sched_setscheduler)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(sched_getscheduler, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough1<SYSCALL_DEF(sched_getscheduler)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(sched_get_priority_max, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough1<SYSCALL_DEF(sched_get_priority_max)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(sched_get_priority_min, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough1<SYSCALL_DEF(sched_get_priority_min)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(mlock, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(mlock)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(munlock, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(munlock)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(pivot_root, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(pivot_root)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(chroot, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough1<SYSCALL_DEF(chroot)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(sync, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY, SyscallPassthrough0<SYSCALL_DEF(sync)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(acct, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY, SyscallPassthrough1<SYSCALL_DEF(acct)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(mount, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough5<SYSCALL_DEF(mount)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(umount2, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(umount2)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(swapon, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(swapon)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(swapoff, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough1<SYSCALL_DEF(swapoff)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(gettid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough0<SYSCALL_DEF(gettid)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(fsetxattr, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough5<SYSCALL_DEF(fsetxattr)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(fgetxattr, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough4<SYSCALL_DEF(fgetxattr)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(flistxattr, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(flistxattr)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(fremovexattr, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(fremovexattr)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(tkill, SyscallFlags::DEFAULT, SyscallPassthrough2<SYSCALL_DEF(tkill)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(sched_setaffinity, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(sched_setaffinity)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(sched_getaffinity, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(sched_getaffinity)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(io_setup, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(io_setup)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(io_destroy, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough1<SYSCALL_DEF(io_destroy)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(io_submit, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(io_submit)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(io_cancel, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(io_cancel)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(remap_file_pages, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough5<SYSCALL_DEF(remap_file_pages)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(fadvise64, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough4<SYSCALL_DEF(fadvise64)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(timer_getoverrun, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough1<SYSCALL_DEF(timer_getoverrun)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(timer_delete, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough1<SYSCALL_DEF(timer_delete)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(tgkill, SyscallFlags::DEFAULT, SyscallPassthrough3<SYSCALL_DEF(tgkill)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(mbind, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough6<SYSCALL_DEF(mbind)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(set_mempolicy, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(set_mempolicy)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(get_mempolicy, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough5<SYSCALL_DEF(get_mempolicy)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(mq_unlink, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough1<SYSCALL_DEF(mq_unlink)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(add_key, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough5<SYSCALL_DEF(add_key)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(request_key, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough4<SYSCALL_DEF(request_key)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(keyctl, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough5<SYSCALL_DEF(keyctl)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(ioprio_set, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(ioprio_set)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(ioprio_get, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(ioprio_get)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(inotify_add_watch, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(inotify_add_watch)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(inotify_rm_watch, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(inotify_rm_watch)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(migrate_pages, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough4<SYSCALL_DEF(migrate_pages)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(mkdirat, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(mkdirat)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(mknodat, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough4<SYSCALL_DEF(mknodat)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(fchownat, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough5<SYSCALL_DEF(fchownat)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(unlinkat, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(unlinkat)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(renameat, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough4<SYSCALL_DEF(renameat)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(linkat, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough5<SYSCALL_DEF(linkat)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(symlinkat, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(symlinkat)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(fchmodat, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(fchmodat)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(unshare, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough1<SYSCALL_DEF(unshare)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(splice, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough6<SYSCALL_DEF(splice)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(tee, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY, SyscallPassthrough4<SYSCALL_DEF(tee)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(move_pages, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough6<SYSCALL_DEF(move_pages)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(timerfd_create, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(timerfd_create)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(accept4, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough4<SYSCALL_DEF(accept4)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(eventfd2, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(eventfd2)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(epoll_create1, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough1<SYSCALL_DEF(epoll_create1)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(inotify_init1, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough1<SYSCALL_DEF(inotify_init1)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(fanotify_init, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(fanotify_init)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(fanotify_mark, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough5<SYSCALL_DEF(fanotify_mark)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(prlimit_64, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough4<SYSCALL_DEF(prlimit_64)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(name_to_handle_at, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough5<SYSCALL_DEF(name_to_handle_at)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(open_by_handle_at, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(open_by_handle_at)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(syncfs, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough1<SYSCALL_DEF(syncfs)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(setns, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(setns)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(getcpu, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(getcpu)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(kcmp, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY, SyscallPassthrough5<SYSCALL_DEF(kcmp)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(sched_setattr, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(sched_setattr)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(sched_getattr, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough4<SYSCALL_DEF(sched_getattr)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(renameat2, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough5<SYSCALL_DEF(renameat2)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(getrandom, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(getrandom)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(memfd_create, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(memfd_create)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(membarrier, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(membarrier)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(mlock2, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(mlock2)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(copy_file_range, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough6<SYSCALL_DEF(copy_file_range)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(pkey_mprotect, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough4<SYSCALL_DEF(pkey_mprotect)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(pkey_alloc, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(pkey_alloc)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(pkey_free, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough1<SYSCALL_DEF(pkey_free)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(io_uring_setup, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(io_uring_setup)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(io_uring_enter, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough6<SYSCALL_DEF(io_uring_enter)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(io_uring_register, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough4<SYSCALL_DEF(io_uring_register)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(open_tree, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(open_tree)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(move_mount, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough5<SYSCALL_DEF(move_mount)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(fsopen, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(fsopen)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(fsconfig, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough5<SYSCALL_DEF(fsconfig)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(fsmount, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(fsmount)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(fspick, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(fspick)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(pidfd_open, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(pidfd_open)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(pidfd_getfd, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(pidfd_getfd)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(mount_setattr, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough5<SYSCALL_DEF(mount_setattr)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(quotactl_fd, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough4<SYSCALL_DEF(quotactl_fd)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(landlock_create_ruleset, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough3<SYSCALL_DEF(landlock_create_ruleset)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(landlock_add_rule, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough4<SYSCALL_DEF(landlock_add_rule)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(landlock_restrict_self, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(landlock_restrict_self)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(memfd_secret, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough1<SYSCALL_DEF(memfd_secret)>);
  REGISTER_SYSCALL_IMPL_PASS_FLAGS(process_mrelease, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                   SyscallPassthrough2<SYSCALL_DEF(process_mrelease)>);
  if (Handler->IsHostKernelVersionAtLeast(5, 16, 0)) {
    REGISTER_SYSCALL_IMPL_PASS_FLAGS(futex_waitv, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                     SyscallPassthrough5<SYSCALL_DEF(futex_waitv)>);
  } else {
    REGISTER_SYSCALL_IMPL(futex_waitv, UnimplementedSyscallSafe);
  }
  if (Handler->IsHostKernelVersionAtLeast(5, 17, 0)) {
    REGISTER_SYSCALL_IMPL_PASS_FLAGS(set_mempolicy_home_node, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                     SyscallPassthrough4<SYSCALL_DEF(set_mempolicy_home_node)>);
  } else {
    REGISTER_SYSCALL_IMPL(set_mempolicy_home_node, UnimplementedSyscallSafe);
  }

  if (Handler->IsHostKernelVersionAtLeast(6, 8, 0)) {
    REGISTER_SYSCALL_IMPL_PASS_FLAGS(futex_wake, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                     SyscallPassthrough4<SYSCALL_DEF(futex_wake)>);
    REGISTER_SYSCALL_IMPL_PASS_FLAGS(futex_wait, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                     SyscallPassthrough6<SYSCALL_DEF(futex_wait)>);
    REGISTER_SYSCALL_IMPL_PASS_FLAGS(futex_requeue, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                     SyscallPassthrough4<SYSCALL_DEF(futex_requeue)>);
    REGISTER_SYSCALL_IMPL_PASS_FLAGS(statmount, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                     SyscallPassthrough4<SYSCALL_DEF(statmount)>);
    REGISTER_SYSCALL_IMPL_PASS_FLAGS(listmount, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                     SyscallPassthrough4<SYSCALL_DEF(listmount)>);
    REGISTER_SYSCALL_IMPL_PASS_FLAGS(lsm_get_self_attr, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                     SyscallPassthrough4<SYSCALL_DEF(lsm_get_self_attr)>);
    REGISTER_SYSCALL_IMPL_PASS_FLAGS(lsm_set_self_attr, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                     SyscallPassthrough4<SYSCALL_DEF(lsm_set_self_attr)>);
    REGISTER_SYSCALL_IMPL_PASS_FLAGS(lsm_list_modules, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                     SyscallPassthrough3<SYSCALL_DEF(lsm_list_modules)>);
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
    REGISTER_SYSCALL_IMPL_PASS_FLAGS(mseal, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                     SyscallPassthrough3<SYSCALL_DEF(mseal)>);
  } else {
    REGISTER_SYSCALL_IMPL(mseal, UnimplementedSyscallSafe);
  }
}

namespace x64 {
  void RegisterPassthrough(FEX::HLE::SyscallHandler* Handler) {
    using namespace FEXCore::IR;
    RegisterCommon(Handler);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(ioctl, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough3<SYSCALL_DEF(ioctl)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(pread_64, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough4<SYSCALL_DEF(pread_64)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(pwrite_64, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough4<SYSCALL_DEF(pwrite_64)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(readv, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough3<SYSCALL_DEF(readv)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(writev, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough3<SYSCALL_DEF(writev)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(dup, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough1<SYSCALL_DEF(dup)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(nanosleep, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough2<SYSCALL_DEF(nanosleep)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(getitimer, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough2<SYSCALL_DEF(getitimer)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(setitimer, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough3<SYSCALL_DEF(setitimer)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(sendfile, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough4<SYSCALL_DEF(sendfile)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(accept, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough3<SYSCALL_DEF(accept)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(sendmsg, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough3<SYSCALL_DEF(sendmsg)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(recvmsg, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough3<SYSCALL_DEF(recvmsg)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(setsockopt, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough5<SYSCALL_DEF(setsockopt)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(getsockopt, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough5<SYSCALL_DEF(getsockopt)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(wait4, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough4<SYSCALL_DEF(wait4)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(semop, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough3<SYSCALL_DEF(semop)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(gettimeofday, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough2<SYSCALL_DEF(gettimeofday)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(getrlimit, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough2<SYSCALL_DEF(getrlimit)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(getrusage, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough2<SYSCALL_DEF(getrusage)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(sysinfo, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough1<SYSCALL_DEF(sysinfo)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(times, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough1<SYSCALL_DEF(times)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(rt_sigqueueinfo, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough3<SYSCALL_DEF(rt_sigqueueinfo)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(fstatfs, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough2<SYSCALL_DEF(fstatfs)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(sched_rr_get_interval, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough2<SYSCALL_DEF(sched_rr_get_interval)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(mlockall, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough1<SYSCALL_DEF(mlockall)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(munlockall, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough0<SYSCALL_DEF(munlockall)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(adjtimex, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough1<SYSCALL_DEF(adjtimex)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(setrlimit, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough2<SYSCALL_DEF(setrlimit)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(settimeofday, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough2<SYSCALL_DEF(settimeofday)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(readahead, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough3<SYSCALL_DEF(readahead)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(futex, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough6<SYSCALL_DEF(futex)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(io_getevents, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough5<SYSCALL_DEF(io_getevents)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(semtimedop, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough4<SYSCALL_DEF(semtimedop)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(timer_create, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough3<SYSCALL_DEF(timer_create)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(timer_settime, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough4<SYSCALL_DEF(timer_settime)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(timer_gettime, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough2<SYSCALL_DEF(timer_gettime)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(clock_settime, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough2<SYSCALL_DEF(clock_settime)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(clock_gettime, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough2<SYSCALL_DEF(clock_gettime)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(clock_getres, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough2<SYSCALL_DEF(clock_getres)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(clock_nanosleep, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough4<SYSCALL_DEF(clock_nanosleep)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(mq_open, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough4<SYSCALL_DEF(mq_open)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(mq_timedsend, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough5<SYSCALL_DEF(mq_timedsend)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(mq_timedreceive, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough5<SYSCALL_DEF(mq_timedreceive)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(mq_notify, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough2<SYSCALL_DEF(mq_notify)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(mq_getsetattr, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough3<SYSCALL_DEF(mq_getsetattr)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(waitid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough5<SYSCALL_DEF(waitid)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(pselect6, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough6<SYSCALL_DEF(pselect6)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(ppoll, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough5<SYSCALL_DEF(ppoll)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(set_robust_list, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough2<SYSCALL_DEF(set_robust_list)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(get_robust_list, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough3<SYSCALL_DEF(get_robust_list)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(sync_file_range, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough4<SYSCALL_DEF(sync_file_range)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(vmsplice, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough4<SYSCALL_DEF(vmsplice)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(utimensat, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough4<SYSCALL_DEF(utimensat)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(fallocate, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough4<SYSCALL_DEF(fallocate)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(timerfd_settime, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough4<SYSCALL_DEF(timerfd_settime)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(timerfd_gettime, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough2<SYSCALL_DEF(timerfd_gettime)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(preadv, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough5<SYSCALL_DEF(preadv)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(pwritev, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough5<SYSCALL_DEF(pwritev)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(rt_tgsigqueueinfo, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough4<SYSCALL_DEF(rt_tgsigqueueinfo)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(recvmmsg, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough5<SYSCALL_DEF(recvmmsg)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(clock_adjtime, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough2<SYSCALL_DEF(clock_adjtime)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(sendmmsg, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough4<SYSCALL_DEF(sendmmsg)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(process_vm_readv, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough6<SYSCALL_DEF(process_vm_readv)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(process_vm_writev, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough6<SYSCALL_DEF(process_vm_writev)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(preadv2, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough6<SYSCALL_DEF(preadv2)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(pwritev2, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough6<SYSCALL_DEF(pwritev2)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(io_pgetevents, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough6<SYSCALL_DEF(io_pgetevents)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(pidfd_send_signal, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough4<SYSCALL_DEF(pidfd_send_signal)>);
    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(process_madvise, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                         SyscallPassthrough5<SYSCALL_DEF(process_madvise)>);
    if (Handler->IsHostKernelVersionAtLeast(6, 5, 0)) {
      REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(cachestat, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                           SyscallPassthrough4<SYSCALL_DEF(cachestat)>);
    } else {
      REGISTER_SYSCALL_IMPL_X64(cachestat, UnimplementedSyscallSafe);
    }
    if (Handler->IsHostKernelVersionAtLeast(6, 6, 0)) {
      REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(fchmodat2, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                           SyscallPassthrough4<SYSCALL_DEF(fchmodat2)>);
    } else {
      REGISTER_SYSCALL_IMPL_X64(fchmodat2, UnimplementedSyscallSafe);
    }
  }
} // namespace x64

namespace x32 {
  void RegisterPassthrough(FEX::HLE::SyscallHandler* Handler) {
    using namespace FEXCore::IR;
    RegisterCommon(Handler);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(getuid32, getuid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough0<SYSCALL_DEF(getuid)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(getgid32, getgid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough0<SYSCALL_DEF(getgid)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(geteuid32, geteuid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough0<SYSCALL_DEF(geteuid)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(getegid32, getegid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough0<SYSCALL_DEF(getegid)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(setreuid32, setreuid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough2<SYSCALL_DEF(setreuid)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(setregid32, setregid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough2<SYSCALL_DEF(setregid)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(getgroups32, getgroups, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough2<SYSCALL_DEF(getgroups)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(setgroups32, setgroups, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough2<SYSCALL_DEF(setgroups)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(fchown32, fchown, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough3<SYSCALL_DEF(fchown)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(setresuid32, setresuid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough3<SYSCALL_DEF(setresuid)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(getresuid32, getresuid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough3<SYSCALL_DEF(getresuid)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(setresgid32, setresgid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough3<SYSCALL_DEF(setresgid)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(getresgid32, getresgid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough3<SYSCALL_DEF(getresgid)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(setuid32, setuid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough1<SYSCALL_DEF(setuid)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(setgid32, setgid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough1<SYSCALL_DEF(setgid)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(setfsuid32, setfsuid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough1<SYSCALL_DEF(setfsuid)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(setfsgid32, setfsgid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough1<SYSCALL_DEF(setfsgid)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(sendfile64, sendfile, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough4<SYSCALL_DEF(sendfile)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(clock_gettime64, clock_gettime, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough2<SYSCALL_DEF(clock_gettime)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(clock_settime64, clock_settime, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough2<SYSCALL_DEF(clock_settime)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(clock_adjtime64, clock_adjtime, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough2<SYSCALL_DEF(clock_adjtime)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(clock_getres_time64, clock_getres, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough2<SYSCALL_DEF(clock_getres)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(clock_nanosleep_time64, clock_nanosleep,
                                                SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough4<SYSCALL_DEF(clock_nanosleep)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(timer_gettime64, timer_gettime, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough2<SYSCALL_DEF(timer_gettime)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(timer_settime64, timer_settime, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough4<SYSCALL_DEF(timer_settime)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(timerfd_gettime64, timerfd_gettime, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough2<SYSCALL_DEF(timerfd_gettime)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(timerfd_settime64, timerfd_settime, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough4<SYSCALL_DEF(timerfd_settime)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(utimensat_time64, utimensat, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough4<SYSCALL_DEF(utimensat)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(ppoll_time64, ppoll, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough5<SYSCALL_DEF(ppoll)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(io_pgetevents_time64, io_pgetevents, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough6<SYSCALL_DEF(io_pgetevents)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(mq_timedsend_time64, mq_timedsend, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough5<SYSCALL_DEF(mq_timedsend)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(mq_timedreceive_time64, mq_timedreceive,
                                                SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough5<SYSCALL_DEF(mq_timedreceive)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(semtimedop_time64, semtimedop, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough4<SYSCALL_DEF(semtimedop)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(futex_time64, futex, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough6<SYSCALL_DEF(futex)>);
    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(sched_rr_get_interval_time64, sched_rr_get_interval,
                                                SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                                                SyscallPassthrough2<SYSCALL_DEF(sched_rr_get_interval)>);
  }
} // namespace x32
} // namespace FEX::HLE
