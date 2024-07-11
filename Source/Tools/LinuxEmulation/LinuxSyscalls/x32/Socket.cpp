// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x32/Syscalls.h"
#include "LinuxSyscalls/x32/Types.h"
#include "LinuxSyscalls/x64/Syscalls.h"

#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/vector.h>

#include <alloca.h>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stddef.h>
#include <sys/socket.h>
#include <unistd.h>

ARG_TO_STR(FEX::HLE::x32::compat_ptr<FEX::HLE::x32::mmsghdr_32>, "%lx")
ARG_TO_STR(FEX::HLE::x32::compat_ptr<void>, "%lx")
ARG_TO_STR(FEX::HLE::x32::compat_ptr<uint32_t>, "%lx")

namespace FEXCore::Core {
struct CpuStateFrame;
}

namespace FEX::HLE::x32 {

// Some sockopt defines for older build environments
#ifndef SO_RCVTIMEO_OLD
#define SO_RCVTIMEO_OLD 20
#endif
#ifndef SO_SNDTIMEO_OLD
#define SO_SNDTIMEO_OLD 21
#endif
#ifndef SO_TIMESTAMP_OLD
#define SO_TIMESTAMP_OLD 29
#endif
#ifndef SO_TIMESTAMPNS_OLD
#define SO_TIMESTAMPNS_OLD 35
#endif
#ifndef SO_TIMESTAMPING_OLD
#define SO_TIMESTAMPING_OLD 37
#endif
#ifndef SO_MEMINFO
#define SO_MEMINFO 55
#endif
#ifndef SO_INCOMING_NAPI_ID
#define SO_INCOMING_NAPI_ID 56
#endif
#ifndef SO_PEERGROUPS
#define SO_PEERGROUPS 59
#endif
#ifndef SO_ZEROCOPY
#define SO_ZEROCOPY 60
#endif
#ifndef SO_TXTIME
#define SO_TXTIME 61
#endif
#ifndef SO_BINDTOIFINDEX
#define SO_BINDTOIFINDEX 62
#endif
#ifndef SO_TIMESTAMP_NEW
#define SO_TIMESTAMP_NEW 63
#endif
#ifndef SO_TIMESTAMPNS_NEW
#define SO_TIMESTAMPNS_NEW 64
#endif
#ifndef SO_TIMESTAMPING_NEW
#define SO_TIMESTAMPING_NEW 65
#endif
#ifndef SO_RCVTIMEO_NEW
#define SO_RCVTIMEO_NEW 66
#endif
#ifndef SO_SNDTIMEO_NEW
#define SO_SNDTIMEO_NEW 67
#endif
#ifndef SO_DETACH_REUSEPORT_BPF
#define SO_DETACH_REUSEPORT_BPF 68
#endif
#ifndef SO_PREFER_BUSY_POLL
#define SO_PREFER_BUSY_POLL 69
#endif
#ifndef SO_BUSY_POLL_BUDGET
#define SO_BUSY_POLL_BUDGET 70
#endif
#ifndef SO_NETNS_COOKIE
#define SO_NETNS_COOKIE 71
#endif
#ifndef SO_BUF_LOCK
#define SO_BUF_LOCK 72
#endif
#ifndef SO_RESERVE_MEM
#define SO_RESERVE_MEM 73
#endif
#ifndef SO_TXREHASH
#define SO_TXREHASH 74
#endif
#ifndef SO_RCVMARK
#define SO_RCVMARK 75
#endif
#ifndef SO_PASSPIDFD
#define SO_PASSPIDFD 76
#endif
#ifndef SO_PEERPIDFD
#define SO_PEERPIDFD 77
#endif

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

static uint64_t SendMsg(int sockfd, const struct msghdr32* msg, int flags) {
  struct msghdr HostHeader {};
  fextl::vector<iovec> Host_iovec(msg->msg_iovlen);
  for (size_t i = 0; i < msg->msg_iovlen; ++i) {
    Host_iovec[i] = msg->msg_iov[i];
  }

  HostHeader.msg_name = msg->msg_name;
  HostHeader.msg_namelen = msg->msg_namelen;

  HostHeader.msg_iov = Host_iovec.data();
  HostHeader.msg_iovlen = msg->msg_iovlen;

  HostHeader.msg_control = alloca(msg->msg_controllen * 2);
  HostHeader.msg_controllen = msg->msg_controllen;

  HostHeader.msg_flags = msg->msg_flags;
  if (HostHeader.msg_controllen) {
    void* CurrentGuestPtr = msg->msg_control;
    struct cmsghdr* CurrentHost = reinterpret_cast<struct cmsghdr*>(HostHeader.msg_control);

    for (cmsghdr32* msghdr_guest = reinterpret_cast<cmsghdr32*>(CurrentGuestPtr); CurrentGuestPtr != 0;
         msghdr_guest = reinterpret_cast<cmsghdr32*>(CurrentGuestPtr)) {

      CurrentHost->cmsg_level = msghdr_guest->cmsg_level;
      CurrentHost->cmsg_type = msghdr_guest->cmsg_type;

      if (msghdr_guest->cmsg_len) {
        size_t SizeIncrease = (CMSG_LEN(0) - sizeof(cmsghdr32));
        CurrentHost->cmsg_len = msghdr_guest->cmsg_len + SizeIncrease;
        HostHeader.msg_controllen += SizeIncrease;
        memcpy(CMSG_DATA(CurrentHost), msghdr_guest->cmsg_data, msghdr_guest->cmsg_len - sizeof(cmsghdr32));
      }

      // Go to next host
      CurrentHost = CMSG_NXTHDR(&HostHeader, CurrentHost);

      // Go to next msg
      if (msghdr_guest->cmsg_len < sizeof(cmsghdr32)) {
        CurrentGuestPtr = nullptr;
      } else {
        CurrentGuestPtr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(CurrentGuestPtr) + msghdr_guest->cmsg_len);
        CurrentGuestPtr = reinterpret_cast<void*>((reinterpret_cast<uintptr_t>(CurrentGuestPtr) + 3) & ~3ULL);
        if (CurrentGuestPtr >= reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(static_cast<void*>(msg->msg_control)) + msg->msg_controllen)) {
          CurrentGuestPtr = nullptr;
        }
      }
    }
  }

  uint64_t Result = ::sendmsg(sockfd, &HostHeader, flags);
  SYSCALL_ERRNO();
}

