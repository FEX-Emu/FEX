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

#include <stddef.h>
#include <stdint.h>
#include <sys/mount.h>
#include <sys/swap.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/xattr.h>

namespace FEX::HLE {
using namespace FEXCore::IR;

auto rename(FEXCore::Core::CpuStateFrame* Frame, const char* oldpath, const char* newpath) -> uint64_t {
  uint64_t Result = ::rename(oldpath, newpath);
  SYSCALL_ERRNO();
}

auto mkdir(FEXCore::Core::CpuStateFrame* Frame, const char* pathname, mode_t mode) -> uint64_t {
  uint64_t Result = ::mkdir(pathname, mode);
  SYSCALL_ERRNO();
}

auto rmdir(FEXCore::Core::CpuStateFrame* Frame, const char* pathname) -> uint64_t {
  uint64_t Result = ::rmdir(pathname);
  SYSCALL_ERRNO();
}

auto link(FEXCore::Core::CpuStateFrame* Frame, const char* oldpath, const char* newpath) -> uint64_t {
  uint64_t Result = ::link(oldpath, newpath);
  SYSCALL_ERRNO();
}

auto unlink(FEXCore::Core::CpuStateFrame* Frame, const char* pathname) -> uint64_t {
  uint64_t Result = ::unlink(pathname);
  SYSCALL_ERRNO();
}

auto symlink(FEXCore::Core::CpuStateFrame* Frame, const char* target, const char* linkpath) -> uint64_t {
  uint64_t Result = ::symlink(target, linkpath);
  SYSCALL_ERRNO();
}

auto readlink(FEXCore::Core::CpuStateFrame* Frame, const char* pathname, char* buf, size_t bufsiz) -> uint64_t {
  uint64_t Result = FEX::HLE::_SyscallHandler->FM.Readlink(pathname, buf, bufsiz);
  SYSCALL_ERRNO();
}

auto chmod(FEXCore::Core::CpuStateFrame* Frame, const char* pathname, mode_t mode) -> uint64_t {
  uint64_t Result = ::chmod(pathname, mode);
  SYSCALL_ERRNO();
}

auto mknod(FEXCore::Core::CpuStateFrame* Frame, const char* pathname, mode_t mode, dev_t dev) -> uint64_t {
  uint64_t Result = FEX::HLE::_SyscallHandler->FM.Mknod(pathname, mode, dev);
  SYSCALL_ERRNO();
}

auto creat(FEXCore::Core::CpuStateFrame* Frame, const char* pathname, mode_t mode) -> uint64_t {
  uint64_t Result = ::creat(pathname, mode);
  SYSCALL_ERRNO();
}

auto setxattr(FEXCore::Core::CpuStateFrame* Frame, const char* path, const char* name, const void* value, size_t size, int flags) -> uint64_t {
  uint64_t Result = FEX::HLE::_SyscallHandler->FM.Setxattr(path, name, value, size, flags);
  SYSCALL_ERRNO();
}

auto lsetxattr(FEXCore::Core::CpuStateFrame* Frame, const char* path, const char* name, const void* value, size_t size, int flags) -> uint64_t {
  uint64_t Result = FEX::HLE::_SyscallHandler->FM.LSetxattr(path, name, value, size, flags);
  SYSCALL_ERRNO();
}

auto getxattr(FEXCore::Core::CpuStateFrame* Frame, const char* path, const char* name, void* value, size_t size) -> uint64_t {
  uint64_t Result = FEX::HLE::_SyscallHandler->FM.Getxattr(path, name, value, size);
  SYSCALL_ERRNO();
}

auto lgetxattr(FEXCore::Core::CpuStateFrame* Frame, const char* path, const char* name, void* value, size_t size) -> uint64_t {
  uint64_t Result = FEX::HLE::_SyscallHandler->FM.LGetxattr(path, name, value, size);
  SYSCALL_ERRNO();
}

auto listxattr(FEXCore::Core::CpuStateFrame* Frame, const char* path, char* list, size_t size) -> uint64_t {
  uint64_t Result = FEX::HLE::_SyscallHandler->FM.Listxattr(path, list, size);
  SYSCALL_ERRNO();
}

auto llistxattr(FEXCore::Core::CpuStateFrame* Frame, const char* path, char* list, size_t size) -> uint64_t {
  uint64_t Result = FEX::HLE::_SyscallHandler->FM.LListxattr(path, list, size);
  SYSCALL_ERRNO();
}

auto removexattr(FEXCore::Core::CpuStateFrame* Frame, const char* path, const char* name) -> uint64_t {
  uint64_t Result = FEX::HLE::_SyscallHandler->FM.Removexattr(path, name);
  SYSCALL_ERRNO();
}

auto lremovexattr(FEXCore::Core::CpuStateFrame* Frame, const char* path, const char* name) -> uint64_t {
  uint64_t Result = FEX::HLE::_SyscallHandler->FM.LRemovexattr(path, name);
  SYSCALL_ERRNO();
}
auto setxattrat(FEXCore::Core::CpuStateFrame* Frame, int dfd, const char* pathname, uint32_t at_flags, const char* name,
                const FileManager::xattr_args* uargs, size_t usize) -> uint64_t {
  if (!FEX::HLE::_SyscallHandler->IsHostKernelVersionAtLeast(6, 13, 0)) {
    return -ENOSYS;
  }
  uint64_t Result = FEX::HLE::_SyscallHandler->FM.SetxattrAt(dfd, pathname, at_flags, name, uargs, usize);
  SYSCALL_ERRNO();
}
auto getxattrat(FEXCore::Core::CpuStateFrame* Frame, int dfd, const char* pathname, uint32_t at_flags, const char* name,
                const FileManager::xattr_args* uargs, size_t usize) -> uint64_t {

  if (!FEX::HLE::_SyscallHandler->IsHostKernelVersionAtLeast(6, 13, 0)) {
    return -ENOSYS;
  }
  uint64_t Result = FEX::HLE::_SyscallHandler->FM.GetxattrAt(dfd, pathname, at_flags, name, uargs, usize);
  SYSCALL_ERRNO();
}

auto listxattrat(FEXCore::Core::CpuStateFrame* Frame, int dfd, const char* pathname, uint32_t at_flags, char* list, size_t size) -> uint64_t {

  if (!FEX::HLE::_SyscallHandler->IsHostKernelVersionAtLeast(6, 13, 0)) {
    return -ENOSYS;
  }
  uint64_t Result = FEX::HLE::_SyscallHandler->FM.ListxattrAt(dfd, pathname, at_flags, list, size);
  SYSCALL_ERRNO();
}
auto removexattrat(FEXCore::Core::CpuStateFrame* Frame, int dfd, const char* pathname, uint32_t at_flags, const char* name) -> uint64_t {

  if (!FEX::HLE::_SyscallHandler->IsHostKernelVersionAtLeast(6, 13, 0)) {
    return -ENOSYS;
  }
  uint64_t Result = FEX::HLE::_SyscallHandler->FM.RemovexattrAt(dfd, pathname, at_flags, name);
  SYSCALL_ERRNO();
}
void RegisterFS(FEX::HLE::SyscallHandler* Handler) {
  using namespace FEXCore::IR;

  REGISTER_SYSCALL_IMPL(rename, rename);
  REGISTER_SYSCALL_IMPL(mkdir, mkdir);
  REGISTER_SYSCALL_IMPL(rmdir, rmdir);
  REGISTER_SYSCALL_IMPL(link, link);
  REGISTER_SYSCALL_IMPL(unlink, unlink);
  REGISTER_SYSCALL_IMPL(symlink, symlink);
  REGISTER_SYSCALL_IMPL(readlink, readlink);
  REGISTER_SYSCALL_IMPL(chmod, chmod);
  REGISTER_SYSCALL_IMPL(mknod, mknod);
  REGISTER_SYSCALL_IMPL(creat, creat);
  REGISTER_SYSCALL_IMPL(setxattr, setxattr);
  REGISTER_SYSCALL_IMPL(lsetxattr, lsetxattr);
  REGISTER_SYSCALL_IMPL(getxattr, getxattr);
  REGISTER_SYSCALL_IMPL(lgetxattr, lgetxattr);
  REGISTER_SYSCALL_IMPL(listxattr, listxattr);
  REGISTER_SYSCALL_IMPL(llistxattr, llistxattr);
  REGISTER_SYSCALL_IMPL(removexattr, removexattr);
  REGISTER_SYSCALL_IMPL(lremovexattr, lremovexattr);
  REGISTER_SYSCALL_IMPL(setxattrat, setxattrat);
  REGISTER_SYSCALL_IMPL(getxattrat, getxattrat);
  REGISTER_SYSCALL_IMPL(listxattrat, listxattrat);
  REGISTER_SYSCALL_IMPL(removexattrat, removexattrat);
}
} // namespace FEX::HLE
