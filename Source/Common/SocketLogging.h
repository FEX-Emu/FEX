#pragma once
#include <FEXCore/Utils/Event.h>
#include <FEXCore/Utils/LogManager.h>

namespace FEX::SocketLogging {
  // Client side
  namespace Client {
    void MsgHandler(LogMan::DebugLevels Level, char const *Message);
    void AssertHandler(char const *Message);

    bool ConnectToClient(const std::string &Remote);
  }

  // Server side
  namespace Server {
    class ServerListener {
      public:
        using MsgHandlerType = std::function<void(int FD, uint64_t Timestamp, uint32_t PID, uint32_t TID, uint32_t Level, const char* Msg)>;
        using FDClosedHandlerType = std::function<void(int FD)>;

        void SetMsgHandler(MsgHandlerType Handler) {
          MsgHandler = std::move(Handler);
        }

        void SetFDClosedHandler(FDClosedHandlerType Handler) {
          ClosedHandler = std::move(Handler);
        }

        void WaitForShutdown() {
          ShutdownEvent.Wait();
        }

      protected:

        static void DefaultMsgHandler(int, uint64_t, uint32_t, uint32_t, uint32_t, const char*) {
        }

        static void DefaultFDClosedHandler(int) {
        }

        MsgHandlerType MsgHandler {DefaultMsgHandler};
        FDClosedHandlerType ClosedHandler {DefaultFDClosedHandler};
        Event ShutdownEvent{};
    };
    std::unique_ptr<ServerListener> StartListening(std::string_view Socket);
  }
}