static uint64_t RecvMsg(int sockfd, struct msghdr32* msg, int flags) {
  struct msghdr HostHeader {};
  fextl::vector<iovec> Host_iovec(msg->msg_iovlen);
  for (size_t i = 0; i < msg->msg_iovlen; ++i) {
    Host_iovec[i] = msg->msg_iov[i];
  }

  HostHeader.msg_name = msg->msg_name;
  HostHeader.msg_namelen = msg->msg_namelen;

  HostHeader.msg_iov = Host_iovec.data();
  HostHeader.msg_iovlen = msg->msg_iovlen;

  HostHeader.msg_control = alloca(msg->msg_controllen * 2);
  HostHeader.msg_controllen = msg->msg_controllen * 2;

  HostHeader.msg_flags = msg->msg_flags;

  uint64_t Result = ::recvmsg(sockfd, &HostHeader, flags);
  if (Result != -1) {
    for (size_t i = 0; i < msg->msg_iovlen; ++i) {
      msg->msg_iov[i] = Host_iovec[i];
    }

    msg->msg_namelen = HostHeader.msg_namelen;
    msg->msg_controllen = HostHeader.msg_controllen;
    msg->msg_flags = HostHeader.msg_flags;
    if (HostHeader.msg_controllen) {
      // Host and guest cmsg data structures aren't compatible.
      // Copy them over now
      void* CurrentGuestPtr = msg->msg_control;
      for (struct cmsghdr* cmsg = CMSG_FIRSTHDR(&HostHeader); cmsg != nullptr; cmsg = CMSG_NXTHDR(&HostHeader, cmsg)) {
        cmsghdr32* CurrentGuest = reinterpret_cast<cmsghdr32*>(CurrentGuestPtr);

        // Copy over the header first
        // cmsg_len needs to be adjusted by the size of the header between host and guest
        // Host is 16 bytes, guest is 12 bytes
        CurrentGuest->cmsg_level = cmsg->cmsg_level;
        CurrentGuest->cmsg_type = cmsg->cmsg_type;

        // Now copy over the data
        if (cmsg->cmsg_len) {
          size_t SizeIncrease = (CMSG_LEN(0) - sizeof(cmsghdr32));
          CurrentGuest->cmsg_len = cmsg->cmsg_len - SizeIncrease;

          // Controllen size also changes
          msg->msg_controllen -= SizeIncrease;

          memcpy(CurrentGuest->cmsg_data, CMSG_DATA(cmsg), cmsg->cmsg_len - sizeof(struct cmsghdr));
          CurrentGuestPtr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(CurrentGuestPtr) + CurrentGuest->cmsg_len);
          CurrentGuestPtr = reinterpret_cast<void*>((reinterpret_cast<uintptr_t>(CurrentGuestPtr) + 3) & ~3ULL);
        }
      }
    }
  }
  SYSCALL_ERRNO();
}

