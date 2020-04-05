#include "Common/ArgumentLoader.h"
#include "CommonCore/VMFactory.h"
#include "Common/Config.h"
#include "ELFLoader.h"
#include "HarnessHelpers.h"
#include "LogManager.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CodeLoader.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Memory/SharedMem.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

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
  case LogMan::STDOUT:
    CharLevel = "STDOUT";
    break;
  case LogMan::STDERR:
    CharLevel = "STDERR";
    break;
  default:
    CharLevel = "???";
    break;
  }

  printf("[%s] %s\n", CharLevel, Message);
  fflush(nullptr);
}

void AssertHandler(char const *Message) {
  printf("[ASSERT] %s\n", Message);
  fflush(nullptr);
}

int main(int argc, char **argv, char **const envp) {
  LogMan::Throw::InstallHandler(AssertHandler);
  LogMan::Msg::InstallHandler(MsgHandler);
  FEX::Config::Init();
  FEX::ArgLoader::Load(argc, argv);

  FEX::Config::Value<uint8_t> CoreConfig{"Core", 0};
  FEX::Config::Value<uint64_t> BlockSizeConfig{"MaxInst", 1};
  FEX::Config::Value<bool> SingleStepConfig{"SingleStep", false};
  FEX::Config::Value<bool> MultiblockConfig{"Multiblock", false};
  FEX::Config::Value<bool> GdbServerConfig{"GdbServer", false};
  FEX::Config::Value<bool> AccurateSTDConfig{"AccurateSTDOut", false};
  FEX::Config::Value<bool> UnifiedMemory{"UnifiedMemory", false};
  FEX::Config::Value<std::string> LDPath{"RootFS", ""};

  auto Args = FEX::ArgLoader::Get();
  auto ParsedArgs = FEX::ArgLoader::GetParsedArgs();

  LogMan::Throw::A(!Args.empty(), "Not enough arguments");

  FEX::HarnessHelper::ELFCodeLoader Loader{Args[0], LDPath(), Args, ParsedArgs, envp};

  FEXCore::Context::InitializeStaticTables();
  auto SHM = FEXCore::SHM::AllocateSHMRegion(1ULL << 32);
  auto CTX = FEXCore::Context::CreateNewContext();
  FEXCore::Context::InitializeContext(CTX);
  FEXCore::Context::SetApplicationFile(CTX, std::filesystem::canonical(Args[0]));

  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_DEFAULTCORE, CoreConfig() > 3 ? FEXCore::Config::CONFIG_CUSTOM : CoreConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_MULTIBLOCK, MultiblockConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_ACCURATESTDOUT, AccurateSTDConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_SINGLESTEP, SingleStepConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_MAXBLOCKINST, BlockSizeConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_GDBSERVER, GdbServerConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_ROOTFSPATH, LDPath());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_UNIFIED_MEMORY, UnifiedMemory());
  FEXCore::Context::SetCustomCPUBackendFactory(CTX, VMFactory::CPUCreationFactory);
  // FEXCore::Context::SetFallbackCPUBackendFactory(CTX, VMFactory::CPUCreationFactoryFallback);

  FEXCore::Context::AddGuestMemoryRegion(CTX, SHM);
  FEXCore::Context::InitCore(CTX, &Loader);

  FEXCore::Context::ExitReason ShutdownReason = FEXCore::Context::ExitReason::EXIT_SHUTDOWN;


  // There might already be an exit handler, leave it installed
  if(!FEXCore::Context::GetExitHandler(CTX)) {
    FEXCore::Context::SetExitHandler(CTX, [&](uint64_t thread, FEXCore::Context::ExitReason reason) {
      if (reason != FEXCore::Context::ExitReason::EXIT_DEBUG) {
        ShutdownReason = reason;
        FEXCore::Context::Stop(CTX);
      }
    });
  }

  FEXCore::Context::RunUntilExit(CTX);

  LogMan::Msg::D("Reason we left VM: %d", ShutdownReason);
  bool Result = ShutdownReason == 0;

  FEXCore::Context::DestroyContext(CTX);
  FEXCore::SHM::DestroyRegion(SHM);

  printf("Managed to load? %s\n", Result ? "Yes" : "No");

  FEX::Config::Shutdown();
  return 0;
}
