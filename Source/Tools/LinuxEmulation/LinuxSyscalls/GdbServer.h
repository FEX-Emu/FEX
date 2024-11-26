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

#include "LinuxSyscalls/SignalDelegator.h"

namespace FEX {

class GdbServer {
public:
  GdbServer(FEXCore::Context::Context* ctx, FEX::HLE::SignalDelegator* SignalDelegation, FEX::HLE::SyscallHandler* const SyscallHandler);
  ~GdbServer();

  // Public for threading
  void GdbServerLoop();

  void AlertLibrariesChanged() {
    LibraryMapChanged = true;
  }

private:
  void Break(FEXCore::Core::InternalThreadState* Thread, int signal);

  void OpenListenSocket();
  void CloseListenSocket();
  enum class WaitForConnectionResult {
    CONNECTION,
    ERROR,
  };
  WaitForConnectionResult WaitForConnection();
  fextl::unique_ptr<std::iostream> OpenSocket();
  void StartThread();
  fextl::string ReadPacket(std::iostream& stream);
  void SendPacket(std::ostream& stream, const fextl::string& packet);

  void SendACK(std::ostream& stream, bool NACK);

  Event ThreadBreakEvent {};
  void WaitForThreadWakeup();

  struct HandledPacketType {
    fextl::string Response {};
    enum ResponseType {
      TYPE_NONE,
      TYPE_UNKNOWN,
      TYPE_ACK,
      TYPE_NACK,
      TYPE_ONLYACK,
      TYPE_ONLYNACK,
    };
    ResponseType TypeResponse {};
  };

  void SendPacketPair(const HandledPacketType& packetPair);
  HandledPacketType ProcessPacket(const fextl::string& packet);
  HandledPacketType handleProgramOffsets();

  HandledPacketType ThreadAction(char action, uint32_t tid);

  // Binary data transfer handlers
  // XFer function to correctly encode any reply
  static fextl::string EncodeXferString(const fextl::string& data, int offset, int length) {
    if (offset == data.size()) {
      return "l";
    }
    if (offset >= data.size()) {
      return "E34"; // ERANGE
    }
    if ((data.size() - offset) > length) {
      return "m" + data.substr(offset, length);
    }
    return "l" + data.substr(offset);
  };

  HandledPacketType XferCommandExecFile(const fextl::string& annex, int offset, int length);
  HandledPacketType XferCommandFeatures(const fextl::string& annex, int offset, int length);
  HandledPacketType XferCommandThreads(const fextl::string& annex, int offset, int length);
  HandledPacketType XferCommandOSData(const fextl::string& annex, int offset, int length);
  HandledPacketType XferCommandLibraries(const fextl::string& annex, int offset, int length);
  HandledPacketType XferCommandAuxv(const fextl::string& annex, int offset, int length);
  HandledPacketType handleXfer(const fextl::string& packet);

  // Command handlers
  HandledPacketType CommandEnableExtendedMode(const fextl::string& packet);
  HandledPacketType CommandQueryHalted(const fextl::string& packet);
  HandledPacketType CommandContinue(const fextl::string& packet);
  HandledPacketType CommandDetach(const fextl::string& packet);
  HandledPacketType CommandReadRegisters(const fextl::string& packet);
  HandledPacketType CommandThreadOp(const fextl::string& packet);
  HandledPacketType CommandKill(const fextl::string& packet);
  HandledPacketType CommandMemory(const fextl::string& packet);
  HandledPacketType CommandReadReg(const fextl::string& packet);
  HandledPacketType CommandQuery(const fextl::string& packet);
  HandledPacketType CommandSingleStep(const fextl::string& packet);
  HandledPacketType CommandQueryThreadAlive(const fextl::string& packet);
  HandledPacketType CommandMultiLetterV(const fextl::string& packet);
  HandledPacketType CommandBreakpoint(const fextl::string& packet);
  HandledPacketType CommandUnknown(const fextl::string& packet);

  /**
   * @brief Returns the ThreadStateObject for the matching TID, or parent thread if TID isn't found
   *
   * @param TID Which TID to search for
   */
  const FEX::HLE::ThreadStateObject* FindThreadByTID(uint32_t TID);

  struct X80Float {
    uint8_t Data[10];
  };

  struct FEX_PACKED GDBContextDefinition {
    uint64_t gregs[FEXCore::Core::CPUState::NUM_GPRS];
    uint64_t rip;
    uint32_t eflags;
    uint32_t cs, ss, ds, es, fs, gs;
    X80Float mm[FEXCore::Core::CPUState::NUM_MMS];
    uint32_t fctrl;
    uint32_t fstat;
    uint32_t dummies[6];
    uint64_t xmm[FEXCore::Core::CPUState::NUM_XMMS][4];
    uint32_t mxcsr;
  };

  GDBContextDefinition GenerateContextDefinition(const FEX::HLE::ThreadStateObject* ThreadObject);

  FEXCore::Context::Context* CTX;
  FEX::HLE::SyscallHandler* const SyscallHandler;
  fextl::unique_ptr<FEXCore::Threads::Thread> gdbServerThread;
  fextl::unique_ptr<std::iostream> CommsStream;
  std::mutex sendMutex;
  bool SettingNoAckMode {false};
  bool NoAckMode {false};
  bool NonStopMode {false};
  fextl::string ThreadString {};
  fextl::string OSDataString {};
  void buildLibraryMap();
  std::atomic<bool> LibraryMapChanged = true;
  std::atomic<bool> CoreShuttingDown {};
  fextl::string LibraryMapString {};

  // Used to keep track of which signals to pass to the guest
  std::array<bool, FEX::HLE::SignalDelegator::MAX_SIGNALS + 1> PassSignals {};
  uint32_t CurrentDebuggingThread {};
  int ListenSocket {};
  fextl::string GdbUnixSocketPath {};
  FEX_CONFIG_OPT(Filename, APP_FILENAME);
  FEX_CONFIG_OPT(Is64BitMode, IS64BIT_MODE);
};

} // namespace FEX