void ConvertHeaderToHost(fextl::vector<iovec>& iovec, struct msghdr* Host, const struct msghdr32* Guest) {
  size_t CurrentIOVecSize = iovec.size();
  iovec.resize(CurrentIOVecSize + Guest->msg_iovlen);
  for (size_t i = 0; i < Guest->msg_iovlen; ++i) {
    iovec[CurrentIOVecSize + i] = Guest->msg_iov[i];
  }

  Host->msg_name = Guest->msg_name;
  Host->msg_namelen = Guest->msg_namelen;

  Host->msg_iov = &iovec[CurrentIOVecSize];
  Host->msg_iovlen = Guest->msg_iovlen;

  // XXX: This could result in a stack overflow
  Host->msg_control = alloca(Guest->msg_controllen * 2);
  Host->msg_controllen = Guest->msg_controllen * 2;

  Host->msg_flags = Guest->msg_flags;
}

void ConvertHeaderToGuest(struct msghdr32* Guest, struct msghdr* Host) {
  for (size_t i = 0; i < Guest->msg_iovlen; ++i) {
    Guest->msg_iov[i] = Host->msg_iov[i];
  }

  Guest->msg_namelen = Host->msg_namelen;
  Guest->msg_controllen = Host->msg_controllen;
  Guest->msg_flags = Host->msg_flags;

  if (Host->msg_controllen) {
    // Host and guest cmsg data structures aren't compatible.
    // Copy them over now
    void* CurrentGuestPtr = Guest->msg_control;
    for (struct cmsghdr* cmsg = CMSG_FIRSTHDR(Host); cmsg != nullptr; cmsg = CMSG_NXTHDR(Host, cmsg)) {
      cmsghdr32* CurrentGuest = reinterpret_cast<cmsghdr32*>(CurrentGuestPtr);

      // Copy over the header first
      // cmsg_len needs to be adjusted by the size of the header between host and guest
      // Host is 16 bytes, guest is 12 bytes
      CurrentGuest->cmsg_level = cmsg->cmsg_level;
      CurrentGuest->cmsg_type = cmsg->cmsg_type;

      // Now copy over the data
      if (cmsg->cmsg_len) {
        size_t SizeIncrease = (CMSG_LEN(0) - sizeof(cmsghdr32));
        CurrentGuest->cmsg_len = cmsg->cmsg_len - SizeIncrease;

        // Controllen size also changes
        Guest->msg_controllen -= SizeIncrease;

        memcpy(CurrentGuest->cmsg_data, CMSG_DATA(cmsg), cmsg->cmsg_len - sizeof(struct cmsghdr));
        CurrentGuestPtr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(CurrentGuestPtr) + CurrentGuest->cmsg_len);
        CurrentGuestPtr = reinterpret_cast<void*>((reinterpret_cast<uintptr_t>(CurrentGuestPtr) + 3) & ~3ULL);
      }
    }
  }
}

