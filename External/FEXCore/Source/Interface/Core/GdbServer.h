/*
$info$
tags: glue|gdbserver
$end_info$
*/
#pragma once

#include <FEXCore/Config/Config.h>
#include <FEXCore/Utils/Event.h>
#include <FEXCore/Utils/Threads.h>

#include <atomic>
#include <istream>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <string>

namespace FEXCore {

namespace Context {
  class ContextImpl;
}

class GdbServer {
public:
    GdbServer(FEXCore::Context::ContextImpl *ctx);

    // Public for threading
    void GdbServerLoop();

    void AlertLibrariesChanged() {
      LibraryMapChanged = true;
    }

private:
    void Break(int signal);

    void OpenListenSocket();
    std::unique_ptr<std::iostream> OpenSocket();
    void StartThread();
    std::string ReadPacket(std::iostream &stream);
    void SendPacket(std::ostream &stream, const std::string& packet);

    void SendACK(std::ostream &stream, bool NACK);

    Event ThreadBreakEvent{};
    void WaitForThreadWakeup();

    struct HandledPacketType {
      std::string Response{};
      enum ResponseType {
        TYPE_NONE,
        TYPE_UNKNOWN,
        TYPE_ACK,
        TYPE_NACK,
        TYPE_ONLYACK,
        TYPE_ONLYNACK,
      };
      ResponseType TypeResponse{};
    };

    void SendPacketPair(const HandledPacketType& packetPair);
    HandledPacketType ProcessPacket(const std::string &packet);
    HandledPacketType handleQuery(const std::string &packet);
    HandledPacketType handleXfer(const std::string &packet);
    HandledPacketType handleMemory(const std::string &packet);
    HandledPacketType handleV(const std::string& packet);
    HandledPacketType handleThreadOp(const std::string &packet);
    HandledPacketType handleBreakpoint(const std::string &packet);
    HandledPacketType handleProgramOffsets();

    HandledPacketType ThreadAction(char action, uint32_t tid);

    std::string readRegs();
    HandledPacketType readReg(const std::string& packet);

    FEXCore::Context::ContextImpl *CTX;
    std::unique_ptr<FEXCore::Threads::Thread> gdbServerThread;
    std::unique_ptr<std::iostream> CommsStream;
    std::mutex sendMutex;
    bool SettingNoAckMode{false};
    bool NoAckMode{false};
    bool NonStopMode{false};
    std::string ThreadString{};
    std::string OSDataString{};
    void buildLibraryMap();
    std::atomic<bool> LibraryMapChanged = true;
    std::string LibraryMapString{};

    // Used to keep track of which signals to pass to the guest
    std::array<bool, SignalDelegator::MAX_SIGNALS + 1> PassSignals{};
    uint32_t CurrentDebuggingThread{};
    int ListenSocket{};
    FEX_CONFIG_OPT(Filename, APP_FILENAME);
};

}



