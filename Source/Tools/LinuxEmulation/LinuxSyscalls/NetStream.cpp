// SPDX-License-Identifier: MIT
#include "LinuxSyscalls/NetStream.h"

#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/LogManager.h>

#include <cstring>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <poll.h>
#include <unistd.h>

namespace FEX::Utils {

NetStream::NetStream()
  : receive_buffer(1500) {
  eventfd = ::eventfd(0, EFD_CLOEXEC);
}

NetStream::ReturnGet NetStream::get() {
  if (read_offset != receive_buffer.size() && read_offset != receive_offset) {
    auto Result = receive_buffer.at(read_offset);
    ++read_offset;
    return NetStream::ReturnGet {Result};
  }

  if (read_offset == receive_buffer.size()) {
    read_offset = 0;
    receive_offset = 0;
  }

  struct pollfd pfds[2] = {
    {
      .fd = socketfd,
      .events = POLLIN,
      .revents = 0,
    },

    {
      .fd = eventfd,
      .events = POLLIN,
      .revents = 0,
    },
  };

  auto Result = poll(pfds, sizeof(pfds), -1);
  if (Result > 0) {
    for (auto& pfd : pfds) {
      if (pfd.fd == eventfd && (pfd.revents & POLLIN)) {
        // Interrupted by eventfd.
        uint64_t data;
        ::read(pfd.fd, &data, sizeof(data));
        break;
      }

      if (pfd.revents & POLLHUP) {
        return NetStream::ReturnGet {true};
      }

      const auto remaining_size = receive_buffer.size() - receive_offset;
      Result = ::recv(socketfd, &receive_buffer.at(receive_offset), remaining_size, 0);
      if (Result > 0) {
        receive_offset += Result;
        auto Result = receive_buffer.at(read_offset);
        ++read_offset;
        return NetStream::ReturnGet {Result};
      }
    }
  }

  return NetStream::ReturnGet {false};
}

size_t NetStream::read(char* buf, size_t size, bool ContinueOnInterrupt) {
  size_t Read {};
  while (Read < size) {
    auto Result = get();
    if (Result.HasData()) {
      buf[Read] = Result.GetData();
      ++Read;
    } else if ((!Result.HasData() && !ContinueOnInterrupt) || Result.HasHangup()) {
      return Read;
    }
  }
  return Read;
}

void NetStream::InterruptReader() {
  uint64_t data {1};
  ::write(eventfd, &data, sizeof(data));
}

bool NetStream::SendPacket(const fextl::string& packet) {
  size_t Total {};
  while (Total < packet.size()) {
    size_t Remaining = packet.size() - Total;
    size_t sent = ::send(socketfd, &packet.at(Total), Remaining, MSG_NOSIGNAL);
    if (sent == -1) {
      return false;
    }
    Total += sent;
  }

  return Total == packet.size();
}

} // namespace FEX::Utils
