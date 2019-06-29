#include "LogManager.h"
#include "SimpleCodeLoader.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Memory/SharedMem.h>
#include <cstdio>

void MsgHandler(LogMan::DebugLevels Level, const char *Message) {
  const char *CharLevel{nullptr};

  switch (Level) {
  case LogMan::NONE:
    CharLevel = "NONE";
    break;
  case LogMan::ASSERT:
    CharLevel = "ASSERT";
    break;
  case LogMan::ERROR:
    CharLevel = "ERROR";
    break;
  case LogMan::DEBUG:
    CharLevel = "DEBUG";
    break;
  case LogMan::INFO:
    CharLevel = "Info";
    break;
  default:
    CharLevel = "???";
    break;
  }
  printf("[%s] %s\n", CharLevel, Message);
}

void AssertHandler(const char *Message) {
  printf("[ASSERT] %s\n", Message);
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) {
  LogMan::Throw::InstallHandler(AssertHandler);
  LogMan::Msg::InstallHandler(MsgHandler);

  static constexpr uint8_t RawCode[] = {
    0x90, // NOP
    0xF4  // HLT
  };

  TestCode Test(RawCode, sizeof(RawCode));
  FEXCore::Context::InitializeStaticTables();
  auto SHM = FEXCore::SHM::AllocateSHMRegion(1ULL << 36);
  auto CTX = FEXCore::Context::CreateNewContext();

  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_MAXBLOCKINST, 1);
  FEXCore::Context::AddGuestMemoryRegion(CTX, SHM);
  FEXCore::Context::InitCore(CTX, &Test);

  auto ShutdownReason = FEXCore::Context::RunLoop(CTX, true);
  LogMan::Msg::D("Reason we left VM: %d", ShutdownReason);

  FEXCore::Context::DestroyContext(CTX);
  FEXCore::SHM::DestroyRegion(SHM);
  return 0;
}
