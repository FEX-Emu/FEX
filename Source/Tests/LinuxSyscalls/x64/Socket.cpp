/*
$info$
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include <FEXCore/Core/Context.h>

namespace FEX::HLE::x64 {
  void RegisterSocket() {
    REGISTER_SYSCALL_IMPL_X64(accept, [](FEXCore::Core::CpuStateFrame *Frame, int sockfd, struct sockaddr *addr, socklen_t *addrlen) -> uint64_t {
      uint64_t Result = ::accept(sockfd, addr, addrlen);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(recvmmsg, [](FEXCore::Core::CpuStateFrame *Frame, int sockfd, struct mmsghdr *msgvec, unsigned int vlen, int flags, struct timespec *timeout) -> uint64_t {
      uint64_t Result = ::recvmmsg(sockfd, msgvec, vlen, flags, timeout);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(sendmmsg, [](FEXCore::Core::CpuStateFrame *Frame, int sockfd, struct mmsghdr *msgvec, uint32_t vlen, int flags) -> uint64_t {
      uint64_t Result = ::sendmmsg(sockfd, msgvec, vlen, flags);
      SYSCALL_ERRNO();
    });
  }
}
