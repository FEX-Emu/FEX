#pragma once
#include <cstdint>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <syscall.h>
#include <sys/stat.h>
#include <unistd.h>

namespace FHU::Syscalls {
#ifndef MAP_FIXED_NOREPLACE
#define MAP_FIXED_NOREPLACE 0x100000
#endif

#ifndef SEM_STAT_ANY
#define SEM_STAT_ANY 20
#endif

#ifndef SHM_STAT_ANY
#define SHM_STAT_ANY 15
#endif

#ifndef MSG_STAT_ANY
#define MSG_STAT_ANY 13
#endif

#ifndef CLONE_PIDFD
#define CLONE_PIDFD 0x00001000
#endif

#ifndef SYS_statx
#define SYS_statx 291
#endif

inline int32_t getcpu(uint32_t *cpu, uint32_t *node) {
  // Third argument is unused
#if defined(HAS_SYSCALL_GETCPU) && HAS_SYSCALL_GETCPU
  return ::getcpu(cpu, node, nullptr);
#else
  return ::syscall(SYS_getcpu, cpu, node, nullptr);
#endif
}

inline int32_t gettid() {
#if defined(HAS_SYSCALL_GETTID) && HAS_SYSCALL_GETTID
  return ::gettid();
#else
  return ::syscall(SYS_gettid);
#endif
}

inline int32_t tgkill(pid_t tgid, pid_t tid, int sig) {
#if defined(HAS_SYSCALL_GETTID) && HAS_SYSCALL_GETTID
  return ::tgkill(tggid, tid, sig);
#else
  return ::syscall(SYS_tgkill, tgid, tid, sig);
#endif
}

inline int32_t statx(int dirfd, const char *pathname, int32_t flags, uint32_t mask, void *statxbuf) {
#if defined(HAS_SYSCALL_STATX) && HAS_SYSCALL_STATX
  return ::statx(dirfd, pathname, flags, mask, statxbuf);
#else
  return ::syscall(SYS_statx, dirfd, pathname, flags, mask, statxbuf);
#endif
}

inline int32_t renameat2(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, unsigned int flags) {
#if defined(HAS_SYSCALL_STATX) && HAS_SYSCALL_STATX
  return ::renameat2(olddirfd, oldpath, newdirfd, newpath, flags);
#else
  return ::syscall(SYS_renameat2, olddirfd, oldpath, newdirfd, newpath, flags);
#endif
}


}