static uint64_t RecvMMsg(int sockfd, auto_compat_ptr<mmsghdr_32> msgvec, uint32_t vlen, int flags, struct timespec* timeout_ts) {
  fextl::vector<iovec> Host_iovec;
  fextl::vector<struct mmsghdr> HostMHeader(vlen);
  for (size_t i = 0; i < vlen; ++i) {
    ConvertHeaderToHost(Host_iovec, &HostMHeader[i].msg_hdr, &msgvec[i].msg_hdr);
    HostMHeader[i].msg_len = msgvec[i].msg_len;
  }
  uint64_t Result = ::recvmmsg(sockfd, HostMHeader.data(), vlen, flags, timeout_ts);
  if (Result != -1) {
    for (size_t i = 0; i < Result; ++i) {
      ConvertHeaderToGuest(&msgvec[i].msg_hdr, &HostMHeader[i].msg_hdr);
      msgvec[i].msg_len = HostMHeader[i].msg_len;
    }
  }
  SYSCALL_ERRNO();
}

static uint64_t SendMMsg(int sockfd, auto_compat_ptr<mmsghdr_32> msgvec, uint32_t vlen, int flags) {
  fextl::vector<iovec> Host_iovec;
  fextl::vector<struct mmsghdr> HostMmsg(vlen);

  // Walk the iovec and convert them
  // Calculate controllen at the same time
  size_t Controllen_size {};
  for (size_t i = 0; i < vlen; ++i) {
    msghdr32& guest = msgvec[i].msg_hdr;

    Controllen_size += guest.msg_controllen * 2;
    for (size_t j = 0; j < guest.msg_iovlen; ++j) {
      iovec guest_iov = guest.msg_iov[j];
      Host_iovec.emplace_back(guest_iov);
    }
  }

  fextl::vector<uint8_t> Controllen(Controllen_size);

  size_t current_iov {};
  size_t current_controllen_offset {};
  for (size_t i = 0; i < vlen; ++i) {
    msghdr32& guest = msgvec[i].msg_hdr;
    struct msghdr& msg = HostMmsg[i].msg_hdr;
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
      void* CurrentGuestPtr = guest.msg_control;
      struct cmsghdr* CurrentHost = reinterpret_cast<struct cmsghdr*>(msg.msg_control);

      for (cmsghdr32* msghdr_guest = reinterpret_cast<cmsghdr32*>(CurrentGuestPtr); CurrentGuestPtr != 0;
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
        } else {
          CurrentGuestPtr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(CurrentGuestPtr) + msghdr_guest->cmsg_len);
          CurrentGuestPtr = reinterpret_cast<void*>((reinterpret_cast<uintptr_t>(CurrentGuestPtr) + 3) & ~3ULL);
          if (CurrentGuestPtr >= reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(static_cast<void*>(guest.msg_control)) + guest.msg_controllen)) {
            CurrentGuestPtr = nullptr;
          }
        }
      }
    }

    HostMmsg[i].msg_len = msgvec[i].msg_len;
  }

  uint64_t Result = ::sendmmsg(sockfd, HostMmsg.data(), vlen, flags);

  if (Result != -1) {
    // Update guest msglen
    for (size_t i = 0; i < Result; ++i) {
      msgvec[i].msg_len = HostMmsg[i].msg_len;
    }
  }
  SYSCALL_ERRNO();
}

