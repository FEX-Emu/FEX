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
#include <memory>
#include <mutex>
#include <stdint.h>

#include "LinuxSyscalls/NetStream.h"
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

  void OnThreadCreated(FEX::HLE::ThreadStateObject* ThreadObject);

  void ClaimLibraryChange(FEX::HLE::ThreadStateObject* ThreadObject);
  void SetLibrariesChanged() {
    LibraryMapChanged = true;
  }

private:
  void Break(FEX::HLE::ThreadStateObject* ThreadObject, int signal);

  void OpenListenSocket();
  void CloseListenSocket();
  enum class WaitForConnectionResult {
    CONNECTION,
    ERROR,
  };
  WaitForConnectionResult WaitForConnection();
  fextl::unique_ptr<FEX::Utils::NetStream> OpenSocket();
  void StartThread();
  fextl::string ReadPacket();
  void SendPacket(const fextl::string& packet);

  void SendACK(bool NACK);

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
  HandledPacketType SingleThreadAction(char action, uint32_t tid, uint32_t Signal, uint64_t NewRIP);

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

  HandledPacketType HandlevFile(const fextl::string& packet);
  HandledPacketType HandlevCont(const fextl::string& packet);

  // Command handlers
  HandledPacketType CommandEnableExtendedMode(const fextl::string& packet);
  HandledPacketType CommandQueryHalted(const fextl::string& packet);
  HandledPacketType CommandContinue(const fextl::string& packet);
  HandledPacketType CommandContinueSignal(const fextl::string& packet);
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
  FEX::HLE::SignalDelegator* SignalDelegation;
  fextl::unique_ptr<FEXCore::Threads::Thread> gdbServerThread;
  fextl::unique_ptr<FEX::Utils::NetStream> CommsStream;
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
  FEX_CONFIG_OPT(GdbServerNoWait, GDBSERVERNOWAIT);

  bool CurrentStateRunning{};
  std::atomic_bool InSignaledState{};

  class PersonalFaultHandler final : public FEX::HLE::SignalDelegatorBase {
  public:
    void HandleSignal(FEX::HLE::ThreadStateObject* Thread, int Signal, void* Info, void* UContext) override;
  };
  PersonalFaultHandler BasicSignalHandler;
  FEX::HLE::ThreadStateObject FakeThreadObject;

  static inline fextl::vector<fextl::string> split(const fextl::string& Str, char deliminator) {
    fextl::vector<fextl::string> Elements;
    fextl::istringstream Input(Str);
    for (fextl::string line; std::getline(Input, line); Elements.emplace_back(line))
      ;
    return Elements;
  };

  std::mutex RunningResponseQueueMutex;

  struct ResponsePacketContainer {
    HandledPacketType Packet;
    std::function<void()> PrologueHandler;
  };
  fextl::list<ResponsePacketContainer> ResponseQueueWhileRunning;
  void QueuePacketWhileForWhileRunning(HandledPacketType& response, std::function<void()> PrologueHandler = nullptr) {
    std::lock_guard lk(RunningResponseQueueMutex);
    ResponseQueueWhileRunning.emplace_back(response, PrologueHandler);
  }
  std::optional<ResponsePacketContainer> GetResponsePacket() {
    std::lock_guard lk(RunningResponseQueueMutex);
    if (ResponseQueueWhileRunning.empty()) return std::nullopt;
    auto pkt = ResponseQueueWhileRunning.front();
    ResponseQueueWhileRunning.pop_front();
    return pkt;
  }
};

} // namespace FEX
