/*
$info$
tags: glue|gdbserver
$end_info$
*/
#pragma once

#include <FEXCore/Config/Config.h>
#include <FEXCore/Utils/Threads.h>

#include <istream>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <string>

namespace FEXCore {

namespace Context {
  struct Context;
}

class GdbServer {
public:
    GdbServer(FEXCore::Context::Context *ctx);

    // Public for threading
    void GdbServerLoop();

private:
    void Break(int signal);

    std::unique_ptr<std::iostream> OpenSocket();
    void StartThread();
    std::string ReadPacket(std::iostream &stream);
    void SendPacket(std::ostream &stream, const std::string& packet);

    void SendACK(std::ostream &stream, bool NACK);

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

    std::string readRegs();
    HandledPacketType readReg(const std::string& packet);

    FEXCore::Context::Context *CTX;
    std::unique_ptr<FEXCore::Threads::Thread> gdbServerThread;
    std::unique_ptr<std::iostream> CommsStream;
    std::mutex sendMutex;
    bool SettingNoAckMode{false};
    bool NoAckMode{false};
    std::string ThreadString{};
    uint32_t CurrentDebuggingThread{};
    FEX_CONFIG_OPT(Filename, APP_FILENAME);
};

}



