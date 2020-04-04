#pragma once
#include <stdint.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <linux/futex.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {
  uint64_t Getpid(FEXCore::Core::InternalThreadState *Thread);
  uint64_t Clone(FEXCore::Core::InternalThreadState *Thread, uint32_t flags, void *stack, pid_t *parent_tid, pid_t *child_tid, void *tls);
  uint64_t Execve(FEXCore::Core::InternalThreadState *Thread, const char *pathname, char *const argv[], char *const envp[]);
  uint64_t Exit(FEXCore::Core::InternalThreadState *Thread, int status);
  uint64_t Wait4(FEXCore::Core::InternalThreadState *Thread, pid_t pid, int *wstatus, int options, struct rusage *rusage);
  uint64_t Getuid(FEXCore::Core::InternalThreadState *Thread);
  uint64_t Getgid(FEXCore::Core::InternalThreadState *Thread);
  uint64_t Setuid(FEXCore::Core::InternalThreadState *Thread, uid_t uid);
  uint64_t Setgid(FEXCore::Core::InternalThreadState *Thread, gid_t gid);
  uint64_t Geteuid(FEXCore::Core::InternalThreadState *Thread);
  uint64_t Getegid(FEXCore::Core::InternalThreadState *Thread);
  uint64_t Setregid(FEXCore::Core::InternalThreadState *Thread, gid_t rgid, gid_t egid);
  uint64_t Setresuid(FEXCore::Core::InternalThreadState *Thread, uid_t ruid, uid_t euid, uid_t suid);
  uint64_t Getresuid(FEXCore::Core::InternalThreadState *Thread, uid_t *ruid, uid_t *euid, uid_t *suid);
  uint64_t Setresgid(FEXCore::Core::InternalThreadState *Thread, gid_t rgid, gid_t egid, gid_t sgid);
  uint64_t Getresgid(FEXCore::Core::InternalThreadState *Thread, gid_t *rgid, gid_t *egid, gid_t *sgid);
  uint64_t Prctl(FEXCore::Core::InternalThreadState *Thread, int option, unsigned long arg2, unsigned long arg3, unsigned long arg4, unsigned long arg5);
  uint64_t Arch_Prctl(FEXCore::Core::InternalThreadState *Thread, int code, unsigned long addr);
  uint64_t Gettid(FEXCore::Core::InternalThreadState *Thread);
  uint64_t Futex(FEXCore::Core::InternalThreadState *Thread, int *uaddr, int futex_op, int val, const struct timespec *timeout, int *uaddr2, uint32_t val3);
  uint64_t Set_tid_address(FEXCore::Core::InternalThreadState *Thread, int *tidptr);
  uint64_t Exit_group(FEXCore::Core::InternalThreadState *Thread, int status);
  uint64_t Set_robust_list(FEXCore::Core::InternalThreadState *Thread, struct robust_list_head *head, size_t len);
  uint64_t Prlimit64(FEXCore::Core::InternalThreadState *Thread, pid_t pid, int resource, const struct rlimit *new_limit, struct rlimit *old_limit);
}
