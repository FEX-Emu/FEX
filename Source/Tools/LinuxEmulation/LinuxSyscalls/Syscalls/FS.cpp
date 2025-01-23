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
void RegisterFS(FEX::HLE::SyscallHandler* Handler) {
  using namespace FEXCore::IR;

  REGISTER_SYSCALL_IMPL_FLAGS(rename, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                              [](FEXCore::Core::CpuStateFrame* Frame, const char* oldpath, const char* newpath) -> uint64_t {
                                uint64_t Result = ::rename(oldpath, newpath);
                                SYSCALL_ERRNO();
                              });

  REGISTER_SYSCALL_IMPL_FLAGS(mkdir, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                              [](FEXCore::Core::CpuStateFrame* Frame, const char* pathname, mode_t mode) -> uint64_t {
                                uint64_t Result = ::mkdir(pathname, mode);
                                SYSCALL_ERRNO();
                              });

  REGISTER_SYSCALL_IMPL_FLAGS(rmdir, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                              [](FEXCore::Core::CpuStateFrame* Frame, const char* pathname) -> uint64_t {
                                uint64_t Result = ::rmdir(pathname);
                                SYSCALL_ERRNO();
                              });

  REGISTER_SYSCALL_IMPL_FLAGS(link, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                              [](FEXCore::Core::CpuStateFrame* Frame, const char* oldpath, const char* newpath) -> uint64_t {
                                uint64_t Result = ::link(oldpath, newpath);
                                SYSCALL_ERRNO();
                              });

  REGISTER_SYSCALL_IMPL_FLAGS(unlink, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                              [](FEXCore::Core::CpuStateFrame* Frame, const char* pathname) -> uint64_t {
                                uint64_t Result = ::unlink(pathname);
                                SYSCALL_ERRNO();
                              });

  REGISTER_SYSCALL_IMPL_FLAGS(symlink, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                              [](FEXCore::Core::CpuStateFrame* Frame, const char* target, const char* linkpath) -> uint64_t {
                                uint64_t Result = ::symlink(target, linkpath);
                                SYSCALL_ERRNO();
                              });

  REGISTER_SYSCALL_IMPL_FLAGS(readlink, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                              [](FEXCore::Core::CpuStateFrame* Frame, const char* pathname, char* buf, size_t bufsiz) -> uint64_t {
                                uint64_t Result = FEX::HLE::_SyscallHandler->FM.Readlink(pathname, buf, bufsiz);
                                SYSCALL_ERRNO();
                              });

  REGISTER_SYSCALL_IMPL_FLAGS(chmod, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                              [](FEXCore::Core::CpuStateFrame* Frame, const char* pathname, mode_t mode) -> uint64_t {
                                uint64_t Result = ::chmod(pathname, mode);
                                SYSCALL_ERRNO();
                              });

  REGISTER_SYSCALL_IMPL_FLAGS(mknod, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                              [](FEXCore::Core::CpuStateFrame* Frame, const char* pathname, mode_t mode, dev_t dev) -> uint64_t {
                                uint64_t Result = FEX::HLE::_SyscallHandler->FM.Mknod(pathname, mode, dev);
                                SYSCALL_ERRNO();
                              });

  REGISTER_SYSCALL_IMPL_FLAGS(creat, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                              [](FEXCore::Core::CpuStateFrame* Frame, const char* pathname, mode_t mode) -> uint64_t {
                                uint64_t Result = ::creat(pathname, mode);
                                SYSCALL_ERRNO();
                              });

  REGISTER_SYSCALL_IMPL(
    setxattr, [](FEXCore::Core::CpuStateFrame* Frame, const char* path, const char* name, const void* value, size_t size, int flags) -> uint64_t {
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.Setxattr(path, name, value, size, flags);
      SYSCALL_ERRNO();
    });

  REGISTER_SYSCALL_IMPL(
    lsetxattr, [](FEXCore::Core::CpuStateFrame* Frame, const char* path, const char* name, const void* value, size_t size, int flags) -> uint64_t {
      uint64_t Result = FEX::HLE::_SyscallHandler->FM.LSetxattr(path, name, value, size, flags);
      SYSCALL_ERRNO();
    });

  REGISTER_SYSCALL_IMPL(getxattr, [](FEXCore::Core::CpuStateFrame* Frame, const char* path, const char* name, void* value, size_t size) -> uint64_t {
    uint64_t Result = FEX::HLE::_SyscallHandler->FM.Getxattr(path, name, value, size);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL(lgetxattr, [](FEXCore::Core::CpuStateFrame* Frame, const char* path, const char* name, void* value, size_t size) -> uint64_t {
    uint64_t Result = FEX::HLE::_SyscallHandler->FM.LGetxattr(path, name, value, size);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL(listxattr, [](FEXCore::Core::CpuStateFrame* Frame, const char* path, char* list, size_t size) -> uint64_t {
    uint64_t Result = FEX::HLE::_SyscallHandler->FM.Listxattr(path, list, size);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL(llistxattr, [](FEXCore::Core::CpuStateFrame* Frame, const char* path, char* list, size_t size) -> uint64_t {
    uint64_t Result = FEX::HLE::_SyscallHandler->FM.LListxattr(path, list, size);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL(removexattr, [](FEXCore::Core::CpuStateFrame* Frame, const char* path, const char* name) -> uint64_t {
    uint64_t Result = FEX::HLE::_SyscallHandler->FM.Removexattr(path, name);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL(lremovexattr, [](FEXCore::Core::CpuStateFrame* Frame, const char* path, const char* name) -> uint64_t {
    uint64_t Result = FEX::HLE::_SyscallHandler->FM.LRemovexattr(path, name);
    SYSCALL_ERRNO();
  });
  if (Handler->IsHostKernelVersionAtLeast(6, 13, 0)) {
    REGISTER_SYSCALL_IMPL(setxattrat,
                          [](FEXCore::Core::CpuStateFrame* Frame, int dfd, const char* pathname, uint32_t at_flags, const char* name,
                             const FileManager::xattr_args* uargs, size_t usize) -> uint64_t {
                            uint64_t Result = FEX::HLE::_SyscallHandler->FM.SetxattrAt(dfd, pathname, at_flags, name, uargs, usize);
                            SYSCALL_ERRNO();
                          });
    REGISTER_SYSCALL_IMPL(getxattrat,
                          [](FEXCore::Core::CpuStateFrame* Frame, int dfd, const char* pathname, uint32_t at_flags, const char* name,
                             const FileManager::xattr_args* uargs, size_t usize) -> uint64_t {
                            uint64_t Result = FEX::HLE::_SyscallHandler->FM.GetxattrAt(dfd, pathname, at_flags, name, uargs, usize);
                            SYSCALL_ERRNO();
                          });

    REGISTER_SYSCALL_IMPL(
      listxattrat, [](FEXCore::Core::CpuStateFrame* Frame, int dfd, const char* pathname, uint32_t at_flags, char* list, size_t size) -> uint64_t {
        uint64_t Result = FEX::HLE::_SyscallHandler->FM.ListxattrAt(dfd, pathname, at_flags, list, size);
        SYSCALL_ERRNO();
      });
    REGISTER_SYSCALL_IMPL(
      removexattrat, [](FEXCore::Core::CpuStateFrame* Frame, int dfd, const char* pathname, uint32_t at_flags, const char* name) -> uint64_t {
        uint64_t Result = FEX::HLE::_SyscallHandler->FM.RemovexattrAt(dfd, pathname, at_flags, name);
        SYSCALL_ERRNO();
      });
  } else {
    REGISTER_SYSCALL_IMPL(setxattrat, UnimplementedSyscallSafe);
    REGISTER_SYSCALL_IMPL(getxattrat, UnimplementedSyscallSafe);
    REGISTER_SYSCALL_IMPL(listxattrat, UnimplementedSyscallSafe);
    REGISTER_SYSCALL_IMPL(removexattrat, UnimplementedSyscallSafe);
  }
}
} // namespace FEX::HLE
