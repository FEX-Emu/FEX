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
#include <sys/vfs.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/xattr.h>

namespace FEX::HLE {
  void RegisterFS() {
    REGISTER_SYSCALL_IMPL(getcwd, [](FEXCore::Core::CpuStateFrame *Frame, char *buf, size_t size) -> uint64_t {
      uint64_t Result = syscall(SYS_getcwd, buf, size);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(chdir, [](FEXCore::Core::CpuStateFrame *Frame, const char *path) -> uint64_t {
      uint64_t Result = ::chdir(path);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(fchdir, [](FEXCore::Core::CpuStateFrame *Frame, int fd) -> uint64_t {
      uint64_t Result = ::fchdir(fd);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(rename, [](FEXCore::Core::CpuStateFrame *Frame, const char *oldpath, const char *newpath) -> uint64_t {
      uint64_t Result = ::rename(oldpath, newpath);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(mkdir, [](FEXCore::Core::CpuStateFrame *Frame, const char *pathname, mode_t mode) -> uint64_t {
      uint64_t Result = ::mkdir(pathname, mode);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(rmdir, [](FEXCore::Core::CpuStateFrame *Frame, const char *pathname) -> uint64_t {
      uint64_t Result = ::rmdir(pathname);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(link, [](FEXCore::Core::CpuStateFrame *Frame, const char *oldpath, const char *newpath) -> uint64_t {
      uint64_t Result = ::link(oldpath, newpath);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(unlink, [](FEXCore::Core::CpuStateFrame *Frame, const char *pathname) -> uint64_t {
      uint64_t Result = ::unlink(pathname);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(symlink, [](FEXCore::Core::CpuStateFrame *Frame, const char *target, const char *linkpath) -> uint64_t {
      uint64_t Result = ::symlink(target, linkpath);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(readlink, [](FEXCore::Core::CpuStateFrame *Frame, const char *pathname, char *buf, size_t bufsiz) -> uint64_t {
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.Readlink(pathname, buf, bufsiz);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(chmod, [](FEXCore::Core::CpuStateFrame *Frame, const char *pathname, mode_t mode) -> uint64_t {
      uint64_t Result = ::chmod(pathname, mode);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(umask, [](FEXCore::Core::CpuStateFrame *Frame, mode_t mask) -> uint64_t {
      uint64_t Result = ::umask(mask);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(mknod, [](FEXCore::Core::CpuStateFrame *Frame, const char *pathname, mode_t mode, dev_t dev) -> uint64_t {
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.Mknod(pathname, mode, dev);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(ustat, [](FEXCore::Core::CpuStateFrame *Frame, dev_t dev, struct ustat *ubuf) -> uint64_t {
      // Since version 2.28 of GLIBC it has stopped providing a wrapper for this syscall
#ifdef SYS_ustat
      uint64_t Result = syscall(SYS_ustat, dev, ubuf);
      SYSCALL_ERRNO();
#else
      return -ENOSYS;
#endif
    });

    /*
      arg1 is one of: void, unsigned int fs_index, const char *fsname
      arg2 is one of: void, char *buf
    */
    REGISTER_SYSCALL_IMPL(sysfs, [](FEXCore::Core::CpuStateFrame *Frame, int option,  uint64_t arg1,  uint64_t arg2) -> uint64_t {
#ifdef SYS_sysfs
      uint64_t Result = syscall(SYS_sysfs, option, arg1, arg2);
      SYSCALL_ERRNO();
#else
      return -ENOSYS;
#endif
    });

    REGISTER_SYSCALL_IMPL(statfs, [](FEXCore::Core::CpuStateFrame *Frame, const char *path, struct statfs *buf) -> uint64_t {
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.Statfs(path, buf);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(fstatfs, [](FEXCore::Core::CpuStateFrame *Frame, int fd, struct statfs *buf) -> uint64_t {
      uint64_t Result = ::fstatfs(fd, buf);
      SYSCALL_ERRNO();
    });

    /*REGISTER_SYSCALL_IMPL(truncate, [](FEXCore::Core::CpuStateFrame *Frame, const char *path, off_t length) -> uint64_t {
      SYSCALL_STUB(truncate);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(truncate);

    /*REGISTER_SYSCALL_IMPL(creat, [](FEXCore::Core::CpuStateFrame *Frame, const char *pathname, mode_t mode) -> uint64_t {
      SYSCALL_STUB(creat);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(creat);

    REGISTER_SYSCALL_IMPL(chroot, [](FEXCore::Core::CpuStateFrame *Frame, const char *path) -> uint64_t {
      uint64_t Result = ::chroot(path);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(sync, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      sync();
      return 0; // always successful
    });

    REGISTER_SYSCALL_IMPL(acct, [](FEXCore::Core::CpuStateFrame *Frame, const char *filename) -> uint64_t {
      uint64_t Result = ::acct(filename);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(mount, [](FEXCore::Core::CpuStateFrame *Frame, const char *source, const char *target, const char *filesystemtype, unsigned long mountflags, const void *data) -> uint64_t {
      uint64_t Result = ::mount(source, target, filesystemtype, mountflags, data);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(umount2, [](FEXCore::Core::CpuStateFrame *Frame, const char *target, int flags) -> uint64_t {
      uint64_t Result = ::umount2(target, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(swapon, [](FEXCore::Core::CpuStateFrame *Frame, const char *path, int swapflags) -> uint64_t {
      uint64_t Result = ::swapon(path, swapflags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(swapoff, [](FEXCore::Core::CpuStateFrame *Frame, const char *path) -> uint64_t {
      uint64_t Result = ::swapoff(path);
      SYSCALL_ERRNO();
    });


    /*
    REGISTER_SYSCALL_IMPL(syncfs, [](FEXCore::Core::CpuStateFrame *Frame, int fd) -> uint64_t {
      SYSCALL_STUB(syncfs);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(syncfs);

    /*
    REGISTER_SYSCALL_IMPL(setxattr, [](FEXCore::Core::CpuStateFrame *Frame, const char *path, const char *name, const void *value, size_t size, int flags) -> uint64_t {
      SYSCALL_STUB(setxattr);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(setxattr);

    /*
    REGISTER_SYSCALL_IMPL(lsetxattr, [](FEXCore::Core::CpuStateFrame *Frame, const char *path, const char *name, const void *value, size_t size, int flags) -> uint64_t {
      SYSCALL_STUB(lsetxattr);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(lsetxattr);

    /*
    REGISTER_SYSCALL_IMPL(fsetxattr, [](FEXCore::Core::CpuStateFrame *Frame, int fd, const char *name, const void *value, size_t size, int flags) -> uint64_t {
      SYSCALL_STUB(fsetxattr);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(fsetxattr);

    /*
    REGISTER_SYSCALL_IMPL(getxattr, [](FEXCore::Core::CpuStateFrame *Frame, const char *path, const char *name, void *value, size_t size) -> uint64_t {
      SYSCALL_STUB(getxattr);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(getxattr);

    /*
    REGISTER_SYSCALL_IMPL(lgetxattr, [](FEXCore::Core::CpuStateFrame *Frame, const char *path, const char *name, void *value, size_t size) -> uint64_t {
      SYSCALL_STUB(lgetxattr);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(lgetxattr);

    /*
    REGISTER_SYSCALL_IMPL(fgetxattr, [](FEXCore::Core::CpuStateFrame *Frame, int fd, const char *name, void *value, size_t size) -> uint64_t {
      SYSCALL_STUB(fgetxattr);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(fgetxattr);

    /*
    REGISTER_SYSCALL_IMPL(listxattr, [](FEXCore::Core::CpuStateFrame *Frame, const char *path, char *list, size_t size) -> uint64_t {
      SYSCALL_STUB(listxattr);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(listxattr);

    /*
    REGISTER_SYSCALL_IMPL(llistxattr, [](FEXCore::Core::CpuStateFrame *Frame, const char *path, char *list, size_t size) -> uint64_t {
      SYSCALL_STUB(llistxattr);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(llistxattr);

    /*
    REGISTER_SYSCALL_IMPL(flistxattr, [](FEXCore::Core::CpuStateFrame *Frame, int fd, char *list, size_t size) -> uint64_t {
      SYSCALL_STUB(flistxattr);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(flistxattr);

    /*
    REGISTER_SYSCALL_IMPL(removexattr, [](FEXCore::Core::CpuStateFrame *Frame, const char *path, const char *name) -> uint64_t {
      SYSCALL_STUB(removexattr);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(removexattr);

    /*
    REGISTER_SYSCALL_IMPL(lremovexattr, [](FEXCore::Core::CpuStateFrame *Frame, const char *path, const char *name) -> uint64_t {
      SYSCALL_STUB(lremovexattr);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(lremovexattr);

    /*
    REGISTER_SYSCALL_IMPL(fremovexattr, [](FEXCore::Core::CpuStateFrame *Frame, int fd, const char *name) -> uint64_t {
      SYSCALL_STUB(fremovexattr);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(fremovexattr);

    REGISTER_SYSCALL_IMPL(fanotify_init, [](FEXCore::Core::CpuStateFrame *Frame, unsigned int flags, unsigned int event_f_flags) -> uint64_t {
      uint64_t Result = ::fanotify_init(flags, event_f_flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(fanotify_mark, [](FEXCore::Core::CpuStateFrame *Frame, int fanotify_fd, unsigned int flags, uint64_t mask, int dirfd, const char *pathname) -> uint64_t {
      uint64_t Result = ::fanotify_mark(fanotify_fd, flags, mask, dirfd, pathname);
      SYSCALL_ERRNO();
    });
  }
}
