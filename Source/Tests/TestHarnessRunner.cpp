#include "Common/ArgumentLoader.h"
#include "Common/EnvironmentLoader.h"
#include "CommonCore/VMFactory.h"
#include "HarnessHelpers.h"
#include "LogManager.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CodeLoader.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/Memory/SharedMem.h>

#include <cassert>
#include <cstdint>
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
  default:
    CharLevel = "???";
    break;
  }
  printf("[%s] %s\n", CharLevel, Message);
}

void AssertHandler(char const *Message) {
  printf("[ASSERT] %s\n", Message);
}

int main(int argc, char **argv, char **const envp) {
  LogMan::Throw::InstallHandler(AssertHandler);
  LogMan::Msg::InstallHandler(MsgHandler);
  FEX::Config::Init();
  FEX::EnvLoader::Load(envp);
  FEX::ArgLoader::Load(argc, argv);

  FEX::Config::Value<uint8_t> CoreConfig{"Core", 0};
  FEX::Config::Value<uint64_t> BlockSizeConfig{"MaxInst", 1};
  FEX::Config::Value<bool> SingleStepConfig{"SingleStep", false};
  FEX::Config::Value<bool> MultiblockConfig{"Multiblock", false};
  FEX::Config::Value<bool> SMCChecksConfig{"SMCChecks", false};

  auto Args = FEX::ArgLoader::Get();

  LogMan::Throw::A(Args.size() > 1, "Not enough arguments");

  FEX::HarnessHelper::HarnessCodeLoader Loader{Args[0], Args[1].c_str()};

  FEXCore::Context::InitializeStaticTables(Loader.Is64BitMode() ? FEXCore::Context::MODE_64BIT : FEXCore::Context::MODE_32BIT);
  auto SHM = FEXCore::SHM::AllocateSHMRegion(1ULL << 34);
  auto CTX = FEXCore::Context::CreateNewContext();

  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_UNIFIED_MEMORY, true);
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_DEFAULTCORE, CoreConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_MULTIBLOCK, MultiblockConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_SINGLESTEP, SingleStepConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_MAXBLOCKINST, BlockSizeConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_IS64BIT_MODE, Loader.Is64BitMode());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_SMC_CHECKS, SMCChecksConfig());
  FEXCore::Context::SetCustomCPUBackendFactory(CTX, HostCPUFactory::HostCPUCreationFactory);

  FEXCore::Context::AddGuestMemoryRegion(CTX, SHM);

  FEXCore::Context::InitializeContext(CTX);

  bool Result1 = FEXCore::Context::InitCore(CTX, &Loader);

  if (!Result1)
    return 1;

  FEXCore::Context::RunUntilExit(CTX);

  // Just re-use compare state. It also checks against the expected values in config.
  FEXCore::Core::CPUState State;
  FEXCore::Context::GetCPUState(CTX, &State);
  bool Passed = Loader.CompareStates(&State, nullptr);

  LogMan::Msg::I("Passed? %s\n", Passed ? "Yes" : "No");

  FEXCore::SHM::DestroyRegion(SHM);
  FEXCore::Context::DestroyContext(CTX);

  return Passed ? 0 : -1;
}

