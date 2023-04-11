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
#include <sys/socket.h>

namespace FEX::HLE {
  void RegisterSocket(FEX::HLE::SyscallHandler *Handler) {
    using namespace FEXCore::IR;

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(socket, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int domain, int type, int protocol) -> uint64_t {
      uint64_t Result = ::socket(domain, type, protocol);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(connect, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int sockfd, const struct sockaddr *addr, socklen_t addrlen) -> uint64_t {
      uint64_t Result = ::connect(sockfd, addr, addrlen);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(accept4, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags) -> uint64_t {
      uint64_t Result = ::accept4(sockfd, addr, addrlen, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(sendto, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) -> uint64_t {
      uint64_t Result = ::sendto(sockfd, buf, len, flags, dest_addr, addrlen);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(recvfrom, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) -> uint64_t {
      uint64_t Result = ::recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(shutdown, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int sockfd, int how) -> uint64_t {
      uint64_t Result = ::shutdown(sockfd, how);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(bind, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int sockfd, const struct sockaddr *addr, socklen_t addrlen) -> uint64_t {
      uint64_t Result = ::bind(sockfd, addr, addrlen);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(listen, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int sockfd, int backlog) -> uint64_t {
      uint64_t Result = ::listen(sockfd, backlog);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(getsockname, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int sockfd, struct sockaddr *addr, socklen_t *addrlen) -> uint64_t {
      uint64_t Result = ::getsockname(sockfd, addr, addrlen);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(getpeername, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int sockfd, struct sockaddr *addr, socklen_t *addrlen) -> uint64_t {
      uint64_t Result = ::getpeername(sockfd, addr, addrlen);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(socketpair, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int domain, int type, int protocol, int sv[2]) -> uint64_t {
      uint64_t Result = ::socketpair(domain, type, protocol, sv);
      SYSCALL_ERRNO();
    });
  }
}
