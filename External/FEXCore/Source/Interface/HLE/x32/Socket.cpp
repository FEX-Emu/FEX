#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x32/Syscalls.h"

#include "Interface/Context/Context.h"
#include "Interface/Core/InternalThreadState.h"

#include <FEXCore/Utils/LogManager.h>

#include <cstring>
#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>

namespace FEXCore::HLE::x32 {
  enum SockOp {
    OP_SOCKET = 1,
    OP_BIND = 2,
    OP_CONNECT = 3,
    OP_LISTEN = 4,
    OP_ACCEPT = 5,
    OP_GETSOCKNAME = 6,
    OP_GETPEERNAME = 7,
    OP_SOCKETPAIR = 8,
    OP_SEND = 9,
    OP_RECV = 10,
    OP_SENDTO = 11,
    OP_RECVFROM = 12,
    OP_SHUTDOWN = 13,
    OP_SETSOCKOPT = 14,
    OP_GETSOCKOPT = 15,
    OP_SENDMSG = 16,
    OP_RECVMSG = 17,
    OP_ACCEPT4 = 18,
    OP_RECVMMSG = 19,
    OP_SENDMMSG = 20,
  };

  void RegisterSocket() {
    REGISTER_SYSCALL_IMPL_X32(socketcall, [](FEXCore::Core::InternalThreadState *Thread, uint32_t call, uint32_t *Arguments) -> uint64_t {
      uint64_t Result{};

      switch (call) {
        case OP_SOCKET: {
          Result = ::socket(Arguments[0], Arguments[1], Arguments[2]);
          break;
        }
        case OP_BIND: {
          Result = ::bind(Arguments[0], reinterpret_cast<const struct sockaddr *>(Arguments[1]), Arguments[2]);
          break;
        }
        case OP_CONNECT: {
          Result = ::connect(Arguments[0], reinterpret_cast<const struct sockaddr *>(Arguments[1]), Arguments[2]);
          break;
        }
        case OP_LISTEN: {
          Result = ::listen(Arguments[0], Arguments[1]);
          break;
        }
        case OP_ACCEPT: {
          Result = ::accept(Arguments[0], reinterpret_cast<struct sockaddr *>(Arguments[1]), reinterpret_cast<socklen_t*>(Arguments[2]));
          break;
        }
        case OP_GETSOCKNAME: {
          Result = ::getsockname(Arguments[0], reinterpret_cast<struct sockaddr *>(Arguments[1]), reinterpret_cast<socklen_t*>(Arguments[2]));
          break;
        }
        case OP_GETPEERNAME: {
          Result = ::getpeername(Arguments[0], reinterpret_cast<struct sockaddr *>(Arguments[1]), reinterpret_cast<socklen_t*>(Arguments[2]));
          break;
        }
        case OP_SOCKETPAIR: {
          Result = ::socketpair(Arguments[0], Arguments[1], Arguments[2], reinterpret_cast<int32_t*>(Arguments[3]));
          break;
        }
        case OP_SEND: {
          Result = ::send(Arguments[0], reinterpret_cast<const void*>(Arguments[1]), Arguments[2], Arguments[3]);
          break;
        }
        case OP_RECV: {
          Result = ::recv(Arguments[0], reinterpret_cast<void*>(Arguments[1]), Arguments[2], Arguments[3]);
          break;
        }
        case OP_SENDTO: {
          Result = ::sendto(
            Arguments[0],
            reinterpret_cast<const void*>(Arguments[1]),
            Arguments[2],
            Arguments[3],
            reinterpret_cast<struct sockaddr *>(Arguments[4]), reinterpret_cast<socklen_t>(Arguments[5])
            );
          break;
        }
        case OP_RECVFROM: {
          Result = ::recvfrom(
            Arguments[0],
            reinterpret_cast<void*>(Arguments[1]),
            Arguments[2],
            Arguments[3],
            reinterpret_cast<struct sockaddr *>(Arguments[4]), reinterpret_cast<socklen_t*>(Arguments[5])
            );
          break;
        }
        case OP_SHUTDOWN: {
          Result = ::shutdown(Arguments[0], Arguments[1]);
          break;
        }
        case OP_SETSOCKOPT: {
          Result = ::setsockopt(
            Arguments[0],
            Arguments[1],
            Arguments[2],
            reinterpret_cast<const void*>(Arguments[3]),
            reinterpret_cast<socklen_t>(Arguments[4])
            );
          break;
        }
        case OP_GETSOCKOPT: {
          Result = ::getsockopt(
            Arguments[0],
            Arguments[1],
            Arguments[2],
            reinterpret_cast<void*>(Arguments[3]),
            reinterpret_cast<socklen_t*>(Arguments[4])
            );
          break;
        }
        case OP_SENDMSG: {
          const struct msghdr32 *guest_msg = reinterpret_cast<const struct msghdr32*>(Arguments[1]);

          struct msghdr HostHeader{};
          std::vector<iovec> Host_iovec(guest_msg->msg_iovlen);
          for (int i = 0; i < guest_msg->msg_iovlen; ++i) {
            Host_iovec[i] = guest_msg->msg_iov[i];
          }

          HostHeader.msg_name = reinterpret_cast<void*>(guest_msg->msg_name_ptr);
          HostHeader.msg_namelen = guest_msg->msg_namelen;

          HostHeader.msg_iov = &Host_iovec.at(0);
          HostHeader.msg_iovlen = guest_msg->msg_iovlen;

          HostHeader.msg_control = reinterpret_cast<void*>(guest_msg->msg_control);
          HostHeader.msg_controllen = guest_msg->msg_controllen;

          HostHeader.msg_flags = guest_msg->msg_flags;

          Result = ::sendmsg(Arguments[0], &HostHeader, Arguments[2]);
          break;
        }
        case OP_RECVMSG: {
          struct msghdr32 *guest_msg = reinterpret_cast<struct msghdr32*>(Arguments[1]);

          struct msghdr HostHeader{};
          std::vector<iovec> Host_iovec(guest_msg->msg_iovlen);
          for (int i = 0; i < guest_msg->msg_iovlen; ++i) {
            Host_iovec[i] = guest_msg->msg_iov[i];
          }

          HostHeader.msg_name = reinterpret_cast<void*>(guest_msg->msg_name_ptr);
          HostHeader.msg_namelen = guest_msg->msg_namelen;

          HostHeader.msg_iov = &Host_iovec.at(0);
          HostHeader.msg_iovlen = guest_msg->msg_iovlen;

          HostHeader.msg_control = alloca(guest_msg->msg_controllen);
          HostHeader.msg_controllen = guest_msg->msg_controllen;

          HostHeader.msg_flags = guest_msg->msg_flags;

          Result = ::recvmsg(Arguments[0], &HostHeader, Arguments[2]);
          if (Result != -1) {
            for (int i = 0; i < guest_msg->msg_iovlen; ++i) {
              guest_msg->msg_iov[i] = Host_iovec[i];
            }

            guest_msg->msg_namelen = HostHeader.msg_namelen;

            guest_msg->msg_controllen = HostHeader.msg_controllen;

            guest_msg->msg_flags = HostHeader.msg_flags;
            if (HostHeader.msg_controllen) {
              // Host and guest cmsg data structures aren't compatible.
              // Copy them over now
              uint32_t CurrentGuestPtr = guest_msg->msg_control;
              for (struct cmsghdr *cmsg = CMSG_FIRSTHDR(&HostHeader);
                  cmsg != nullptr;
                  cmsg = CMSG_NXTHDR(&HostHeader, cmsg)) {
                cmsghdr32 *CurrentGuest = reinterpret_cast<cmsghdr32*>(CurrentGuestPtr);

                // Copy over the header first
                CurrentGuest->cmsg_len = cmsg->cmsg_len;
                CurrentGuest->cmsg_level = cmsg->cmsg_level;
                CurrentGuest->cmsg_type = cmsg->cmsg_type;

                CurrentGuestPtr += sizeof(cmsghdr32);

                // Offset the result by the size of the structure change
                Result -= sizeof(cmsghdr) - sizeof(cmsghdr32);

                // Now copy over the data
                if (cmsg->cmsg_len) {
                  uint8_t *GuestData = reinterpret_cast<uint8_t*>(CurrentGuestPtr);
                  memcpy(GuestData, CMSG_DATA(cmsg), cmsg->cmsg_len);
                  CurrentGuestPtr += cmsg->cmsg_len;
                }
              }
            }
          }
          break;
        }
        default:
          LogMan::Msg::A("Unsupported socketcall op: %d", call);
          break;
      }
      SYSCALL_ERRNO();
    });
  }
}
