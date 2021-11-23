/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"

#include <stddef.h>
#include <stdint.h>
#include <sys/fanotify.h>
#include <sys/mount.h>
#include <sys/swap.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/xattr.h>

namespace FEX::HLE {
  void RegisterFS(FEX::HLE::SyscallHandler *const Handler) {
    REGISTER_SYSCALL_IMPL_PASS(getcwd, [](FEXCore::Core::CpuStateFrame *Frame, char *buf, size_t size) -> uint64_t {
      uint64_t Result = syscall(SYSCALL_DEF(getcwd), buf, size);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(chdir, [](FEXCore::Core::CpuStateFrame *Frame, const char *path) -> uint64_t {
      uint64_t Result = ::chdir(path);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(fchdir, [](FEXCore::Core::CpuStateFrame *Frame, int fd) -> uint64_t {
      uint64_t Result = ::fchdir(fd);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(rename, [](FEXCore::Core::CpuStateFrame *Frame, const char *oldpath, const char *newpath) -> uint64_t {
      uint64_t Result = ::rename(oldpath, newpath);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(mkdir, [](FEXCore::Core::CpuStateFrame *Frame, const char *pathname, mode_t mode) -> uint64_t {
      uint64_t Result = ::mkdir(pathname, mode);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(rmdir, [](FEXCore::Core::CpuStateFrame *Frame, const char *pathname) -> uint64_t {
      uint64_t Result = ::rmdir(pathname);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(link, [](FEXCore::Core::CpuStateFrame *Frame, const char *oldpath, const char *newpath) -> uint64_t {
      uint64_t Result = ::link(oldpath, newpath);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(unlink, [](FEXCore::Core::CpuStateFrame *Frame, const char *pathname) -> uint64_t {
      uint64_t Result = ::unlink(pathname);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(symlink, [](FEXCore::Core::CpuStateFrame *Frame, const char *target, const char *linkpath) -> uint64_t {
      uint64_t Result = ::symlink(target, linkpath);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(readlink, [](FEXCore::Core::CpuStateFrame *Frame, const char *pathname, char *buf, size_t bufsiz) -> uint64_t {
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.Readlink(pathname, buf, bufsiz);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(chmod, [](FEXCore::Core::CpuStateFrame *Frame, const char *pathname, mode_t mode) -> uint64_t {
      uint64_t Result = ::chmod(pathname, mode);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(umask, [](FEXCore::Core::CpuStateFrame *Frame, mode_t mask) -> uint64_t {
      uint64_t Result = ::umask(mask);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(mknod, [](FEXCore::Core::CpuStateFrame *Frame, const char *pathname, mode_t mode, dev_t dev) -> uint64_t {
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.Mknod(pathname, mode, dev);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(ustat, [](FEXCore::Core::CpuStateFrame *Frame, dev_t dev, struct ustat *ubuf) -> uint64_t {
      // Doesn't exist on AArch64, will return -ENOSYS
      // Since version 2.28 of GLIBC it has stopped providing a wrapper for this syscall
      uint64_t Result = syscall(SYSCALL_DEF(ustat), dev, ubuf);
      SYSCALL_ERRNO();
    });

    /*
      arg1 is one of: void, unsigned int fs_index, const char *fsname
      arg2 is one of: void, char *buf
    */
    REGISTER_SYSCALL_IMPL_PASS(sysfs, [](FEXCore::Core::CpuStateFrame *Frame, int option,  uint64_t arg1,  uint64_t arg2) -> uint64_t {
      // Doesn't exist on AArch64, will return -ENOSYS
      uint64_t Result = syscall(SYSCALL_DEF(sysfs), option, arg1, arg2);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(truncate, [](FEXCore::Core::CpuStateFrame *Frame, const char *path, off_t length) -> uint64_t {
      uint64_t Result = ::truncate(path, length);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(creat, [](FEXCore::Core::CpuStateFrame *Frame, const char *pathname, mode_t mode) -> uint64_t {
      uint64_t Result = ::creat(pathname, mode);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(chroot, [](FEXCore::Core::CpuStateFrame *Frame, const char *path) -> uint64_t {
      uint64_t Result = ::chroot(path);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(sync, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      sync();
      return 0; // always successful
    });

    REGISTER_SYSCALL_IMPL_PASS(acct, [](FEXCore::Core::CpuStateFrame *Frame, const char *filename) -> uint64_t {
      uint64_t Result = ::acct(filename);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(mount, [](FEXCore::Core::CpuStateFrame *Frame, const char *source, const char *target, const char *filesystemtype, unsigned long mountflags, const void *data) -> uint64_t {
      uint64_t Result = ::mount(source, target, filesystemtype, mountflags, data);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(umount2, [](FEXCore::Core::CpuStateFrame *Frame, const char *target, int flags) -> uint64_t {
      uint64_t Result = ::umount2(target, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(swapon, [](FEXCore::Core::CpuStateFrame *Frame, const char *path, int swapflags) -> uint64_t {
      uint64_t Result = ::swapon(path, swapflags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(swapoff, [](FEXCore::Core::CpuStateFrame *Frame, const char *path) -> uint64_t {
      uint64_t Result = ::swapoff(path);
      SYSCALL_ERRNO();
    });


    REGISTER_SYSCALL_IMPL_PASS(syncfs, [](FEXCore::Core::CpuStateFrame *Frame, int fd) -> uint64_t {
      uint64_t Result = ::syncfs(fd);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(setxattr, [](FEXCore::Core::CpuStateFrame *Frame, const char *path, const char *name, const void *value, size_t size, int flags) -> uint64_t {
      uint64_t Result = ::setxattr(path, name, value, size, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(lsetxattr, [](FEXCore::Core::CpuStateFrame *Frame, const char *path, const char *name, const void *value, size_t size, int flags) -> uint64_t {
      uint64_t Result = ::lsetxattr(path, name, value, size, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(fsetxattr, [](FEXCore::Core::CpuStateFrame *Frame, int fd, const char *name, const void *value, size_t size, int flags) -> uint64_t {
      uint64_t Result = ::fsetxattr(fd, name, value, size, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(getxattr, [](FEXCore::Core::CpuStateFrame *Frame, const char *path, const char *name, void *value, size_t size) -> uint64_t {
      uint64_t Result = ::getxattr(path, name, value, size);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(lgetxattr, [](FEXCore::Core::CpuStateFrame *Frame, const char *path, const char *name, void *value, size_t size) -> uint64_t {
      uint64_t Result = ::lgetxattr(path, name, value, size);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(fgetxattr, [](FEXCore::Core::CpuStateFrame *Frame, int fd, const char *name, void *value, size_t size) -> uint64_t {
      uint64_t Result = ::fgetxattr(fd, name, value, size);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(listxattr, [](FEXCore::Core::CpuStateFrame *Frame, const char *path, char *list, size_t size) -> uint64_t {
      uint64_t Result = ::listxattr(path, list, size);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(llistxattr, [](FEXCore::Core::CpuStateFrame *Frame, const char *path, char *list, size_t size) -> uint64_t {
      uint64_t Result = ::llistxattr(path, list, size);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(flistxattr, [](FEXCore::Core::CpuStateFrame *Frame, int fd, char *list, size_t size) -> uint64_t {
      uint64_t Result = ::flistxattr(fd, list, size);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(removexattr, [](FEXCore::Core::CpuStateFrame *Frame, const char *path, const char *name) -> uint64_t {
      uint64_t Result = ::removexattr(path, name);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(lremovexattr, [](FEXCore::Core::CpuStateFrame *Frame, const char *path, const char *name) -> uint64_t {
      uint64_t Result = ::lremovexattr(path, name);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(fremovexattr, [](FEXCore::Core::CpuStateFrame *Frame, int fd, const char *name) -> uint64_t {
      uint64_t Result = ::fremovexattr(fd, name);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(fanotify_init, [](FEXCore::Core::CpuStateFrame *Frame, unsigned int flags, unsigned int event_f_flags) -> uint64_t {
      uint64_t Result = ::fanotify_init(flags, event_f_flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(fanotify_mark, [](FEXCore::Core::CpuStateFrame *Frame, int fanotify_fd, unsigned int flags, uint64_t mask, int dirfd, const char *pathname) -> uint64_t {
      uint64_t Result = ::fanotify_mark(fanotify_fd, flags, mask, dirfd, pathname);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS(pivot_root, [](FEXCore::Core::CpuStateFrame *Frame, const char *new_root, const char *put_old) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(pivot_root), new_root, put_old);
      SYSCALL_ERRNO();
    });

    if (Handler->IsHostKernelVersionAtLeast(5, 14, 0)) {
      REGISTER_SYSCALL_IMPL_PASS(quotactl_fd, [](FEXCore::Core::CpuStateFrame *Frame, uint32_t fd, uint32_t cmd, uint32_t id, void* addr) -> uint64_t {
        uint64_t Result = ::syscall(SYSCALL_DEF(quotactl_fd), fd, cmd, id, addr);
        SYSCALL_ERRNO();
      });
    }
    else {
      REGISTER_SYSCALL_IMPL(quotactl_fd, UnimplementedSyscallSafe);
    }
  }
}
