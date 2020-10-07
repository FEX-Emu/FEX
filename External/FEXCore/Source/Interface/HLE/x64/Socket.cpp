#include "Interface/Context/Context.h"
#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"

namespace FEXCore::HLE::x64 {
  void RegisterSocket() {
    REGISTER_SYSCALL_IMPL_X64(accept, [](FEXCore::Core::InternalThreadState *Thread, int sockfd, struct sockaddr *addr, socklen_t *addrlen) -> uint64_t {
      uint64_t Result = ::accept(sockfd, addr, addrlen);
      SYSCALL_ERRNO();
    });
  }
}
