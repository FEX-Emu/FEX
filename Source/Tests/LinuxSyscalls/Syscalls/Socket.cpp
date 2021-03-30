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
#include <sys/socket.h>

namespace FEX::HLE {
  void RegisterSocket() {

    REGISTER_SYSCALL_IMPL(socket, [](FEXCore::Core::CpuStateFrame *Frame, int domain, int type, int protocol) -> uint64_t {
      uint64_t Result = ::socket(domain, type, protocol);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(connect, [](FEXCore::Core::CpuStateFrame *Frame, int sockfd, const struct sockaddr *addr, socklen_t addrlen) -> uint64_t {
      uint64_t Result = ::connect(sockfd, addr, addrlen);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(accept4, [](FEXCore::Core::CpuStateFrame *Frame, int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags) -> uint64_t {
      uint64_t Result = ::accept4(sockfd, addr, addrlen, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(sendto, [](FEXCore::Core::CpuStateFrame *Frame, int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) -> uint64_t {
      uint64_t Result = ::sendto(sockfd, buf, len, flags, dest_addr, addrlen);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(recvfrom, [](FEXCore::Core::CpuStateFrame *Frame, int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) -> uint64_t {
      uint64_t Result = ::recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(sendmsg, [](FEXCore::Core::CpuStateFrame *Frame, int sockfd, const struct msghdr *msg, int flags) -> uint64_t {
      uint64_t Result = ::sendmsg(sockfd, msg, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(recvmsg, [](FEXCore::Core::CpuStateFrame *Frame, int sockfd, struct msghdr *msg, int flags) -> uint64_t {
      uint64_t Result = ::recvmsg(sockfd, msg, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(shutdown, [](FEXCore::Core::CpuStateFrame *Frame, int sockfd, int how) -> uint64_t {
      uint64_t Result = ::shutdown(sockfd, how);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(bind, [](FEXCore::Core::CpuStateFrame *Frame, int sockfd, const struct sockaddr *addr, socklen_t addrlen) -> uint64_t {
      uint64_t Result = ::bind(sockfd, addr, addrlen);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(listen, [](FEXCore::Core::CpuStateFrame *Frame, int sockfd, int backlog) -> uint64_t {
      uint64_t Result = ::listen(sockfd, backlog);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(getsockname, [](FEXCore::Core::CpuStateFrame *Frame, int sockfd, struct sockaddr *addr, socklen_t *addrlen) -> uint64_t {
      uint64_t Result = ::getsockname(sockfd, addr, addrlen);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(getpeername, [](FEXCore::Core::CpuStateFrame *Frame, int sockfd, struct sockaddr *addr, socklen_t *addrlen) -> uint64_t {
      uint64_t Result = ::getpeername(sockfd, addr, addrlen);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(socketpair, [](FEXCore::Core::CpuStateFrame *Frame, int domain, int type, int protocol, int sv[2]) -> uint64_t {
      uint64_t Result = ::socketpair(domain, type, protocol, sv);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(setsockopt, [](FEXCore::Core::CpuStateFrame *Frame, int sockfd, int level, int optname, const void *optval, socklen_t optlen) -> uint64_t {
      uint64_t Result = ::setsockopt(sockfd, level, optname, optval, optlen);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(getsockopt, [](FEXCore::Core::CpuStateFrame *Frame, int sockfd, int level, int optname, void *optval, socklen_t *optlen) -> uint64_t {
      uint64_t Result = ::getsockopt(sockfd, level, optname, optval, optlen);
      SYSCALL_ERRNO();
    });
  }
}
