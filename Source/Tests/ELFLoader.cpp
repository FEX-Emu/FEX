#include "Common/ArgumentLoader.h"
#include "Common/EnvironmentLoader.h"
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
#include <unistd.h>
#include <vector>

namespace {
static bool SilentLog;
static FILE *OutputFD {stdout};

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

  if (!SilentLog) {
    fprintf(OutputFD, "[%s] %s\n", CharLevel, Message);
    fflush(OutputFD);
  }
}

void AssertHandler(char const *Message) {
  if (!SilentLog) {
    fprintf(OutputFD, "[ASSERT] %s\n", Message);
    fflush(OutputFD);
  }
}

bool CheckMemMapping() {
  std::fstream fs;
  fs.open("/proc/self/maps", std::fstream::in | std::fstream::binary);
  std::string Line;
  while (std::getline(fs, Line)) {
    if (fs.eof()) break;
    uint64_t Begin, End;
    if (sscanf(Line.c_str(), "%lx-%lx", &Begin, &End) == 2) {
      // If a memory range is living inside the 32bit memory space then we have a problem
      if (Begin < 0x1'0000'0000) {
        return false;
      }
    }
  }

  fs.close();
  return true;
}
}

void InterpreterHandler(std::string *Filename, std::string const &RootFS, std::vector<std::string> *args) {
  // Open the file pointer to the filename and see if we need to find an interpreter
  std::fstream File;
  size_t FileSize{0};
  File.open(*Filename, std::fstream::in | std::fstream::binary);

  if (!File.is_open())
    return;

  File.seekg(0, File.end);
  FileSize = File.tellg();
  File.seekg(0, File.beg);

  // Is the file large enough for shebang
  if (FileSize <= 2)
    return;

  // Handle shebang files
  if (File.get() == '#' &&
      File.get() == '!') {
    std::string InterpreterLine;
    std::getline(File, InterpreterLine);

    // Shebang line can have a single argument
    std::istringstream InterpreterSS(InterpreterLine);
    std::string Argument;
    if (std::getline(InterpreterSS, Argument, ' ')) {
      // Push the filename in to the argument
      args->emplace(args->begin(), Argument);

      // Replace the filename
      *Filename = Argument;
    }

    // Now check for argument
    if (std::getline(InterpreterSS, Argument, ' ')) {
      // Insert after the interpreter filename
      args->emplace(args->begin() + 1, Argument);
    }

    // Done here
    return;
  }
}

int main(int argc, char **argv, char **const envp) {
  LogMan::Throw::InstallHandler(AssertHandler);
  LogMan::Msg::InstallHandler(MsgHandler);

  if (!CheckMemMapping()) {
    LogMan::Msg::E("[Unsupported] FEX mapped to lower 32bits! Exiting!");
    return -1;
  }

  FEX::Config::Init();
  FEX::EnvLoader::Load(envp);
  FEX::ArgLoader::Load(argc, argv);

  FEX::Config::Value<uint8_t> CoreConfig{"Core", 0};
  FEX::Config::Value<uint64_t> BlockSizeConfig{"MaxInst", 1};
  FEX::Config::Value<bool> SingleStepConfig{"SingleStep", false};
  FEX::Config::Value<bool> MultiblockConfig{"Multiblock", false};
  FEX::Config::Value<bool> GdbServerConfig{"GdbServer", false};
  FEX::Config::Value<uint64_t> ThreadsConfig{"Threads", 1};
  FEX::Config::Value<std::string> LDPath{"RootFS", ""};
  FEX::Config::Value<std::string> ThunkLibsPath{"ThunkLibs", ""};
  FEX::Config::Value<bool> SilentLog{"SilentLog", false};
  FEX::Config::Value<std::string> Environment{"Env", ""};
  FEX::Config::Value<std::string> OutputLog{"OutputLog", "stderr"};
  FEX::Config::Value<std::string> DumpIR{"DumpIR", "no"};
  FEX::Config::Value<bool> TSOEnabledConfig{"TSOEnabled", true};
  FEX::Config::Value<bool> SMCChecksConfig{"SMCChecks", false};
  FEX::Config::Value<bool> ABILocalFlags{"ABILocalFlags", false};
  FEX::Config::Value<bool> AbiNoPF{"AbiNoPF", false};

  ::SilentLog = SilentLog();

  if (!::SilentLog) {
    auto LogFile = OutputLog();
    if (LogFile == "stderr") {
      OutputFD = stderr;
    }
    else if (LogFile == "stdout") {
      OutputFD = stdout;
    }
    else if (!LogFile.empty()) {
      OutputFD = fopen(LogFile.c_str(), "wb");
    }
  }

  auto Args = FEX::ArgLoader::Get();
  auto ParsedArgs = FEX::ArgLoader::GetParsedArgs();

  LogMan::Throw::A(!Args.empty(), "Not enough arguments");

  std::string Program = Args[0];

  InterpreterHandler(&Program, LDPath(), &Args);

  FEX::HarnessHelper::ELFCodeLoader Loader{Program, LDPath(), Args, ParsedArgs, envp, &Environment};

  FEXCore::Context::InitializeStaticTables(Loader.Is64BitMode() ? FEXCore::Context::MODE_64BIT : FEXCore::Context::MODE_32BIT);
  uint64_t VMemSize = 1ULL << 36;
  if (!Loader.Is64BitMode()) {
    VMemSize = 1ULL << 32;
  }

  auto SHM = FEXCore::SHM::AllocateSHMRegion(VMemSize);
  auto CTX = FEXCore::Context::CreateNewContext();
  FEXCore::Context::InitializeContext(CTX);

  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_DEFAULTCORE, CoreConfig() > 3 ? FEXCore::Config::CONFIG_CUSTOM : CoreConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_MULTIBLOCK, MultiblockConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_SINGLESTEP, SingleStepConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_MAXBLOCKINST, BlockSizeConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_GDBSERVER, GdbServerConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_ROOTFSPATH, LDPath());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_THUNKLIBSPATH, ThunkLibsPath());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_UNIFIED_MEMORY, true);
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_IS64BIT_MODE, Loader.Is64BitMode());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_EMULATED_CPU_CORES, ThreadsConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_TSO_ENABLED, TSOEnabledConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_SMC_CHECKS, SMCChecksConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_ABI_LOCAL_FLAGS, ABILocalFlags());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_ABI_NO_PF, AbiNoPF());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_DUMPIR, DumpIR());

  FEXCore::Context::SetCustomCPUBackendFactory(CTX, VMFactory::CPUCreationFactory);
  // FEXCore::Context::SetFallbackCPUBackendFactory(CTX, VMFactory::CPUCreationFactoryFallback);

  FEXCore::Context::AddGuestMemoryRegion(CTX, SHM);
  FEXCore::Context::InitCore(CTX, &Loader);
  FEXCore::Context::SetApplicationFile(CTX, std::filesystem::canonical(Program));

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

  auto ProgramStatus = FEXCore::Context::GetProgramStatus(CTX);

  FEXCore::Context::DestroyContext(CTX);
  FEXCore::SHM::DestroyRegion(SHM);

  FEX::Config::Shutdown();

  if (OutputFD != stderr &&
      OutputFD != stdout &&
      OutputFD != nullptr) {
    fclose(OutputFD);
  }

  if (ShutdownReason == FEXCore::Context::ExitReason::EXIT_SHUTDOWN) {
    return ProgramStatus;
  }
  else {
    return -64 | ShutdownReason;
  }
}
