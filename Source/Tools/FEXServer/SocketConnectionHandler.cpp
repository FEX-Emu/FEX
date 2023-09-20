// SPDX-License-Identifier: MIT
#include "SocketConnectionHandler.h"

#include <unistd.h>

namespace FEXServer {
  void SocketConnectionHandler::RunUntilShutdown(std::chrono::system_clock::duration _ExecutionTimeout, time_t ppolltimeout) {
    ExecutionTimeout = _ExecutionTimeout;

    auto LastDataTime = std::chrono::system_clock::now();
    while (!ShouldShutdown) {
      struct timespec ts{};
      ts.tv_sec = ppolltimeout;
      int Result = ppoll(PollFDs.data(), PollFDs.size(), &ts, nullptr);
      if (Result > 0) {
        // Walk the FDs and see if we got any results
        for (auto it = PollFDs.begin(); it != PollFDs.end(); ) {
          auto &Event = *it;
          bool Erase{};

          if (Event.revents != 0) {
            auto FDResult = HandleFDEvent(Event);
            if (FDResult == FDEventResult::ERASE) {
              Erase = true;
              close(Event.fd);
            }

            Event.revents = 0;
            --Result;
          }

          if (Erase) {
            it = PollFDs.erase(it);
          }
          else {
            ++it;
          }

          if (Result == 0) {
            // Early break if we've consumed all the results
            break;
          }
        }

        // Insert the new FDs to poll
        PollFDs.insert(PollFDs.end(), std::make_move_iterator(NewFDs.begin()), std::make_move_iterator(NewFDs.end()));
        NewFDs.clear();

        LastDataTime = std::chrono::system_clock::now();
      }
      else {
        auto Now = std::chrono::system_clock::now();
        auto Diff = Now - LastDataTime;
        CheckShouldShutdownHandler(std::chrono::duration_cast<std::chrono::seconds>(Diff));
      }
    }

    OnShutdown();
  }
}
