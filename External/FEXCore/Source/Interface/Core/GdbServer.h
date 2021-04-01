/*
$info$
tags: glue|gdbserver
$end_info$
*/
#pragma once

#include <mutex>
#include <thread>

#include "Interface/Context/Context.h"
#include "Common/NetStream.h"

#include <FEXCore/Utils/Threads.h>

#include <mutex>

namespace FEXCore {

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
    void SendPacket(std::ostream &stream, std::string packet);

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

    void SendPacketPair(HandledPacketType packetPair);
    HandledPacketType ProcessPacket(std::string &packet);
    HandledPacketType handleQuery(std::string &packet);
    HandledPacketType handleXfer(std::string &packet);
    HandledPacketType handleMemory(std::string &packet);
    HandledPacketType handleV(std::string& packet);
    HandledPacketType handleThreadOp(std::string &packet);
    HandledPacketType handleBreakpoint(std::string &packet);
    HandledPacketType handleProgramOffsets();

    std::string readRegs();
    HandledPacketType readReg(std::string& packet);

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