static uint64_t SetSockOpt(int sockfd, int level, int optname, auto_compat_ptr<void> optval, int optlen) {
  uint64_t Result {};

  if (level == SOL_SOCKET) {
    switch (optname) {
    case SO_ATTACH_FILTER:
    case SO_ATTACH_REUSEPORT_CBPF: {
      struct sock_fprog32 {
        uint16_t len;
        uint32_t filter;
      };
      struct sock_fprog64 {
        uint16_t len;
        uint64_t filter;
      };

      if (optlen != sizeof(sock_fprog32)) {
        return -EINVAL;
      }

      sock_fprog32* prog = reinterpret_cast<sock_fprog32*>(optval.Ptr);
      sock_fprog64 prog64 {};
      prog64.len = prog->len;
      prog64.filter = prog->filter;

      Result = ::syscall(SYSCALL_DEF(setsockopt), sockfd, level, optname, &prog64, sizeof(sock_fprog64));
      break;
    }
    case SO_RCVTIMEO_OLD: {
      // _OLD uses old_timeval32. Needs to be converted
      struct timeval tv64 = *reinterpret_cast<timeval32*>(optval.Ptr);
      Result = ::syscall(SYSCALL_DEF(setsockopt), sockfd, level, SO_RCVTIMEO_NEW, &tv64, sizeof(tv64));
      break;
    }
    case SO_SNDTIMEO_OLD: {
      // _OLD uses old_timeval32. Needs to be converted
      struct timeval tv64 = *reinterpret_cast<timeval32*>(optval.Ptr);
      Result = ::syscall(SYSCALL_DEF(setsockopt), sockfd, level, SO_SNDTIMEO_NEW, &tv64, sizeof(tv64));
      break;
    }
    // Each optname as a reminder which setting has been manually checked
    case SO_DEBUG:
    case SO_REUSEADDR:
    case SO_TYPE:
    case SO_ERROR:
    case SO_DONTROUTE:
    case SO_BROADCAST:
    case SO_SNDBUF:
    case SO_RCVBUF:
    case SO_SNDBUFFORCE:
    case SO_RCVBUFFORCE:
    case SO_KEEPALIVE:
    case SO_OOBINLINE:
    case SO_NO_CHECK:
    case SO_PRIORITY:
    case SO_LINGER:
    case SO_BSDCOMPAT:
    case SO_REUSEPORT:
    /**
     * @name These end up differing between {x86,arm} and {powerpc, alpha, sparc, mips, parisc}
     * @{ */
    case SO_PASSCRED:
    case SO_PEERCRED:
    case SO_RCVLOWAT:
    case SO_SNDLOWAT:
    /**  @} */
    case SO_SECURITY_AUTHENTICATION:
    case SO_SECURITY_ENCRYPTION_TRANSPORT:
    case SO_SECURITY_ENCRYPTION_NETWORK:
    case SO_DETACH_FILTER:
    case SO_PEERNAME:
    case SO_TIMESTAMP_OLD: // Returns int32_t boolean
    case SO_ACCEPTCONN:
    case SO_PEERSEC:
    // Gap 32, 33
    case SO_PASSSEC:
    case SO_TIMESTAMPNS_OLD: // Returns int32_t boolean
    case SO_MARK:
    case SO_TIMESTAMPING_OLD: // Returns so_timestamping
    case SO_PROTOCOL:
    case SO_DOMAIN:
    case SO_RXQ_OVFL:
    case SO_WIFI_STATUS:
    case SO_PEEK_OFF:
    case SO_NOFCS:
    case SO_LOCK_FILTER:
    case SO_SELECT_ERR_QUEUE:
    case SO_BUSY_POLL:
    case SO_MAX_PACING_RATE:
    case SO_BPF_EXTENSIONS:
    case SO_INCOMING_CPU:
    case SO_ATTACH_BPF:
    case SO_ATTACH_REUSEPORT_EBPF:
    case SO_CNX_ADVICE:
    // Gap 54 (SCM_TIMESTAMPING_OPT_STATS)
    case SO_MEMINFO:
    case SO_INCOMING_NAPI_ID:
    case SO_COOKIE: // Cookie always returns 64-bit even on 32-bit
    // Gap 58 (SCM_TIMESTAMPING_PKTINFO)
    case SO_PEERGROUPS:
    case SO_ZEROCOPY:
    case SO_TXTIME:
    case SO_BINDTOIFINDEX:
    case SO_TIMESTAMP_NEW:
    case SO_TIMESTAMPNS_NEW:
    case SO_TIMESTAMPING_NEW:
    case SO_RCVTIMEO_NEW:
    case SO_SNDTIMEO_NEW:
    case SO_DETACH_REUSEPORT_BPF:
    case SO_PREFER_BUSY_POLL:
    case SO_BUSY_POLL_BUDGET:
    case SO_NETNS_COOKIE: // Cookie always returns 64-bit even on 32-bit
    case SO_BUF_LOCK:
    case SO_RESERVE_MEM:
    case SO_TXREHASH:
    case SO_RCVMARK:
    case SO_PASSPIDFD:
    case SO_PEERPIDFD:
    default: Result = ::syscall(SYSCALL_DEF(setsockopt), sockfd, level, optname, reinterpret_cast<const void*>(optval.Ptr), optlen); break;
    }
  } else {
    Result = ::syscall(SYSCALL_DEF(setsockopt), sockfd, level, optname, reinterpret_cast<const void*>(optval.Ptr), optlen);
  }

  SYSCALL_ERRNO();
}

