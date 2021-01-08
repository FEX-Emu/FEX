#include "Common/ArgumentLoader.h"
#include "Common/EnvironmentLoader.h"
#include "Common/Config.h"
#include "HarnessHelpers.h"
#include "IRLoader/Loader.h"
#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/SignalDelegator.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CodeLoader.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Utils/LogManager.h>

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

    uint64_t SetupStack() override {
      return reinterpret_cast<uint64_t>(mmap(nullptr, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    }

    uint64_t DefaultRIP() const override {
      return IR->GetEntryRIP();
    }

    void MapMemoryRegion() override {
      // Map the memory regions the test file asks for
      IR->MapRegions();
    }

    void LoadMemory() override {
      IR->LoadMemory();
    }

    uint64_t GetFinalRIP() override { return 0; }

    virtual void AddIR(IRHandler Handler) override {
      Handler(IR->GetEntryRIP(), IR);
    }

  private:
    FEX::IRLoader::Loader *IR;
    constexpr static uint64_t STACK_SIZE = 8 * 1024 * 1024;
};

int main(int argc, char **argv, char **const envp) {
  LogMan::Throw::InstallHandler(AssertHandler);
  LogMan::Msg::InstallHandler(MsgHandler);

  FEXCore::Config::Initialize();
  FEXCore::Config::AddLayer(std::make_unique<FEX::ArgLoader::ArgLoader>(argc, argv));
  FEXCore::Config::AddLayer(std::make_unique<FEX::Config::EnvLoader>(envp));
  FEXCore::Config::Load();

  FEXCore::Config::Value<uint8_t> CoreConfig{FEXCore::Config::CONFIG_DEFAULTCORE, 0};
  FEXCore::Config::Value<uint64_t> BlockSizeConfig{FEXCore::Config::CONFIG_MAXBLOCKINST, 1};
  FEXCore::Config::Value<bool> SingleStepConfig{FEXCore::Config::CONFIG_SINGLESTEP, false};
  FEXCore::Config::Value<bool> MultiblockConfig{FEXCore::Config::CONFIG_MULTIBLOCK, false};
  FEXCore::Config::Value<bool> GdbServerConfig{FEXCore::Config::CONFIG_GDBSERVER, false};
  FEXCore::Config::Value<std::string> LDPath{FEXCore::Config::CONFIG_ROOTFSPATH, ""};

  auto Args = FEX::ArgLoader::Get();
  auto ParsedArgs = FEX::ArgLoader::GetParsedArgs();

  LogMan::Throw::A(Args.size() > 1, "Not enough arguments");

  FEXCore::Context::InitializeStaticTables();
  auto CTX = FEXCore::Context::CreateNewContext();
  FEXCore::Context::InitializeContext(CTX);

  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_DEFAULTCORE, CoreConfig() > 3 ? FEXCore::Config::CONFIG_CUSTOM : CoreConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_MULTIBLOCK, MultiblockConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_SINGLESTEP, SingleStepConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_MAXBLOCKINST, BlockSizeConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_GDBSERVER, GdbServerConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_ROOTFSPATH, LDPath());
  std::unique_ptr<FEX::HLE::SignalDelegator> SignalDelegation = std::make_unique<FEX::HLE::SignalDelegator>();

  FEXCore::Context::SetSignalDelegator(CTX, SignalDelegation.get());

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

  return Return;
}
