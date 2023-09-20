// SPDX-License-Identifier: MIT
#pragma once
#include <atomic>
#include <chrono>
#include <optional>
#include <poll.h>
#include <vector>
namespace FEXServer {
  class SocketConnectionHandler {
    public:
      void RunUntilShutdown(std::chrono::system_clock::duration ExecutionTimeout, time_t ppolltimeout);

      enum class FDEventResult {
        SUCCESS,
        ERASE,
      };

      void RequestShutdown() {
        ShouldShutdown = true;
      }

    protected:
      void AppendFDToTrack(int FD) {
        NewFDs.push_back(pollfd {
          .fd = FD,
          .events = POLLIN | POLLHUP | POLLERR | POLLNVAL | POLLREMOVE | POLLRDHUP,
          .revents = 0,
        });
      }

      virtual void OnShutdown() {}
      virtual void CheckShouldShutdownHandler(std::chrono::seconds TimeDuration) {
        if (TimeDuration >= std::chrono::duration_cast<std::chrono::seconds>(ExecutionTimeout)) {
          // If we have no data after a timeout
          // Then we can just go ahead and leave
          RequestShutdown();
        }
      }
      virtual FDEventResult HandleFDEvent(struct pollfd &Event) = 0;

      std::vector<struct pollfd> PollFDs{};
      std::chrono::system_clock::duration ExecutionTimeout;
    private:
      std::atomic<bool> ShouldShutdown;

      std::vector<struct pollfd> NewFDs{};
  };
}
