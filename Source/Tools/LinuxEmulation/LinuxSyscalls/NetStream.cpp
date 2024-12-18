// SPDX-License-Identifier: MIT
#include "LinuxSyscalls/NetStream.h"

#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/LogManager.h>

#include <cstring>
#include <sys/socket.h>
#include <poll.h>
#include <unistd.h>

namespace FEX::Utils {
std::optional<NetStream::ReturnGet> NetStream::get(std::optional<std::chrono::seconds> timeout) {
  if (read_offset != receive_buffer.size() && read_offset != receive_offset) {
    auto Result = receive_buffer.at(read_offset);
    ++read_offset;
    return ReturnGet {Result, false};
  }

  if (read_offset == receive_buffer.size()) {
    read_offset = 0;
    receive_offset = 0;
  }

  struct pollfd pfd {
    .fd = socketfd,
    .events = POLLIN,
    .revents = 0,
  };

  timespec tmo {
    .tv_sec = timeout.has_value() ? timeout->count() : 0,
    .tv_nsec = 0,
  };

  timespec *tmo_p  = timeout.has_value() ? &tmo : nullptr;

  while (true) {
    auto Result = ppoll(&pfd, 1, tmo_p, nullptr);
    if (Result > 0) {
      if (pfd.revents & POLLHUP) {
        return ReturnGet {'\0', true};
      }

      const auto remaining_size = receive_buffer.size() - receive_offset;
      Result = ::recv(socketfd, &receive_buffer.at(receive_offset), remaining_size, 0);
      if (Result > 0) {
        receive_offset += Result;
        auto Result = receive_buffer.at(read_offset);
        ++read_offset;
        return ReturnGet {Result, false};
      }
      else {
        return std::nullopt;
      }
    } else {
      return std::nullopt;
    }
  }
}

size_t NetStream::read(char *buf, size_t size) {
  size_t Read{};
  while (Read < size) {
    auto Result = get();
    if (Result.has_value() && !Result->Hangup) {
      buf[Read] = Result->data;
      ++Read;
    }
    else {
      return Read;
    }
  }
  return Read;
}

bool NetStream::SendPacket(fextl::string const& packet) {
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
