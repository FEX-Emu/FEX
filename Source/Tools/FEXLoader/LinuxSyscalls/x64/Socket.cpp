// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x64/Syscalls.h"

#include <stdint.h>
#include <sys/socket.h>

namespace FEXCore::Core {
  struct CpuStateFrame;
}

namespace FEX::HLE::x64 {
  void RegisterSocket(FEX::HLE::SyscallHandler *Handler) {
    REGISTER_SYSCALL_IMPL_X64_PASS(accept, [](FEXCore::Core::CpuStateFrame *Frame, int sockfd, struct sockaddr *addr, socklen_t *addrlen) -> uint64_t {
      uint64_t Result = ::accept(sockfd, addr, addrlen);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(recvmmsg, [](FEXCore::Core::CpuStateFrame *Frame, int sockfd, struct mmsghdr *msgvec, unsigned int vlen, int flags, struct timespec *timeout) -> uint64_t {
      uint64_t Result = ::recvmmsg(sockfd, msgvec, vlen, flags, timeout);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(sendmmsg, [](FEXCore::Core::CpuStateFrame *Frame, int sockfd, struct mmsghdr *msgvec, uint32_t vlen, int flags) -> uint64_t {
      uint64_t Result = ::sendmmsg(sockfd, msgvec, vlen, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(sendmsg, [](FEXCore::Core::CpuStateFrame *Frame, int sockfd, const struct msghdr *msg, int flags) -> uint64_t {
      uint64_t Result = ::sendmsg(sockfd, msg, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(recvmsg, [](FEXCore::Core::CpuStateFrame *Frame, int sockfd, struct msghdr *msg, int flags) -> uint64_t {
      uint64_t Result = ::recvmsg(sockfd, msg, flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(setsockopt, [](FEXCore::Core::CpuStateFrame *Frame, int sockfd, int level, int optname, const void *optval, socklen_t optlen) -> uint64_t {
      uint64_t Result = ::setsockopt(sockfd, level, optname, optval, optlen);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS(getsockopt, [](FEXCore::Core::CpuStateFrame *Frame, int sockfd, int level, int optname, void *optval, socklen_t *optlen) -> uint64_t {
      uint64_t Result = ::getsockopt(sockfd, level, optname, optval, optlen);
      SYSCALL_ERRNO();
    });
  }
}