static uint64_t GetSockOpt(int sockfd, int level, int optname, auto_compat_ptr<void> optval, auto_compat_ptr<socklen_t> optlen) {
  uint64_t Result {};
  if (level == SOL_SOCKET) {
    switch (optname) {
    case SO_RCVTIMEO_OLD: {
      // _OLD uses old_timeval32. Needs to be converted
      struct timeval tv64 {};
      Result = ::syscall(SYSCALL_DEF(getsockopt), sockfd, level, SO_RCVTIMEO_NEW, &tv64, sizeof(tv64));
      *reinterpret_cast<timeval32*>(optval.Ptr) = tv64;
      break;
    }
    case SO_SNDTIMEO_OLD: {
      // _OLD uses old_timeval32. Needs to be converted
      struct timeval tv64 {};
      Result = ::syscall(SYSCALL_DEF(getsockopt), sockfd, level, SO_SNDTIMEO_NEW, &tv64, sizeof(tv64));
      *reinterpret_cast<timeval32*>(optval.Ptr) = tv64;
      break;
    }
    // Each optname as a reminder which setting has been manually checked
    case SO_DEBUG:
    case SO_REUSEADDR:
    case SO_TYPE:
    case SO_ERROR:
    case SO_DONTROUTE:
    case SO_BROADCAST:
    case SO_SNDBUF:
    case SO_RCVBUF:
    case SO_SNDBUFFORCE:
    case SO_RCVBUFFORCE:
    case SO_KEEPALIVE:
    case SO_OOBINLINE:
    case SO_NO_CHECK:
    case SO_PRIORITY:
    case SO_LINGER:
    case SO_BSDCOMPAT:
    case SO_REUSEPORT:
    /**
     * @name These end up differing between {x86,arm} and {powerpc, alpha, sparc, mips, parisc}
     * @{ */
    case SO_PASSCRED:
    case SO_PEERCRED:
    case SO_RCVLOWAT:
    case SO_SNDLOWAT:
    /**  @} */
    case SO_SECURITY_AUTHENTICATION:
    case SO_SECURITY_ENCRYPTION_TRANSPORT:
    case SO_SECURITY_ENCRYPTION_NETWORK:
    case SO_ATTACH_FILTER: // Renamed to SO_GET_FILTER on get. Same between 32-bit and 64-bit
    case SO_DETACH_FILTER:
    case SO_PEERNAME:
    case SO_TIMESTAMP_OLD: // Returns int32_t boolean
    case SO_ACCEPTCONN:
    case SO_PEERSEC:
    // Gap 32, 33
    case SO_PASSSEC:
    case SO_TIMESTAMPNS_OLD: // Returns int32_t boolean
    case SO_MARK:
    case SO_TIMESTAMPING_OLD: // Returns so_timestamping
    case SO_PROTOCOL:
    case SO_DOMAIN:
    case SO_RXQ_OVFL:
    case SO_WIFI_STATUS:
    case SO_PEEK_OFF:
    case SO_NOFCS:
    case SO_LOCK_FILTER:
    case SO_SELECT_ERR_QUEUE:
    case SO_BUSY_POLL:
    case SO_MAX_PACING_RATE:
    case SO_BPF_EXTENSIONS:
    case SO_INCOMING_CPU:
    case SO_ATTACH_BPF:
    case SO_ATTACH_REUSEPORT_CBPF: // Doesn't do anything in get
    case SO_ATTACH_REUSEPORT_EBPF:
    case SO_CNX_ADVICE:
    // Gap 54 (SCM_TIMESTAMPING_OPT_STATS)
    case SO_MEMINFO:
    case SO_INCOMING_NAPI_ID:
    case SO_COOKIE: // Cookie always returns 64-bit even on 32-bit
    // Gap 58 (SCM_TIMESTAMPING_PKTINFO)
    case SO_PEERGROUPS:
    case SO_ZEROCOPY:
    case SO_TXTIME:
    case SO_BINDTOIFINDEX:
    case SO_TIMESTAMP_NEW:
    case SO_TIMESTAMPNS_NEW:
    case SO_TIMESTAMPING_NEW:
    case SO_RCVTIMEO_NEW:
    case SO_SNDTIMEO_NEW:
    case SO_DETACH_REUSEPORT_BPF:
    case SO_PREFER_BUSY_POLL:
    case SO_BUSY_POLL_BUDGET:
    case SO_NETNS_COOKIE: // Cookie always returns 64-bit even on 32-bit
    case SO_BUF_LOCK:
    case SO_RESERVE_MEM:
    default: Result = ::syscall(SYSCALL_DEF(getsockopt), sockfd, level, optname, optval, optlen); break;
    }
  } else {
    Result = ::syscall(SYSCALL_DEF(getsockopt), sockfd, level, optname, optval, optlen);
  }
  SYSCALL_ERRNO();
}

