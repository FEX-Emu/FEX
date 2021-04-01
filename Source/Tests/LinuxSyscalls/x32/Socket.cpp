/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"

#include <FEXCore/Utils/LogManager.h>

#include <cstring>
#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>

ARG_TO_STR(FEX::HLE::x32::compat_ptr<FEX::HLE::x32::mmsghdr_32>, "%lx")

namespace FEX::HLE::x32 {
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
    REGISTER_SYSCALL_IMPL_X32(socketcall, [](FEXCore::Core::CpuStateFrame *Frame, uint32_t call, uint32_t *Arguments) -> uint64_t {
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

          HostHeader.msg_name = guest_msg->msg_name;
          HostHeader.msg_namelen = guest_msg->msg_namelen;

          HostHeader.msg_iov = &Host_iovec.at(0);
          HostHeader.msg_iovlen = guest_msg->msg_iovlen;

          HostHeader.msg_control = alloca(guest_msg->msg_controllen * 2);
          HostHeader.msg_controllen = guest_msg->msg_controllen;

          HostHeader.msg_flags = guest_msg->msg_flags;
          if (HostHeader.msg_controllen) {
            //fprintf(stderr, "sendmsg: msg_controllen: %ld\n", HostHeader.msg_controllen);
            void *CurrentGuestPtr = guest_msg->msg_control;
            struct cmsghdr *CurrentHost = reinterpret_cast<struct cmsghdr*>(HostHeader.msg_control);

            for (cmsghdr32 *msghdr_guest = reinterpret_cast<cmsghdr32*>(CurrentGuestPtr);
                CurrentGuestPtr != 0;
                msghdr_guest = reinterpret_cast<cmsghdr32*>(CurrentGuestPtr)) {

              CurrentHost->cmsg_level = msghdr_guest->cmsg_level;
              CurrentHost->cmsg_type = msghdr_guest->cmsg_type;

              if (msghdr_guest->cmsg_len) {
                size_t SizeIncrease = (CMSG_LEN(0) - sizeof(cmsghdr32));
                CurrentHost->cmsg_len = msghdr_guest->cmsg_len + SizeIncrease;
                //fprintf(stderr, "sendmsg: cmsg_len: %ld\n", CurrentHost->cmsg_len);
                HostHeader.msg_controllen += SizeIncrease;
                memcpy(CMSG_DATA(CurrentHost), msghdr_guest->cmsg_data, msghdr_guest->cmsg_len - sizeof(cmsghdr32));
              }

              // Go to next host
              CurrentHost = CMSG_NXTHDR(&HostHeader, CurrentHost);

              // Go to next msg
              if (msghdr_guest->cmsg_len < sizeof(cmsghdr32)) {
                CurrentGuestPtr = nullptr;
              }
              else {
                CurrentGuestPtr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(CurrentGuestPtr) + msghdr_guest->cmsg_len);
                CurrentGuestPtr = reinterpret_cast<void*>((reinterpret_cast<uintptr_t>(CurrentGuestPtr) + 3) & ~3ULL);
                if (CurrentGuestPtr >= reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(static_cast<void*>(guest_msg->msg_control)) + guest_msg->msg_controllen)) {
                  CurrentGuestPtr = nullptr;
                }
              }
            }
          }

          //fprintf(stderr, "sendmsg: msg_controllen final: %ld\n", HostHeader.msg_controllen);

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

          HostHeader.msg_name = guest_msg->msg_name;
          HostHeader.msg_namelen = guest_msg->msg_namelen;

          HostHeader.msg_iov = &Host_iovec.at(0);
          HostHeader.msg_iovlen = guest_msg->msg_iovlen;

