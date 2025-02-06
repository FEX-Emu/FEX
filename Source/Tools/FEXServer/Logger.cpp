// SPDX-License-Identifier: MIT
#include <Common/Async.h>
#include <Common/FEXServerClient.h>

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
    } else if (Read == 0) {
      // Socket closed
      return;
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
  fasio::poll_reactor Reactor;

  auto Pipe = fasio::posix_descriptor {Reactor, LogClientQueuePipe[0]};
  fextl::vector<fasio::posix_descriptor> Clients;

  // Wait for AppendLogFD to send file descriptors over LogClientQueuePipe.
  // When data becomes ready, we read the FD and register it to the reactor.
  Pipe.async_wait([&](fasio::error ec) {
    if (ec != fasio::error::success) {
      return fasio::post_callback::stop_reactor;
    }

    int ReceivedFD;
    read(Pipe.FD, &ReceivedFD, sizeof(ReceivedFD));

    // Register client and set up read callback
    Clients.emplace_back(Reactor, ReceivedFD);
    Clients.back().async_wait([&Clients, ReceivedFD](fasio::error ec) {
      if (ec != fasio::error::success) {
        std::iter_swap(std::find_if(Clients.begin(), Clients.end(), [=](auto& desc) { return desc.FD == ReceivedFD; }), std::prev(Clients.end()));
        Clients.pop_back();
        return fasio::post_callback::drop;
      }

      HandleLogData(ReceivedFD);
      return fasio::post_callback::repeat;
    });

    return fasio::post_callback::repeat;
  });

  Reactor.run();
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