void RegisterSocket(FEX::HLE::SyscallHandler* Handler) {
  REGISTER_SYSCALL_IMPL_X32(socketcall, [](FEXCore::Core::CpuStateFrame* Frame, uint32_t call, uint32_t* Arguments) -> uint64_t {
    uint64_t Result {};

    switch (call) {
    case OP_SOCKET: {
      Result = ::socket(Arguments[0], Arguments[1], Arguments[2]);
      break;
    }
    case OP_BIND: {
      Result = ::bind(Arguments[0], reinterpret_cast<const struct sockaddr*>(Arguments[1]), Arguments[2]);
      break;
    }
    case OP_CONNECT: {
      Result = ::connect(Arguments[0], reinterpret_cast<const struct sockaddr*>(Arguments[1]), Arguments[2]);
      break;
    }
    case OP_LISTEN: {
      Result = ::listen(Arguments[0], Arguments[1]);
      break;
    }
    case OP_ACCEPT: {
      Result = ::accept(Arguments[0], reinterpret_cast<struct sockaddr*>(Arguments[1]), reinterpret_cast<socklen_t*>(Arguments[2]));
      break;
    }
    case OP_GETSOCKNAME: {
      Result = ::getsockname(Arguments[0], reinterpret_cast<struct sockaddr*>(Arguments[1]), reinterpret_cast<socklen_t*>(Arguments[2]));
      break;
    }
    case OP_GETPEERNAME: {
      Result = ::getpeername(Arguments[0], reinterpret_cast<struct sockaddr*>(Arguments[1]), reinterpret_cast<socklen_t*>(Arguments[2]));
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
      Result = ::sendto(Arguments[0], reinterpret_cast<const void*>(Arguments[1]), Arguments[2], Arguments[3],
                        reinterpret_cast<struct sockaddr*>(Arguments[4]), reinterpret_cast<socklen_t>(Arguments[5]));
      break;
    }
    case OP_RECVFROM: {
      Result = ::recvfrom(Arguments[0], reinterpret_cast<void*>(Arguments[1]), Arguments[2], Arguments[3],
                          reinterpret_cast<struct sockaddr*>(Arguments[4]), reinterpret_cast<socklen_t*>(Arguments[5]));
      break;
    }
    case OP_SHUTDOWN: {
      Result = ::shutdown(Arguments[0], Arguments[1]);
      break;
    }
    case OP_SETSOCKOPT: {
      return SetSockOpt(Arguments[0], Arguments[1], Arguments[2], Arguments[3], reinterpret_cast<socklen_t>(Arguments[4]));
      break;
    }
    case OP_GETSOCKOPT: {
      return GetSockOpt(Arguments[0], Arguments[1], Arguments[2], reinterpret_cast<void*>(Arguments[3]),
                        reinterpret_cast<socklen_t*>(Arguments[4]));
      break;
    }
    case OP_SENDMSG: {
      return SendMsg(Arguments[0], reinterpret_cast<const struct msghdr32*>(Arguments[1]), Arguments[2]);
      break;
    }
    case OP_RECVMSG: {
      return RecvMsg(Arguments[0], reinterpret_cast<struct msghdr32*>(Arguments[1]), Arguments[2]);
      break;
    }
    case OP_ACCEPT4: {
      return ::accept4(Arguments[0], reinterpret_cast<struct sockaddr*>(Arguments[1]), reinterpret_cast<socklen_t*>(Arguments[2]), Arguments[3]);
      break;
    }
    case OP_RECVMMSG: {
      timespec32* timeout_ts = reinterpret_cast<timespec32*>(Arguments[4]);
      struct timespec tp64 {};
      struct timespec* timed_ptr {};
      if (timeout_ts) {
        tp64 = *timeout_ts;
        timed_ptr = &tp64;
      }

      uint64_t Result = RecvMMsg(Arguments[0], Arguments[1], Arguments[2], Arguments[3], timed_ptr);

      if (timeout_ts) {
        *timeout_ts = tp64;
      }

      return Result;
      break;
    }
    case OP_SENDMMSG: {
      return SendMMsg(Arguments[0], reinterpret_cast<mmsghdr_32*>(Arguments[1]), Arguments[2], Arguments[3]);
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unsupported socketcall op: {}", call); break;
    }
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(sendmsg, [](FEXCore::Core::CpuStateFrame* Frame, int sockfd, const struct msghdr32* msg, int flags) -> uint64_t {
    return SendMsg(sockfd, msg, flags);
  });

  REGISTER_SYSCALL_IMPL_X32(sendmmsg,
                            [](FEXCore::Core::CpuStateFrame* Frame, int sockfd, auto_compat_ptr<mmsghdr_32> msgvec, uint32_t vlen,
                               int flags) -> uint64_t { return SendMMsg(sockfd, msgvec, vlen, flags); });

  REGISTER_SYSCALL_IMPL_X32(recvmmsg,
                            [](FEXCore::Core::CpuStateFrame* Frame, int sockfd, auto_compat_ptr<mmsghdr_32> msgvec, uint32_t vlen,
                               int flags, timespec32* timeout_ts) -> uint64_t {
                              struct timespec tp64 {};
                              struct timespec* timed_ptr {};
                              if (timeout_ts) {
                                tp64 = *timeout_ts;
                                timed_ptr = &tp64;
                              }

                              uint64_t Result = RecvMMsg(sockfd, msgvec, vlen, flags, timed_ptr);

                              if (timeout_ts) {
                                *timeout_ts = tp64;
                              }

                              return Result;
                            });

  REGISTER_SYSCALL_IMPL_X32(recvmmsg_time64,
                            [](FEXCore::Core::CpuStateFrame* Frame, int sockfd, auto_compat_ptr<mmsghdr_32> msgvec, uint32_t vlen, int flags,
                               struct timespec* timeout_ts) -> uint64_t { return RecvMMsg(sockfd, msgvec, vlen, flags, timeout_ts); });

  REGISTER_SYSCALL_IMPL_X32(recvmsg, [](FEXCore::Core::CpuStateFrame* Frame, int sockfd, struct msghdr32* msg, int flags) -> uint64_t {
    return RecvMsg(sockfd, msg, flags);
  });

  REGISTER_SYSCALL_IMPL_X32(setsockopt,
                            [](FEXCore::Core::CpuStateFrame* Frame, int sockfd, int level, int optname, auto_compat_ptr<void> optval,
                               socklen_t optlen) -> uint64_t { return SetSockOpt(sockfd, level, optname, optval, optlen); });

  REGISTER_SYSCALL_IMPL_X32(getsockopt,
                            [](FEXCore::Core::CpuStateFrame* Frame, int sockfd, int level, int optname, auto_compat_ptr<void> optval,
                               auto_compat_ptr<socklen_t> optlen) -> uint64_t { return GetSockOpt(sockfd, level, optname, optval, optlen); });
}
} // namespace FEX::HLE::x32
