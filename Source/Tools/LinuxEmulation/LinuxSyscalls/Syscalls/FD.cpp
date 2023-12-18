// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x64/Syscalls.h"
#include "LinuxSyscalls/x32/Syscalls.h"

#include <FEXCore/IR/IR.h>

#include <FEXHeaderUtils/Syscalls.h>

#include <fcntl.h>
#include <stdint.h>
#include <sys/file.h>
#include <sys/eventfd.h>
#include <sys/inotify.h>
#include <sys/mman.h>
#include <sys/timerfd.h>
#include <poll.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include <sys/syscall.h>

namespace FEX::HLE {
  void RegisterFD(FEX::HLE::SyscallHandler *Handler) {
    using namespace FEXCore::IR;

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(read, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int fd, void *buf, size_t count) -> uint64_t {
      uint64_t Result = ::read(fd, buf, count);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(write, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int fd, void *buf, size_t count) -> uint64_t {
      uint64_t Result = ::write(fd, buf, count);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_FLAGS(open, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, const char *pathname, int flags, uint32_t mode) -> uint64_t {
      flags = FEX::HLE::RemapFromX86Flags(flags);
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.Open(pathname, flags, mode);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_FLAGS(close, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int fd) -> uint64_t {
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.Close(fd);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(chown, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, const char *pathname, uid_t owner, gid_t group) -> uint64_t {
      uint64_t Result = ::chown(pathname, owner, group);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(fchown, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int fd, uid_t owner, gid_t group) -> uint64_t {
      uint64_t Result = ::fchown(fd, owner, group);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(lchown, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, const char *pathname, uid_t owner, gid_t group) -> uint64_t {
      uint64_t Result = ::lchown(pathname, owner, group);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(lseek, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int fd, uint64_t offset, int whence) -> uint64_t {
      uint64_t Result = ::lseek(fd, offset, whence);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_FLAGS(access, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, const char *pathname, int mode) -> uint64_t {
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.Access(pathname, mode);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(pipe, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int pipefd[2]) -> uint64_t {
      uint64_t Result = ::pipe(pipefd);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_FLAGS(dup3, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame* Frame, int oldfd, int newfd, int flags) -> uint64_t {
      flags = FEX::HLE::RemapFromX86Flags(flags);
      uint64_t Result = ::dup3(oldfd, newfd, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(flock, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int fd, int operation) -> uint64_t {
      uint64_t Result = ::flock(fd, operation);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(fsync, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int fd) -> uint64_t {
      uint64_t Result = ::fsync(fd);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(fdatasync, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int fd) -> uint64_t {
      uint64_t Result = ::fdatasync(fd);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(ftruncate, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int fd, off_t length) -> uint64_t {
      uint64_t Result = ::ftruncate(fd, length);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(fchmod, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int fd, int mode) -> uint64_t {
      uint64_t Result = ::fchmod(fd, mode);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(fadvise64, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int fd, off_t offset, off_t len, int advice) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(fadvise64), fd, offset, len, advice);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(inotify_init, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      uint64_t Result = ::inotify_init();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(inotify_add_watch, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int fd, const char *pathname, uint32_t mask) -> uint64_t {
      uint64_t Result = ::inotify_add_watch(fd, pathname, mask);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(inotify_rm_watch, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int fd, int wd) -> uint64_t {
      uint64_t Result = ::inotify_rm_watch(fd, wd);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_FLAGS(openat, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int dirfs, const char *pathname, int flags, uint32_t mode) -> uint64_t {
      flags = FEX::HLE::RemapFromX86Flags(flags);
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.Openat(dirfs, pathname, flags, mode);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(mkdirat, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int dirfd, const char *pathname, mode_t mode) -> uint64_t {
      uint64_t Result = ::mkdirat(dirfd, pathname, mode);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(mknodat, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int dirfd, const char *pathname, mode_t mode, dev_t dev) -> uint64_t {
      uint64_t Result = ::mknodat(dirfd, pathname, mode, dev);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(fchownat, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int dirfd, const char *pathname, uid_t owner, gid_t group, int flags) -> uint64_t {
      // Flags don't need remapped
      uint64_t Result = ::fchownat(dirfd, pathname, owner, group, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(unlinkat, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int dirfd, const char *pathname, int flags) -> uint64_t {
      // Flags don't need remapped
      uint64_t Result = ::unlinkat(dirfd, pathname, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(renameat, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int olddirfd, const char *oldpath, int newdirfd, const char *newpath) -> uint64_t {
      uint64_t Result = ::renameat(olddirfd, oldpath, newdirfd, newpath);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(linkat, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags) -> uint64_t {
      // Flags don't need remapped
      uint64_t Result = ::linkat(olddirfd, oldpath, newdirfd, newpath, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(symlinkat, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, const char *target, int newdirfd, const char *linkpath) -> uint64_t {
      uint64_t Result = ::symlinkat(target, newdirfd, linkpath);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_FLAGS(readlinkat, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int dirfd, const char *pathname, char *buf, size_t bufsiz) -> uint64_t {
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.Readlinkat(dirfd, pathname, buf, bufsiz);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(fchmodat, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int dirfd, const char *pathname, mode_t mode) -> uint64_t {
      uint64_t Result = syscall(SYSCALL_DEF(fchmodat), dirfd, pathname, mode);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_FLAGS(faccessat, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int dirfd, const char *pathname, int mode) -> uint64_t {
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.FAccessat(dirfd, pathname, mode);
      SYSCALL_ERRNO();
    });

    if (Handler->IsHostKernelVersionAtLeast(5, 8, 0)) {
      // Only exists on kernel 5.8+
      REGISTER_SYSCALL_IMPL_FLAGS(faccessat2, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
        [](FEXCore::Core::CpuStateFrame *Frame, int dirfd, const char *pathname, int mode, int flags) -> uint64_t {
        uint64_t Result = FEX::HLE::_SyscallHandler->FM.FAccessat2(dirfd, pathname, mode, flags);
        SYSCALL_ERRNO();
      });

      REGISTER_SYSCALL_IMPL_PASS_FLAGS(pidfd_getfd, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
        [](FEXCore::Core::CpuStateFrame *Frame, int pidfd, int fd, unsigned int flags) -> uint64_t {
        uint64_t Result = ::syscall(SYSCALL_DEF(pidfd_getfd), pidfd, fd, flags);
        SYSCALL_ERRNO();
      });

      REGISTER_SYSCALL_IMPL_FLAGS(openat2, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
        [](FEXCore::Core::CpuStateFrame *Frame, int dirfs, const char *pathname, struct open_how *how, size_t usize) -> uint64_t {
        open_how HostHow{};
        size_t HostSize = std::min(sizeof(open_how), usize);
        memcpy(&HostHow, how, HostSize);

        HostHow.flags = FEX::HLE::RemapFromX86Flags(HostHow.flags);
        uint64_t Result = FEX::HLE::_SyscallHandler->FM.Openat2(dirfs, pathname, &HostHow, HostSize);
        SYSCALL_ERRNO();
      });
    }
    else {
      REGISTER_SYSCALL_IMPL(faccessat2, UnimplementedSyscallSafe);
      REGISTER_SYSCALL_IMPL(pidfd_getfd, UnimplementedSyscallSafe);
      REGISTER_SYSCALL_IMPL(openat2, UnimplementedSyscallSafe);
    }

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(splice, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int fd_in, loff_t *off_in, int fd_out, loff_t *off_out, size_t len, unsigned int flags) -> uint64_t {
      // Flags don't need remapped
      uint64_t Result = ::splice(fd_in, off_in, fd_out, off_out, len, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(tee, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int fd_in, int fd_out, size_t len, unsigned int flags) -> uint64_t {
      // Flags don't need remapped
      uint64_t Result = ::tee(fd_in, fd_out, len, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(timerfd_create, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int32_t clockid, int32_t flags) -> uint64_t {
      // Flags don't need remapped
      uint64_t Result = ::timerfd_create(clockid, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(eventfd, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, uint32_t count) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(eventfd2), count, 0);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_FLAGS(pipe2, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int pipefd[2], int flags) -> uint64_t {
      flags = FEX::HLE::RemapFromX86Flags(flags);
      uint64_t Result = ::pipe2(pipefd, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(inotify_init1, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int flags) -> uint64_t {
      // Flags don't need remapped
      uint64_t Result = ::inotify_init1(flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(renameat2, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int olddirfd, const char *oldpath, int newdirfd, const char *newpath, unsigned int flags) -> uint64_t {
      // Flags don't need remapped
      uint64_t Result = FHU::Syscalls::renameat2(olddirfd, oldpath, newdirfd, newpath, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(memfd_create, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, const char *name, uint32_t flags) -> uint64_t {
      // Flags don't need remapped
      uint64_t Result = ::syscall(SYSCALL_DEF(memfd_create), name, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_FLAGS(statx, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int dirfd, const char *pathname, int flags, uint32_t mask, struct statx *statxbuf) -> uint64_t {
      // Flags don't need remapped
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.Statx(dirfd, pathname, flags, mask, statxbuf);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(name_to_handle_at, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int dirfd, const char *pathname, struct file_handle *handle, int *mount_id, int flags) -> uint64_t {
      // Flags don't need remapped
      uint64_t Result = ::syscall(SYSCALL_DEF(name_to_handle_at), dirfd, pathname, handle, mount_id, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(open_by_handle_at, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int mount_fd, struct file_handle *handle, int flags) -> uint64_t {
      // Flags don't need remapped
      uint64_t Result = ::syscall(SYSCALL_DEF(open_by_handle_at), mount_fd, handle, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(eventfd2, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, unsigned int count, int flags) -> uint64_t {
      // Flags don't need remapped
      uint64_t Result = ::syscall(SYSCALL_DEF(eventfd2), count, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(copy_file_range, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int fd_in, loff_t *off_in, int fd_out, loff_t *off_out, size_t len, unsigned int flags) -> uint64_t {
      // Flags don't need remapped
      uint64_t Result = ::syscall(SYSCALL_DEF(copy_file_range), fd_in, off_in, fd_out, off_out, len, flags);
      SYSCALL_ERRNO();
    });

    if (Handler->IsHostKernelVersionAtLeast(5, 3, 0)) {
      REGISTER_SYSCALL_IMPL_PASS_FLAGS(pidfd_open, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
        [](FEXCore::Core::CpuStateFrame *Frame, pid_t pid, unsigned int flags) -> uint64_t {
        uint64_t Result = ::syscall(SYSCALL_DEF(pidfd_open), pid, flags);
        SYSCALL_ERRNO();
      });
    }
    else {
      REGISTER_SYSCALL_IMPL(pidfd_open, UnimplementedSyscallSafe);
    }

    if (Handler->IsHostKernelVersionAtLeast(5, 9, 0)) {
      REGISTER_SYSCALL_IMPL_FLAGS(close_range, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
        [](FEXCore::Core::CpuStateFrame *Frame, unsigned int first, unsigned int last, unsigned int flags) -> uint64_t {
        uint64_t Result = FEX::HLE::_SyscallHandler->FM.CloseRange(first, last, flags);
        SYSCALL_ERRNO();
      });
    }
    else {
      REGISTER_SYSCALL_IMPL(close_range, UnimplementedSyscallSafe);
    }

    if (Handler->IsHostKernelVersionAtLeast(5, 13, 0)) {
      REGISTER_SYSCALL_IMPL_PASS_FLAGS(landlock_create_ruleset, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
        [](FEXCore::Core::CpuStateFrame *Frame, void *const rule_attr, size_t size, uint32_t flags) -> uint64_t {
        uint64_t Result = ::syscall(SYSCALL_DEF(landlock_create_ruleset), rule_attr, size, flags);
        SYSCALL_ERRNO();
      });
      REGISTER_SYSCALL_IMPL_PASS_FLAGS(landlock_add_rule, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
        [](FEXCore::Core::CpuStateFrame *Frame, uint32_t ruleset_fd, uint64_t rule_type, void *const rule_attr, uint32_t flags) -> uint64_t {
        uint64_t Result = ::syscall(SYSCALL_DEF(landlock_add_rule), ruleset_fd, rule_type, rule_attr, flags);
        SYSCALL_ERRNO();
      });
      REGISTER_SYSCALL_IMPL_PASS_FLAGS(landlock_restrict_self, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
        [](FEXCore::Core::CpuStateFrame *Frame, uint32_t ruleset_fd, uint32_t flags) -> uint64_t {
        uint64_t Result = ::syscall(SYSCALL_DEF(landlock_restrict_self), ruleset_fd, flags);
        SYSCALL_ERRNO();
      });
    }
    else {
      REGISTER_SYSCALL_IMPL(landlock_create_ruleset, UnimplementedSyscallSafe);
      REGISTER_SYSCALL_IMPL(landlock_add_rule, UnimplementedSyscallSafe);
      REGISTER_SYSCALL_IMPL(landlock_restrict_self, UnimplementedSyscallSafe);
    }

    if (Handler->IsHostKernelVersionAtLeast(5, 14, 0)) {
      REGISTER_SYSCALL_IMPL_PASS_FLAGS(memfd_secret, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
        [](FEXCore::Core::CpuStateFrame *Frame, uint32_t flags) -> uint64_t {
        uint64_t Result = ::syscall(SYSCALL_DEF(memfd_secret), flags);
        SYSCALL_ERRNO();
      });
    }
    else {
      REGISTER_SYSCALL_IMPL(memfd_secret, UnimplementedSyscallSafe);
    }

    if (Handler->IsHostKernelVersionAtLeast(5, 15, 0)) {
      REGISTER_SYSCALL_IMPL_PASS_FLAGS(process_mrelease, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
        [](FEXCore::Core::CpuStateFrame *Frame, int pidfd, uint32_t flags) -> uint64_t {
        uint64_t Result = ::syscall(SYSCALL_DEF(process_mrelease), pidfd, flags);
        SYSCALL_ERRNO();
      });
    }
    else {
      REGISTER_SYSCALL_IMPL(process_mrelease, UnimplementedSyscallSafe);
    }
  }
}
