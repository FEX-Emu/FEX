#include "Common/ArgumentLoader.h"
#include "Common/EnvironmentLoader.h"
#include "CommonCore/VMFactory.h"
#include "Common/Config.h"
#include "HarnessHelpers.h"
#include "IRLoader/Loader.h"
#include "LogManager.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CodeLoader.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Memory/SharedMem.h>

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
  fflush(stdout);
}

void AssertHandler(char const *Message) {
  printf("[ASSERT] %s\n", Message);
  fflush(stdout);
}


class IRCodeLoader final : public FEXCore::CodeLoader {
	public:
    IRCodeLoader(FEX::IRLoader::Loader *Loader)
      : IR {Loader} {
    }

    uint64_t StackSize() const override {
      return STACK_SIZE;
    }

    void SetMemoryBase(uint64_t Base, bool Unified) override {
      MemoryBase = Base;
    }

    uint64_t SetupStack([[maybe_unused]] void *HostPtr, uint64_t GuestPtr) const override {
      return GuestPtr + STACK_SIZE - 16;
    }

    uint64_t DefaultRIP() const override {
      return IR->GetEntryRIP();
    }

    void MapMemoryRegion(std::function<void*(uint64_t, uint64_t, bool, bool)> Mapper) override {
      // Map the memory regions the test file asks for
      IR->MapRegions(Mapper);
    }

    void LoadMemory(MemoryWriter Writer) override {
      IR->LoadMemory(MemoryBase, Writer);
    }

    uint64_t GetFinalRIP() override { return 0; }

    virtual void AddIR(IRHandler Handler) override {
      Handler(IR->GetEntryRIP(), IR);
    }

  private:
    FEX::IRLoader::Loader *IR;
    constexpr static uint64_t STACK_SIZE = 8 * 1024 * 1024;
    uint64_t MemoryBase = 0;
};

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
  FEX::Config::Value<bool> GdbServerConfig{"GdbServer", false};
  FEX::Config::Value<bool> UnifiedMemory{"UnifiedMemory", false};
  FEX::Config::Value<std::string> LDPath{"RootFS", ""};

  auto Args = FEX::ArgLoader::Get();
  auto ParsedArgs = FEX::ArgLoader::GetParsedArgs();

  LogMan::Throw::A(Args.size() > 1, "Not enough arguments");

  FEXCore::Context::InitializeStaticTables();
  auto SHM = FEXCore::SHM::AllocateSHMRegion(1ULL << 36);
  auto CTX = FEXCore::Context::CreateNewContext();
  FEXCore::Context::InitializeContext(CTX);

  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_DEFAULTCORE, CoreConfig() > 3 ? FEXCore::Config::CONFIG_CUSTOM : CoreConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_MULTIBLOCK, MultiblockConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_SINGLESTEP, SingleStepConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_MAXBLOCKINST, BlockSizeConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_GDBSERVER, GdbServerConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_ROOTFSPATH, LDPath());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_UNIFIED_MEMORY, UnifiedMemory());
  FEXCore::Context::SetCustomCPUBackendFactory(CTX, VMFactory::CPUCreationFactory);

  FEXCore::Context::AddGuestMemoryRegion(CTX, SHM);

  FEX::IRLoader::InitializeStaticTables();
	FEX::IRLoader::Loader Loader(Args[0], Args[1]);

  int Return{};

	if (Loader.IsValid()) {
    IRCodeLoader CodeLoader{&Loader};
    FEXCore::Context::InitCore(CTX, &CodeLoader);
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

    // Just re-use compare state. It also checks against the expected values in config.
    FEXCore::Core::CPUState State;
    FEXCore::Context::GetCPUState(CTX, &State);
    bool Passed = Loader.CompareStates(&State);

    LogMan::Msg::I("Passed? %s\n", Passed ? "Yes" : "No");

    Return = Passed ? 0 : -1;
  }
  else {
    LogMan::Msg::E("Couldn't load IR");
    Return = -1;
  }

  FEXCore::Context::DestroyContext(CTX);
  FEXCore::SHM::DestroyRegion(SHM);
  FEX::Config::Shutdown();

  return Return;
}
