/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"

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
  void RegisterFD(FEX::HLE::SyscallHandler *const Handler) {
    REGISTER_SYSCALL_IMPL_PASS(read, [](FEXCore::Core::CpuStateFrame *Frame, int fd, void *buf, size_t count) -> uint64_t {
      uint64_t Result = ::read(fd, buf, count);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(write, [](FEXCore::Core::CpuStateFrame *Frame, int fd, void *buf, size_t count) -> uint64_t {
      uint64_t Result = ::write(fd, buf, count);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(open, [](FEXCore::Core::CpuStateFrame *Frame, const char *pathname, int flags, uint32_t mode) -> uint64_t {
      flags = FEX::HLE::RemapFromX86Flags(flags);
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.Open(pathname, flags, mode);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(close, [](FEXCore::Core::CpuStateFrame *Frame, int fd) -> uint64_t {
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.Close(fd);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(chown, [](FEXCore::Core::CpuStateFrame *Frame, const char *pathname, uid_t owner, gid_t group) -> uint64_t {
      uint64_t Result = ::chown(pathname, owner, group);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(fchown, [](FEXCore::Core::CpuStateFrame *Frame, int fd, uid_t owner, gid_t group) -> uint64_t {
      uint64_t Result = ::fchown(fd, owner, group);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(lchown, [](FEXCore::Core::CpuStateFrame *Frame, const char *pathname, uid_t owner, gid_t group) -> uint64_t {
      uint64_t Result = ::lchown(pathname, owner, group);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(lseek, [](FEXCore::Core::CpuStateFrame *Frame, int fd, uint64_t offset, int whence) -> uint64_t {
      uint64_t Result = ::lseek(fd, offset, whence);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(access, [](FEXCore::Core::CpuStateFrame *Frame, const char *pathname, int mode) -> uint64_t {
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.Access(pathname, mode);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(pipe, [](FEXCore::Core::CpuStateFrame *Frame, int pipefd[2]) -> uint64_t {
      uint64_t Result = ::pipe(pipefd);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(dup3, [](FEXCore::Core::CpuStateFrame* Frame, int oldfd, int newfd, int flags) -> uint64_t {
      flags = FEX::HLE::RemapFromX86Flags(flags);
      uint64_t Result = ::dup3(oldfd, newfd, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(flock, [](FEXCore::Core::CpuStateFrame *Frame, int fd, int operation) -> uint64_t {
      uint64_t Result = ::flock(fd, operation);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(fsync, [](FEXCore::Core::CpuStateFrame *Frame, int fd) -> uint64_t {
      uint64_t Result = ::fsync(fd);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(fdatasync, [](FEXCore::Core::CpuStateFrame *Frame, int fd) -> uint64_t {
      uint64_t Result = ::fdatasync(fd);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(ftruncate, [](FEXCore::Core::CpuStateFrame *Frame, int fd, off_t length) -> uint64_t {
      uint64_t Result = ::ftruncate(fd, length);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(fchmod, [](FEXCore::Core::CpuStateFrame *Frame, int fd, int mode) -> uint64_t {
      uint64_t Result = ::fchmod(fd, mode);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(fadvise64, [](FEXCore::Core::CpuStateFrame *Frame, int fd, off_t offset, off_t len, int advice) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(fadvise64), fd, offset, len, advice);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(inotify_init, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      uint64_t Result = ::inotify_init();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(inotify_add_watch, [](FEXCore::Core::CpuStateFrame *Frame, int fd, const char *pathname, uint32_t mask) -> uint64_t {
      uint64_t Result = ::inotify_add_watch(fd, pathname, mask);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(inotify_rm_watch, [](FEXCore::Core::CpuStateFrame *Frame, int fd, int wd) -> uint64_t {
      uint64_t Result = ::inotify_rm_watch(fd, wd);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(openat, [](FEXCore::Core::CpuStateFrame *Frame, int dirfs, const char *pathname, int flags, uint32_t mode) -> uint64_t {
      flags = FEX::HLE::RemapFromX86Flags(flags);
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.Openat(dirfs, pathname, flags, mode);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(mkdirat, [](FEXCore::Core::CpuStateFrame *Frame, int dirfd, const char *pathname, mode_t mode) -> uint64_t {
      uint64_t Result = ::mkdirat(dirfd, pathname, mode);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(mknodat, [](FEXCore::Core::CpuStateFrame *Frame, int dirfd, const char *pathname, mode_t mode, dev_t dev) -> uint64_t {
      uint64_t Result = ::mknodat(dirfd, pathname, mode, dev);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(fchownat, [](FEXCore::Core::CpuStateFrame *Frame, int dirfd, const char *pathname, uid_t owner, gid_t group, int flags) -> uint64_t {
      // Flags don't need remapped
      uint64_t Result = ::fchownat(dirfd, pathname, owner, group, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(unlinkat, [](FEXCore::Core::CpuStateFrame *Frame, int dirfd, const char *pathname, int flags) -> uint64_t {
      // Flags don't need remapped
      uint64_t Result = ::unlinkat(dirfd, pathname, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(renameat, [](FEXCore::Core::CpuStateFrame *Frame, int olddirfd, const char *oldpath, int newdirfd, const char *newpath) -> uint64_t {
      uint64_t Result = ::renameat(olddirfd, oldpath, newdirfd, newpath);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(linkat, [](FEXCore::Core::CpuStateFrame *Frame, int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags) -> uint64_t {
      // Flags don't need remapped
      uint64_t Result = ::linkat(olddirfd, oldpath, newdirfd, newpath, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(symlinkat, [](FEXCore::Core::CpuStateFrame *Frame, const char *target, int newdirfd, const char *linkpath) -> uint64_t {
      uint64_t Result = ::symlinkat(target, newdirfd, linkpath);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(readlinkat, [](FEXCore::Core::CpuStateFrame *Frame, int dirfd, const char *pathname, char *buf, size_t bufsiz) -> uint64_t {
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.Readlinkat(dirfd, pathname, buf, bufsiz);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(fchmodat, [](FEXCore::Core::CpuStateFrame *Frame, int dirfd, const char *pathname, mode_t mode) -> uint64_t {
      uint64_t Result = syscall(SYSCALL_DEF(fchmodat), dirfd, pathname, mode);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(faccessat, [](FEXCore::Core::CpuStateFrame *Frame, int dirfd, const char *pathname, int mode) -> uint64_t {
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.FAccessat(dirfd, pathname, mode);
      SYSCALL_ERRNO();
    });

    if (Handler->IsHostKernelVersionAtLeast(5, 8, 0)) {
      // Only exists on kernel 5.8+
      REGISTER_SYSCALL_IMPL(faccessat2, [](FEXCore::Core::CpuStateFrame *Frame, int dirfd, const char *pathname, int mode, int flags) -> uint64_t {
        uint64_t Result = FEX::HLE::_SyscallHandler->FM.FAccessat2(dirfd, pathname, mode, flags);
        SYSCALL_ERRNO();
      });

      REGISTER_SYSCALL_IMPL_PASS(pidfd_getfd, [](FEXCore::Core::CpuStateFrame *Frame, int pidfd, int fd, unsigned int flags) -> uint64_t {
        uint64_t Result = ::syscall(SYSCALL_DEF(pidfd_getfd), pidfd, fd, flags);
        SYSCALL_ERRNO();
      });

      REGISTER_SYSCALL_IMPL(openat2, [](FEXCore::Core::CpuStateFrame *Frame, int dirfs, const char *pathname, struct open_how *how, size_t usize) -> uint64_t {
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

    REGISTER_SYSCALL_IMPL_PASS(splice, [](FEXCore::Core::CpuStateFrame *Frame, int fd_in, loff_t *off_in, int fd_out, loff_t *off_out, size_t len, unsigned int flags) -> uint64_t {
      // Flags don't need remapped
      uint64_t Result = ::splice(fd_in, off_in, fd_out, off_out, len, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(tee, [](FEXCore::Core::CpuStateFrame *Frame, int fd_in, int fd_out, size_t len, unsigned int flags) -> uint64_t {
      // Flags don't need remapped
      uint64_t Result = ::tee(fd_in, fd_out, len, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(timerfd_create, [](FEXCore::Core::CpuStateFrame *Frame, int32_t clockid, int32_t flags) -> uint64_t {
      // Flags don't need remapped
      uint64_t Result = ::timerfd_create(clockid, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(eventfd, [](FEXCore::Core::CpuStateFrame *Frame, uint32_t count) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(eventfd2), count, 0);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(pipe2, [](FEXCore::Core::CpuStateFrame *Frame, int pipefd[2], int flags) -> uint64_t {
      flags = FEX::HLE::RemapFromX86Flags(flags);
      uint64_t Result = ::pipe2(pipefd, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(inotify_init1, [](FEXCore::Core::CpuStateFrame *Frame, int flags) -> uint64_t {
      // Flags don't need remapped
      uint64_t Result = ::inotify_init1(flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(renameat2, [](FEXCore::Core::CpuStateFrame *Frame, int olddirfd, const char *oldpath, int newdirfd, const char *newpath, unsigned int flags) -> uint64_t {
      // Flags don't need remapped
      uint64_t Result = ::renameat2(olddirfd, oldpath, newdirfd, newpath, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(memfd_create, [](FEXCore::Core::CpuStateFrame *Frame, const char *name, uint32_t flags) -> uint64_t {
      // Flags don't need remapped
      uint64_t Result = ::memfd_create(name, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(statx, [](FEXCore::Core::CpuStateFrame *Frame, int dirfd, const char *pathname, int flags, uint32_t mask, struct statx *statxbuf) -> uint64_t {
      // Flags don't need remapped
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.Statx(dirfd, pathname, flags, mask, statxbuf);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(name_to_handle_at, [](FEXCore::Core::CpuStateFrame *Frame, int dirfd, const char *pathname, struct file_handle *handle, int *mount_id, int flags) -> uint64_t {
      // Flags don't need remapped
      uint64_t Result = ::name_to_handle_at(dirfd, pathname, handle, mount_id, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(open_by_handle_at, [](FEXCore::Core::CpuStateFrame *Frame, int mount_fd, struct file_handle *handle, int flags) -> uint64_t {
      // Flags don't need remapped
      uint64_t Result = ::open_by_handle_at(mount_fd, handle, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(eventfd2, [](FEXCore::Core::CpuStateFrame *Frame, unsigned int count, int flags) -> uint64_t {
      // Flags don't need remapped
      uint64_t Result = ::syscall(SYSCALL_DEF(eventfd2), count, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(copy_file_range, [](FEXCore::Core::CpuStateFrame *Frame, int fd_in, loff_t *off_in, int fd_out, loff_t *off_out, size_t len, unsigned int flags) -> uint64_t {
      // Flags don't need remapped
      uint64_t Result = ::copy_file_range(fd_in, off_in, fd_out, off_out, len, flags);
      SYSCALL_ERRNO();
    });

    if (Handler->IsHostKernelVersionAtLeast(5, 3, 0)) {
      REGISTER_SYSCALL_IMPL_PASS(pidfd_open, [](FEXCore::Core::CpuStateFrame *Frame, pid_t pid, unsigned int flags) -> uint64_t {
        uint64_t Result = ::syscall(SYSCALL_DEF(pidfd_open), pid, flags);
        SYSCALL_ERRNO();
      });
    }
    else {
      REGISTER_SYSCALL_IMPL(pidfd_open, UnimplementedSyscallSafe);
    }

    if (Handler->IsHostKernelVersionAtLeast(5, 9, 0)) {
      REGISTER_SYSCALL_IMPL(close_range, [](FEXCore::Core::CpuStateFrame *Frame, unsigned int first, unsigned int last, unsigned int flags) -> uint64_t {
        uint64_t Result = FEX::HLE::_SyscallHandler->FM.CloseRange(first, last, flags);
        SYSCALL_ERRNO();
      });
    }
    else {
      REGISTER_SYSCALL_IMPL(close_range, UnimplementedSyscallSafe);
    }
  }
}
