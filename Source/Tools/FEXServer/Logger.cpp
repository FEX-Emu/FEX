// SPDX-License-Identifier: MIT
#include "Common/FEXServerClient.h"

#include <atomic>
#include <mutex>
#include <poll.h>
#include <thread>
#include <vector>

namespace Logging {
void ClientMsgHandler(int FD, FEXServerClient::Logging::PacketMsg* const Msg, const char* MsgStr);
}

namespace Logger {
int LogClientQueuePipe[2];
std::thread LogThread;

void HandleLogData(int Socket) {
  std::vector<uint8_t> Data(1500);
  size_t CurrentRead {};
  while (true) {
    int Read = read(Socket, &Data.at(CurrentRead), Data.size() - CurrentRead);
    if (Read > 0) {
      CurrentRead += Read;
      if (CurrentRead == Data.size()) {
        Data.resize(Data.size() << 1);
      } else {
        // No more to read
        break;
      }
    } else {
      if (errno == EWOULDBLOCK) {
        // no error
      } else {
        perror("read");
      }
      break;
    }
  }

  size_t CurrentOffset {};
  while (CurrentOffset < CurrentRead) {
    FEXServerClient::Logging::PacketHeader* Header = reinterpret_cast<FEXServerClient::Logging::PacketHeader*>(&Data[CurrentOffset]);
    if (Header->PacketType == FEXServerClient::Logging::PacketTypes::TYPE_MSG) {
      FEXServerClient::Logging::PacketMsg* Msg = reinterpret_cast<FEXServerClient::Logging::PacketMsg*>(&Data[CurrentOffset]);
      const char* MsgText = reinterpret_cast<const char*>(&Data[CurrentOffset + sizeof(FEXServerClient::Logging::PacketMsg)]);
      Logging::ClientMsgHandler(Socket, Msg, MsgText);

      CurrentOffset += sizeof(FEXServerClient::Logging::PacketMsg) + Msg->MessageLength;
    } else {
      CurrentOffset = CurrentRead;
    }
  }
}

void LogThreadFunc() {
  std::vector<pollfd> PollFDs;
  PollFDs.push_back(pollfd {.fd = LogClientQueuePipe[0], .events = POLLIN | POLLHUP, .revents = 0});

  while (true) {
    int Result = ppoll(PollFDs.data(), PollFDs.size(), nullptr, nullptr);
    if (Result <= 0) {
      continue;
    }

    // Process events for client pipe
    if (PollFDs[0].revents) {
      --Result;

      auto [PipeFD, _, revents] = PollFDs[0];
      PollFDs[0].revents = 0;

      if (revents & POLLIN) {
        int ReceivedFD;
        read(PipeFD, &ReceivedFD, sizeof(ReceivedFD));
        PollFDs.push_back(pollfd {
          .fd = ReceivedFD,
          .events = POLLIN,
          .revents = 0,
        });
      }

      if (revents & POLLHUP) {
        close(PipeFD);
        return;
      }
    }

    // Process events for log FDs
    for (auto it = PollFDs.begin() + 1; Result && it != PollFDs.end();) {
      if (it->revents == 0) {
        ++it;
        continue;
      }
      --Result;

      bool Erase {};
      if (it->revents & POLLIN) {
        // Data from the socket
        HandleLogData(it->fd);
      } else if (it->revents & (POLLHUP | POLLERR | POLLNVAL | POLLRDHUP)) {
        // Error or hangup, close the socket and erase it from our list
        Erase = true;
        close(it->fd);
      }

      it->revents = 0;

      if (Erase) {
        it = PollFDs.erase(it);
      } else {
        ++it;
      }
    }
  }
}

void StartLogThread() {
  pipe2(LogClientQueuePipe, 0);

  LogThread = std::thread(LogThreadFunc);
}

void AppendLogFD(int FD) {
  write(LogClientQueuePipe[1], &FD, sizeof(FD));
}

bool LogThreadRunning() {
  return LogThread.joinable();
}

void Shutdown() {
  close(LogClientQueuePipe[1]);

  if (LogThread.joinable()) {
    LogThread.join();
  }
}
} // namespace Logger
