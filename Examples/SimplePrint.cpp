#include "LogManager.h"
#include "SimpleCodeLoader.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Memory/SharedMem.h>
#include <cstdio>

void MsgHandler(LogMan::DebugLevels Level, char const *Message) {
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
  case LogMan::STDERR:
    CharLevel = "STDERR";
    break;
  case LogMan::STDOUT:
    CharLevel = "STDOUT";
    break;
  default:
    CharLevel = "???";
    break;
  }
  printf("[%s] %s\n", CharLevel, Message);
}

void AssertHandler(char const *Message) {
  printf("[ASSERT] %s\n", Message);
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) {
  LogMan::Throw::InstallHandler(AssertHandler);
  LogMan::Msg::InstallHandler(MsgHandler);

  // Set up a syscall to do a syscall WRITE to stdout
  // Syscall handler catches writes to stdout/stderr and pumps it through LogManager
  static constexpr uint8_t RawCode[] = {
    0X48,
    0XC7,
    0XC0,
    0X01,
    0X00,
    0X00,
    0X00, // MOV RAX, 0x1

    0X48,
    0XC7,
    0XC7,
    0X01,
    0X00,
    0X00,
    0X00, // MOV RDI, 0x1

    0X48,
    0XC7,
    0XC6,
    0X1F,
    0X00,
    0X00,
    0X00, // MOV RSI, 0x1F

    0X48,
    0XC7,
    0XC2,
    0X01,
    0X00,
    0X00,
    0X00, // MOV RDX, 1

    0X0F,
    0X05, // SYSCALL

    0XF4, // HLT

    0X54,
    0X65,
    0X73,
    0X74,
    0X65,
    0X72,
    0X00, // 'Tester\0'
  };

  TestCode Test(RawCode, sizeof(RawCode));
  FEXCore::Context::InitializeStaticTables();
  auto SHM = FEXCore::SHM::AllocateSHMRegion(1ULL << 36);
  auto CTX = FEXCore::Context::CreateNewContext();

//  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_MAXBLOCKINST, 1);
  FEXCore::Context::AddGuestMemoryRegion(CTX, SHM);
  FEXCore::Context::InitCore(CTX, &Test);

  auto ShutdownReason = FEXCore::Context::RunLoop(CTX, true);
  LogMan::Msg::D("Reason we left VM: %d", ShutdownReason);

  FEXCore::Context::DestroyContext(CTX);
  FEXCore::SHM::DestroyRegion(SHM);
  return 0;
}
