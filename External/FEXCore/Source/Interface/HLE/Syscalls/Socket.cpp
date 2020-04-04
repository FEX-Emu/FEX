#include "Interface/HLE/Syscalls.h"

#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {
  uint64_t Socket(FEXCore::Core::InternalThreadState *Thread, int domain, int type, int protocol) {
    uint64_t Result = ::socket(domain, type, protocol);
    SYSCALL_ERRNO();
  }

  uint64_t Connect(FEXCore::Core::InternalThreadState *Thread, int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    uint64_t Result = ::connect(sockfd, addr, addrlen);
    SYSCALL_ERRNO();
  }

  uint64_t Sendto(FEXCore::Core::InternalThreadState *Thread, int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) {
    uint64_t Result = ::sendto(sockfd, buf, len, flags, dest_addr, addrlen);
    SYSCALL_ERRNO();
  }

  uint64_t Recvfrom(FEXCore::Core::InternalThreadState *Thread, int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    uint64_t Result = ::recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
    SYSCALL_ERRNO();
  }

  uint64_t Sendmsg(FEXCore::Core::InternalThreadState *Thread, int sockfd, const struct msghdr *msg, int flags) {
    uint64_t Result = ::sendmsg(sockfd, msg, flags);
    SYSCALL_ERRNO();
  }

  uint64_t Recvmsg(FEXCore::Core::InternalThreadState *Thread, int sockfd, struct msghdr *msg, int flags) {
    uint64_t Result = ::recvmsg(sockfd, msg, flags);
    SYSCALL_ERRNO();
  }

  uint64_t Shutdown(FEXCore::Core::InternalThreadState *Thread, int sockfd, int how) {
    uint64_t Result = ::shutdown(sockfd, how);
    SYSCALL_ERRNO();
  }

  uint64_t Bind(FEXCore::Core::InternalThreadState *Thread, int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    uint64_t Result = ::bind(sockfd, addr, addrlen);
    SYSCALL_ERRNO();
  }

  uint64_t Listen(FEXCore::Core::InternalThreadState *Thread, int sockfd, int backlog) {
    uint64_t Result = ::listen(sockfd, backlog);
    SYSCALL_ERRNO();
  }

  uint64_t GetSockName(FEXCore::Core::InternalThreadState *Thread, int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    uint64_t Result = ::getsockname(sockfd, addr, addrlen);
    SYSCALL_ERRNO();
  }

  uint64_t GetPeerName(FEXCore::Core::InternalThreadState *Thread, int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    uint64_t Result = ::getpeername(sockfd, addr, addrlen);
    SYSCALL_ERRNO();
  }

  uint64_t SetSockOpt(FEXCore::Core::InternalThreadState *Thread, int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
    uint64_t Result = ::setsockopt(sockfd, level, optname, optval, optlen);
    SYSCALL_ERRNO();
  }

  uint64_t GetSockOpt(FEXCore::Core::InternalThreadState *Thread, int sockfd, int level, int optname, void *optval, socklen_t *optlen) {
    uint64_t Result = ::getsockopt(sockfd, level, optname, optval, optlen);
    SYSCALL_ERRNO();
  }

  uint64_t Sendmmsg(FEXCore::Core::InternalThreadState *Thread, int sockfd, struct mmsghdr *msgvec, uint32_t vlen, int flags) {
    uint64_t Result = ::sendmmsg(sockfd, msgvec, vlen, flags);
    SYSCALL_ERRNO();
  }
}
