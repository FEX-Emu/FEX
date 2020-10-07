#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"
#include "Interface/HLE/x32/Syscalls.h"
#include "Interface/Context/Context.h"

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

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {

  void RegisterFS() {
    REGISTER_SYSCALL_IMPL(getcwd, [](FEXCore::Core::InternalThreadState *Thread, char *buf, size_t size) -> uint64_t {
      uint64_t Result = syscall(SYS_getcwd, buf, size);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(chdir, [](FEXCore::Core::InternalThreadState *Thread, const char *path) -> uint64_t {
      uint64_t Result = ::chdir(path);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(fchdir, [](FEXCore::Core::InternalThreadState *Thread, int fd) -> uint64_t {
      uint64_t Result = ::fchdir(fd);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(rename, [](FEXCore::Core::InternalThreadState *Thread, const char *oldpath, const char *newpath) -> uint64_t {
      uint64_t Result = ::rename(oldpath, newpath);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(mkdir, [](FEXCore::Core::InternalThreadState *Thread, const char *pathname, mode_t mode) -> uint64_t {
      uint64_t Result = ::mkdir(pathname, mode);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(rmdir, [](FEXCore::Core::InternalThreadState *Thread, const char *pathname) -> uint64_t {
      uint64_t Result = ::rmdir(pathname);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(link, [](FEXCore::Core::InternalThreadState *Thread, const char *oldpath, const char *newpath) -> uint64_t {
      uint64_t Result = ::link(oldpath, newpath);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(unlink, [](FEXCore::Core::InternalThreadState *Thread, const char *pathname) -> uint64_t {
      uint64_t Result = ::unlink(pathname);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(symlink, [](FEXCore::Core::InternalThreadState *Thread, const char *target, const char *linkpath) -> uint64_t {
      uint64_t Result = ::symlink(target, linkpath);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(readlink, [](FEXCore::Core::InternalThreadState *Thread, const char *pathname, char *buf, size_t bufsiz) -> uint64_t {
      uint64_t Result = Thread->CTX->SyscallHandler->FM.Readlink(pathname, buf, bufsiz);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(chmod, [](FEXCore::Core::InternalThreadState *Thread, const char *pathname, mode_t mode) -> uint64_t {
      uint64_t Result = ::chmod(pathname, mode);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(umask, [](FEXCore::Core::InternalThreadState *Thread, mode_t mask) -> uint64_t {
      uint64_t Result = ::umask(mask);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(mknod, [](FEXCore::Core::InternalThreadState *Thread, const char *pathname, mode_t mode, dev_t dev) -> uint64_t {
      uint64_t Result = Thread->CTX->SyscallHandler->FM.Mknod(pathname, mode, dev);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(ustat, [](FEXCore::Core::InternalThreadState *Thread, dev_t dev, struct ustat *ubuf) -> uint64_t {
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
    REGISTER_SYSCALL_IMPL(sysfs, [](FEXCore::Core::InternalThreadState *Thread, int option,  uint64_t arg1,  uint64_t arg2) -> uint64_t {
#ifdef SYS_sysfs
      uint64_t Result = syscall(SYS_sysfs, option, arg1, arg2);
      SYSCALL_ERRNO();
#else
      return -ENOSYS;
#endif
    });

    REGISTER_SYSCALL_IMPL(statfs, [](FEXCore::Core::InternalThreadState *Thread, const char *path, struct statfs *buf) -> uint64_t {
      uint64_t Result = Thread->CTX->SyscallHandler->FM.Statfs(path, buf);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(fstatfs, [](FEXCore::Core::InternalThreadState *Thread, int fd, struct statfs *buf) -> uint64_t {
      uint64_t Result = ::fstatfs(fd, buf);
      SYSCALL_ERRNO();
    });

    /*REGISTER_SYSCALL_IMPL(truncate, [](FEXCore::Core::InternalThreadState *Thread, const char *path, off_t length) -> uint64_t {
      SYSCALL_STUB(truncate);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(truncate);

    /*REGISTER_SYSCALL_IMPL(creat, [](FEXCore::Core::InternalThreadState *Thread, const char *pathname, mode_t mode) -> uint64_t {
      SYSCALL_STUB(creat);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(creat);

    REGISTER_SYSCALL_IMPL(chroot, [](FEXCore::Core::InternalThreadState *Thread, const char *path) -> uint64_t {
      uint64_t Result = ::chroot(path);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(sync, [](FEXCore::Core::InternalThreadState *Thread) -> uint64_t {
      sync();
      return 0; // always successful
    });

    REGISTER_SYSCALL_IMPL(acct, [](FEXCore::Core::InternalThreadState *Thread, const char *filename) -> uint64_t {
      uint64_t Result = ::acct(filename);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(mount, [](FEXCore::Core::InternalThreadState *Thread, const char *source, const char *target, const char *filesystemtype, unsigned long mountflags, const void *data) -> uint64_t {
      uint64_t Result = ::mount(source, target, filesystemtype, mountflags, data);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(umount2, [](FEXCore::Core::InternalThreadState *Thread, const char *target, int flags) -> uint64_t {
      uint64_t Result = ::umount2(target, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(swapon, [](FEXCore::Core::InternalThreadState *Thread, const char *path, int swapflags) -> uint64_t {
      uint64_t Result = ::swapon(path, swapflags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(swapoff, [](FEXCore::Core::InternalThreadState *Thread, const char *path) -> uint64_t {
      uint64_t Result = ::swapoff(path);
      SYSCALL_ERRNO();
    });


    /*
    REGISTER_SYSCALL_IMPL(syncfs, [](FEXCore::Core::InternalThreadState *Thread, int fd) -> uint64_t {
      SYSCALL_STUB(syncfs);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(syncfs);

    /*
    REGISTER_SYSCALL_IMPL(setxattr, [](FEXCore::Core::InternalThreadState *Thread, const char *path, const char *name, const void *value, size_t size, int flags) -> uint64_t {
      SYSCALL_STUB(setxattr);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(setxattr);

    /*
    REGISTER_SYSCALL_IMPL(lsetxattr, [](FEXCore::Core::InternalThreadState *Thread, const char *path, const char *name, const void *value, size_t size, int flags) -> uint64_t {
      SYSCALL_STUB(lsetxattr);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(lsetxattr);

    /*
    REGISTER_SYSCALL_IMPL(fsetxattr, [](FEXCore::Core::InternalThreadState *Thread, int fd, const char *name, const void *value, size_t size, int flags) -> uint64_t {
      SYSCALL_STUB(fsetxattr);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(fsetxattr);

    /*
    REGISTER_SYSCALL_IMPL(getxattr, [](FEXCore::Core::InternalThreadState *Thread, const char *path, const char *name, void *value, size_t size) -> uint64_t {
      SYSCALL_STUB(getxattr);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(getxattr);

    /*
    REGISTER_SYSCALL_IMPL(lgetxattr, [](FEXCore::Core::InternalThreadState *Thread, const char *path, const char *name, void *value, size_t size) -> uint64_t {
      SYSCALL_STUB(lgetxattr);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(lgetxattr);

    /*
    REGISTER_SYSCALL_IMPL(fgetxattr, [](FEXCore::Core::InternalThreadState *Thread, int fd, const char *name, void *value, size_t size) -> uint64_t {
      SYSCALL_STUB(fgetxattr);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(fgetxattr);

    /*
    REGISTER_SYSCALL_IMPL(listxattr, [](FEXCore::Core::InternalThreadState *Thread, const char *path, char *list, size_t size) -> uint64_t {
      SYSCALL_STUB(listxattr);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(listxattr);

    /*
    REGISTER_SYSCALL_IMPL(llistxattr, [](FEXCore::Core::InternalThreadState *Thread, const char *path, char *list, size_t size) -> uint64_t {
      SYSCALL_STUB(llistxattr);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(llistxattr);

    /*
    REGISTER_SYSCALL_IMPL(flistxattr, [](FEXCore::Core::InternalThreadState *Thread, int fd, char *list, size_t size) -> uint64_t {
      SYSCALL_STUB(flistxattr);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(flistxattr);

    /*
    REGISTER_SYSCALL_IMPL(removexattr, [](FEXCore::Core::InternalThreadState *Thread, const char *path, const char *name) -> uint64_t {
      SYSCALL_STUB(removexattr);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(removexattr);

    /*
    REGISTER_SYSCALL_IMPL(lremovexattr, [](FEXCore::Core::InternalThreadState *Thread, const char *path, const char *name) -> uint64_t {
      SYSCALL_STUB(lremovexattr);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(lremovexattr);

    /*
    REGISTER_SYSCALL_IMPL(fremovexattr, [](FEXCore::Core::InternalThreadState *Thread, int fd, const char *name) -> uint64_t {
      SYSCALL_STUB(fremovexattr);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(fremovexattr);

    REGISTER_SYSCALL_IMPL(fanotify_init, [](FEXCore::Core::InternalThreadState *Thread, unsigned int flags, unsigned int event_f_flags) -> uint64_t {
      uint64_t Result = ::fanotify_init(flags, event_f_flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(fanotify_mark, [](FEXCore::Core::InternalThreadState *Thread, int fanotify_fd, unsigned int flags, uint64_t mask, int dirfd, const char *pathname) -> uint64_t {
      uint64_t Result = ::fanotify_mark(fanotify_fd, flags, mask, dirfd, pathname);
      SYSCALL_ERRNO();
    });
  }
}
