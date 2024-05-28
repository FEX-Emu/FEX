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
std::vector<struct pollfd> PollFDs {};
std::mutex IncomingPollFDsLock {};
std::vector<struct pollfd> IncomingPollFDs {};
std::thread LogThread;
std::atomic<bool> ShouldShutdown {false};
std::atomic<int32_t> LoggerThreadTID {};

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
  LoggerThreadTID = FHU::Syscalls::gettid();

  while (!ShouldShutdown) {
    struct timespec ts {};
    ts.tv_sec = 5;

    {
      std::unique_lock lk {IncomingPollFDsLock};
      PollFDs.insert(PollFDs.end(), std::make_move_iterator(IncomingPollFDs.begin()), std::make_move_iterator(IncomingPollFDs.end()));
      IncomingPollFDs.clear();
    }
    if (PollFDs.size() == 0) {
      pselect(0, nullptr, nullptr, nullptr, &ts, nullptr);
    } else {
      int Result = ppoll(&PollFDs.at(0), PollFDs.size(), &ts, nullptr);
      if (Result > 0) {
        // Walk the FDs and see if we got any results
        for (auto it = PollFDs.begin(); it != PollFDs.end();) {
          bool Erase {};
          if (it->revents != 0) {
            if (it->revents & POLLIN) {
              // Data from the socket
              HandleLogData(it->fd);
            } else if (it->revents & (POLLHUP | POLLERR | POLLNVAL | POLLRDHUP)) {
              // Error or hangup, close the socket and erase it from our list
              Erase = true;
              close(it->fd);
            }

            it->revents = 0;
            --Result;
          }

          if (Erase) {
            it = PollFDs.erase(it);
          } else {
            ++it;
          }

          if (Result == 0) {
            // Early break if we've consumed all the results
            break;
          }
        }
      }
    }
  }
}

void StartLogThread() {
  LogThread = std::thread(LogThreadFunc);
}

void AppendLogFD(int FD) {
  {
    std::unique_lock lk {IncomingPollFDsLock};
    IncomingPollFDs.emplace_back(pollfd {
      .fd = FD,
      .events = POLLIN,
      .revents = 0,
    });
  }

  // Wake up the thread immediately
  FHU::Syscalls::tgkill(::getpid(), LoggerThreadTID, SIGUSR1);
}

bool LogThreadRunning() {
  return LogThread.joinable();
}

void Shutdown() {
  ShouldShutdown = true;

  // Wake up the thread immediately
  FHU::Syscalls::tgkill(::getpid(), LoggerThreadTID, SIGUSR1);

  if (LogThread.joinable()) {
    LogThread.join();
  }
}
} // namespace Logger
