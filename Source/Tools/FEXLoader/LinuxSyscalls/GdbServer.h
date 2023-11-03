// SPDX-License-Identifier: MIT
/*
$info$
tags: glue|gdbserver
$end_info$
*/
#pragma once

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/Utils/Event.h>
#include <FEXCore/Utils/Threads.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/string.h>

#include <atomic>
#include <istream>
#include <memory>
#include <mutex>
#include <stdint.h>

namespace FEXCore {

class GdbServer {
public:
    GdbServer(FEXCore::Context::Context *ctx, SignalDelegator *SignalDelegation, FEXCore::HLE::SyscallHandler *const SyscallHandler);

    // Public for threading
    void GdbServerLoop();

    void AlertLibrariesChanged() {
      LibraryMapChanged = true;
    }

private:
    void Break(int signal);

    void OpenListenSocket();
    fextl::unique_ptr<std::iostream> OpenSocket();
    void StartThread();
    fextl::string ReadPacket(std::iostream &stream);
    void SendPacket(std::ostream &stream, const fextl::string& packet);

    void SendACK(std::ostream &stream, bool NACK);

    Event ThreadBreakEvent{};
    void WaitForThreadWakeup();

    struct HandledPacketType {
      fextl::string Response{};
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
    HandledPacketType ProcessPacket(const fextl::string &packet);
    HandledPacketType handleQuery(const fextl::string &packet);
    HandledPacketType handleXfer(const fextl::string &packet);
    HandledPacketType handleMemory(const fextl::string &packet);
    HandledPacketType handleV(const fextl::string& packet);
    HandledPacketType handleThreadOp(const fextl::string &packet);
    HandledPacketType handleBreakpoint(const fextl::string &packet);
    HandledPacketType handleProgramOffsets();

    HandledPacketType ThreadAction(char action, uint32_t tid);

    fextl::string readRegs();
    HandledPacketType readReg(const fextl::string& packet);

    FEXCore::Context::Context *CTX;
    FEXCore::HLE::SyscallHandler *const SyscallHandler;
    fextl::unique_ptr<FEXCore::Threads::Thread> gdbServerThread;
    fextl::unique_ptr<std::iostream> CommsStream;
    std::mutex sendMutex;
    bool SettingNoAckMode{false};
    bool NoAckMode{false};
    bool NonStopMode{false};
    fextl::string ThreadString{};
    fextl::string OSDataString{};
    void buildLibraryMap();
    std::atomic<bool> LibraryMapChanged = true;
    std::atomic<bool> CoreShuttingDown{};
    fextl::string LibraryMapString{};

    // Used to keep track of which signals to pass to the guest
    std::array<bool, SignalDelegator::MAX_SIGNALS + 1> PassSignals{};
    uint32_t CurrentDebuggingThread{};
    int ListenSocket{};
    FEX_CONFIG_OPT(Filename, APP_FILENAME);
    FEX_CONFIG_OPT(Is64BitMode, IS64BIT_MODE);
};

}



