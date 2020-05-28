#include "Interface/HLE/Syscalls.h"
#include "Interface/Context/Context.h"

#include <stddef.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <unistd.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {
  uint64_t Getcwd(FEXCore::Core::InternalThreadState *Thread, char *buf, size_t size) {
    uint64_t Result = syscall(SYS_getcwd, buf, size);
    SYSCALL_ERRNO();
  }

  uint64_t Chdir(FEXCore::Core::InternalThreadState *Thread, const char *path) {
    uint64_t Result = ::chdir(path);
    SYSCALL_ERRNO();
  }

  uint64_t Rename(FEXCore::Core::InternalThreadState *Thread, const char *oldpath, const char *newpath) {
    uint64_t Result = ::rename(oldpath, newpath);
    SYSCALL_ERRNO();
  }

  uint64_t Mkdir(FEXCore::Core::InternalThreadState *Thread, const char *pathname, mode_t mode) {
    uint64_t Result = ::mkdir(pathname, mode);
    SYSCALL_ERRNO();
  }

  uint64_t Rmdir(FEXCore::Core::InternalThreadState *Thread, const char *pathname) {
    uint64_t Result = ::rmdir(pathname);
    SYSCALL_ERRNO();
  }

  uint64_t Link(FEXCore::Core::InternalThreadState *Thread, const char *oldpath, const char *newpath) {
    uint64_t Result = ::link(oldpath, newpath);
    SYSCALL_ERRNO();
  }

  uint64_t Unlink(FEXCore::Core::InternalThreadState *Thread, const char *pathname) {
    uint64_t Result = ::unlink(pathname);
    SYSCALL_ERRNO();
  }

  uint64_t Symlink(FEXCore::Core::InternalThreadState *Thread, const char *target, const char *linkpath) {
    uint64_t Result = ::symlink(target, linkpath);
    SYSCALL_ERRNO();
  }

  uint64_t Readlink(FEXCore::Core::InternalThreadState *Thread, const char *pathname, char *buf, size_t bufsiz) {
    uint64_t Result = Thread->CTX->SyscallHandler->FM.Readlink(pathname, buf, bufsiz);
    SYSCALL_ERRNO();
  }

  uint64_t Chmod(FEXCore::Core::InternalThreadState *Thread, const char *pathname, mode_t mode) {
    uint64_t Result = ::chmod(pathname, mode);
    SYSCALL_ERRNO();
  }

  uint64_t Umask(FEXCore::Core::InternalThreadState *Thread, mode_t mask) {
    uint64_t Result = ::umask(mask);
    SYSCALL_ERRNO();
  }

  uint64_t Mknod(FEXCore::Core::InternalThreadState *Thread, const char *pathname, mode_t mode, dev_t dev) {
    uint64_t Result = Thread->CTX->SyscallHandler->FM.Mknod(pathname, mode, dev);
    SYSCALL_ERRNO();
  }

  uint64_t Statfs(FEXCore::Core::InternalThreadState *Thread, const char *path, struct statfs *buf) {
    uint64_t Result = Thread->CTX->SyscallHandler->FM.Statfs(path, buf);
    SYSCALL_ERRNO();
  }

  uint64_t FStatfs(FEXCore::Core::InternalThreadState *Thread, int fd, struct statfs *buf) {
    uint64_t Result = ::fstatfs(fd, buf);
    SYSCALL_ERRNO();
  }
}