          HostHeader.msg_control = alloca(guest_msg->msg_controllen+8);
          HostHeader.msg_controllen = guest_msg->msg_controllen+8;

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
              //fprintf(stderr, "recvmsg: msg_controllen: %ld\n", HostHeader.msg_controllen);
              // Host and guest cmsg data structures aren't compatible.
              // Copy them over now
              void *CurrentGuestPtr = guest_msg->msg_control;
              for (struct cmsghdr *cmsg = CMSG_FIRSTHDR(&HostHeader);
                  cmsg != nullptr;
                  cmsg = CMSG_NXTHDR(&HostHeader, cmsg)) {
                cmsghdr32 *CurrentGuest = reinterpret_cast<cmsghdr32*>(CurrentGuestPtr);

                // Copy over the header first
                // cmsg_len needs to be adjusted by the size of the header between host and guest
                // Host is 16 bytes, guest is 12 bytes
                CurrentGuest->cmsg_level = cmsg->cmsg_level;
                CurrentGuest->cmsg_type = cmsg->cmsg_type;

                // Now copy over the data
                if (cmsg->cmsg_len) {
                  size_t SizeIncrease = (CMSG_LEN(0) - sizeof(cmsghdr32));
                  CurrentGuest->cmsg_len = cmsg->cmsg_len - SizeIncrease;
                  //fprintf(stderr, "recvmsg: cmsg_len: %d\n", CurrentGuest->cmsg_len);

                  // Controllen size also changes
                  guest_msg->msg_controllen -= SizeIncrease;

                  memcpy(CurrentGuest->cmsg_data, CMSG_DATA(cmsg), cmsg->cmsg_len - sizeof(struct cmsghdr));
                  CurrentGuestPtr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(CurrentGuestPtr) + CurrentGuest->cmsg_len);
                  CurrentGuestPtr = reinterpret_cast<void*>((reinterpret_cast<uintptr_t>(CurrentGuestPtr) + 3) & ~3ULL);
                } else {
                  CurrentGuest->cmsg_len = 0;
                  fprintf(stderr, "cmsg_len == 0?!\n");
                }
              }
            }
            //fprintf(stderr, "recvmsg: msg_controllen: %d\n", guest_msg->msg_controllen);
          }
          break;
        }
        default:
          LogMan::Msg::A("Unsupported socketcall op: %d", call);
          break;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(sendmmsg, [](FEXCore::Core::CpuStateFrame *Frame, int sockfd, compat_ptr<mmsghdr_32> msgvec, uint32_t vlen, int flags) -> uint64_t {
      std::vector<iovec> Host_iovec;
      std::vector<uint8_t> Controllen;
      std::vector<struct msghdr> Messages{vlen};
      std::vector<struct mmsghdr> HostMmsg{vlen};

      // Walk the iovec and convert them
      // Calculate controllen at the same time
      size_t Controllen_size{};
      for (size_t i = 0; i < vlen; ++i) {
        msghdr32 &guest = msgvec[i].msg_hdr;

        Controllen_size += guest.msg_controllen * 2;
        for (size_t j = 0; j < guest.msg_iovlen; ++j) {
          iovec guest_iov = guest.msg_iov[j];
          Host_iovec.emplace_back(guest_iov);
        }
      }

      Controllen.resize(Controllen_size);

      size_t current_iov{};
      size_t current_controllen_offset{};
      for (size_t i = 0; i < vlen; ++i) {
        msghdr32 &guest = msgvec[i].msg_hdr;
        struct msghdr &msg = HostMmsg[i].msg_hdr;
        msg.msg_name = guest.msg_name;
        msg.msg_namelen = guest.msg_namelen;

        msg.msg_iov = &Host_iovec.at(current_iov);
        msg.msg_iovlen = guest.msg_iovlen;
        current_iov += msg.msg_iovlen;

        if (guest.msg_controllen) {
          msg.msg_control = &Controllen.at(current_controllen_offset);
          current_controllen_offset += guest.msg_controllen * 2;
        }
        msg.msg_controllen = guest.msg_controllen;

        msg.msg_flags = guest.msg_flags;

        if (msg.msg_controllen) {
          void *CurrentGuestPtr = guest.msg_control;
          struct cmsghdr *CurrentHost = reinterpret_cast<struct cmsghdr*>(msg.msg_control);

          for (cmsghdr32 *msghdr_guest = reinterpret_cast<cmsghdr32*>(CurrentGuestPtr);
              CurrentGuestPtr != 0;
              msghdr_guest = reinterpret_cast<cmsghdr32*>(CurrentGuestPtr)) {

            CurrentHost->cmsg_level = msghdr_guest->cmsg_level;
            CurrentHost->cmsg_type = msghdr_guest->cmsg_type;

            if (msghdr_guest->cmsg_len) {
              size_t SizeIncrease = (CMSG_LEN(0) - sizeof(cmsghdr32));
              CurrentHost->cmsg_len = msghdr_guest->cmsg_len + SizeIncrease;
              msg.msg_controllen += SizeIncrease;
              memcpy(CMSG_DATA(CurrentHost), msghdr_guest->cmsg_data, msghdr_guest->cmsg_len - sizeof(cmsghdr32));
            }

            // Go to next host
            CurrentHost = CMSG_NXTHDR(&msg, CurrentHost);

            // Go to next msg
            if (msghdr_guest->cmsg_len < sizeof(cmsghdr32)) {
              CurrentGuestPtr = nullptr;
            }
            else {
              CurrentGuestPtr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(CurrentGuestPtr) + msghdr_guest->cmsg_len);
              if (CurrentGuestPtr >= reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(static_cast<void*>(guest.msg_control)) + guest.msg_controllen)) {
                CurrentGuestPtr = nullptr;
              }
            }
          }
        }

        HostMmsg[i].msg_len = msgvec[i].msg_len;
      }

      uint64_t Result = ::sendmmsg(sockfd, &HostMmsg.at(0), vlen, flags);

      if (Result != -1) {
        // Update guest msglen
        for (size_t i = 0; i < Result; ++i) {
          msgvec[i].msg_len = HostMmsg[i].msg_len;
        }
      }
      SYSCALL_ERRNO();
    });
  }
}
