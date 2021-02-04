#include "Common/ArgumentLoader.h"
#include "Common/EnvironmentLoader.h"
#include "Common/Config.h"
#include "HarnessHelpers.h"
#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/SignalDelegator.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CodeLoader.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Utils/ELFLoader.h>
#include <FEXCore/Utils/LogManager.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <unistd.h>
#include <vector>
#include <fstream>
#include <filesystem>

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
    std::vector<std::string> ShebangArguments{};

    // Shebang line can have a single argument
    std::istringstream InterpreterSS(InterpreterLine);
    std::string Argument;
    while (std::getline(InterpreterSS, Argument, ' ')) {
      if (Argument.empty()) {
        continue;
      }
      ShebangArguments.emplace_back(Argument);
    }

    // Executable argument
    std::string &ShebangProgram = ShebangArguments[0];

    // If the filename is absolute then prepend the rootfs
    // If it is relative then don't append the rootfs
    if (ShebangProgram[0] == '/') {
      ShebangProgram = RootFS + ShebangProgram;
    }
    *Filename = ShebangProgram;

    // Insert all the arguments at the start
    args->insert(args->begin(), ShebangArguments.begin(), ShebangArguments.end());

    // Done here
    return;
  }
}

bool RanAsInterpreter(char *Program) {
  return strstr(Program, "FEXInterpreter") != nullptr;
}

bool IsInterpreterInstalled() {
  // The interpreter is installed if both the binfmt_misc handlers are available
  return std::filesystem::exists("/proc/sys/fs/binfmt_misc/FEX-x86") &&
         std::filesystem::exists("/proc/sys/fs/binfmt_misc/FEX-x86_64");
}

int main(int argc, char **argv, char **const envp) {
  bool IsInterpreter = RanAsInterpreter(argv[0]);
  LogMan::Throw::InstallHandler(AssertHandler);
  LogMan::Msg::InstallHandler(MsgHandler);

#if !(defined(ENABLE_ASAN) && ENABLE_ASAN)
  // LLVM ASAN maps things to the lower 32bits
  if (!CheckMemMapping()) {
    LogMan::Msg::E("[Unsupported] FEX mapped to lower 32bits! Exiting!");
    return -1;
  }
#endif

  FEXCore::Config::Initialize();
  FEXCore::Config::AddLayer(std::make_unique<FEX::Config::MainLoader>());

  if (IsInterpreter) {
    FEX::ArgLoader::LoadWithoutArguments(argc, argv);
  }
  else {
    FEXCore::Config::AddLayer(std::make_unique<FEX::ArgLoader::ArgLoader>(argc, argv));
  }

  FEXCore::Config::AddLayer(std::make_unique<FEX::Config::EnvLoader>(envp));
  FEXCore::Config::Load();

  auto Args = FEX::ArgLoader::Get();
  auto ParsedArgs = FEX::ArgLoader::GetParsedArgs();

  if (Args.empty()) {
    // Early exit if we weren't passed an argument
    return 0;
  }

  std::string Program = Args[0];

  // These layers load on initialization
  FEXCore::Config::AddLayer(std::make_unique<FEX::Config::AppLoader>(std::filesystem::path(Program).filename(), true));
  FEXCore::Config::AddLayer(std::make_unique<FEX::Config::AppLoader>(std::filesystem::path(Program).filename(), false));

  // Reload the meta layer
  FEXCore::Config::ReloadMetaLayer();
  FEXCore::Config::Set(FEXCore::Config::CONFIG_IS_INTERPRETER, IsInterpreter ? "1" : "0");
  FEXCore::Config::Set(FEXCore::Config::CONFIG_INTERPRETER_INSTALLED, IsInterpreterInstalled() ? "1" : "0");

  FEXCore::Config::Value<uint8_t> CoreConfig{FEXCore::Config::CONFIG_DEFAULTCORE, 1};
  FEXCore::Config::Value<uint64_t> BlockSizeConfig{FEXCore::Config::CONFIG_MAXBLOCKINST, 5000};
  FEXCore::Config::Value<bool> SingleStepConfig{FEXCore::Config::CONFIG_SINGLESTEP, false};
  FEXCore::Config::Value<bool> MultiblockConfig{FEXCore::Config::CONFIG_MULTIBLOCK, true};
  FEXCore::Config::Value<bool> GdbServerConfig{FEXCore::Config::CONFIG_GDBSERVER, false};
  FEXCore::Config::Value<std::string> LDPath{FEXCore::Config::CONFIG_ROOTFSPATH, ""};
  FEXCore::Config::Value<std::string> ThunkLibsPath{FEXCore::Config::CONFIG_THUNKLIBSPATH, ""};
  FEXCore::Config::Value<bool> SilentLog{FEXCore::Config::CONFIG_SILENTLOGS, false};
  FEXCore::Config::Value<std::string> Environment{FEXCore::Config::CONFIG_ENVIRONMENT, ""};
  FEXCore::Config::Value<std::string> OutputLog{FEXCore::Config::CONFIG_OUTPUTLOG, "stderr"};
  FEXCore::Config::Value<std::string> DumpIR{FEXCore::Config::CONFIG_DUMPIR, "no"};
  FEXCore::Config::Value<bool> TSOEnabledConfig{FEXCore::Config::CONFIG_TSO_ENABLED, true};
  FEXCore::Config::Value<bool> SMCChecksConfig{FEXCore::Config::CONFIG_SMC_CHECKS, false};
  FEXCore::Config::Value<bool> ABILocalFlags{FEXCore::Config::CONFIG_ABI_LOCAL_FLAGS, false};
  FEXCore::Config::Value<bool> AbiNoPF{FEXCore::Config::CONFIG_ABI_NO_PF, false};
  FEXCore::Config::Value<bool> AOTIRCapture{FEXCore::Config::CONFIG_AOTIR_GENERATE, false};
  FEXCore::Config::Value<bool> AOTIRLoad{FEXCore::Config::CONFIG_AOTIR_LOAD, false};

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

  InterpreterHandler(&Program, LDPath(), &Args);

  if (!std::filesystem::exists(Program)) {
    // Early exit if the program passed in doesn't exist
    // Will prevent a crash later
    LogMan::Msg::E("%s: command not found", Program.c_str());
    return 0;
  }

  FEX::HarnessHelper::ELFCodeLoader Loader{Program, LDPath(), Args, ParsedArgs, envp, &Environment};

  uint32_t KernelVersion = FEX::HLE::SyscallHandler::CalculateHostKernelVersion();
  if (KernelVersion < FEX::HLE::SyscallHandler::KernelVersion(4, 17) &&
      !Loader.Is64BitMode()) {
    // We require 4.17 minimum for MAP_FIXED_NOREPLACE on 32bit ELFs
    LogMan::Msg::E("FEXLoader requires kernel 4.17 minimum");
    return -1;
  }

  FEXCore::Context::InitializeStaticTables(Loader.Is64BitMode() ? FEXCore::Context::MODE_64BIT : FEXCore::Context::MODE_32BIT);
  auto CTX = FEXCore::Context::CreateNewContext();
  FEXCore::Context::InitializeContext(CTX);

  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_DEFAULTCORE, CoreConfig() > 3 ? FEXCore::Config::CONFIG_CUSTOM : CoreConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_MULTIBLOCK, MultiblockConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_SINGLESTEP, SingleStepConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_MAXBLOCKINST, BlockSizeConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_GDBSERVER, GdbServerConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_ROOTFSPATH, LDPath());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_THUNKLIBSPATH, ThunkLibsPath());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_IS64BIT_MODE, Loader.Is64BitMode());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_TSO_ENABLED, TSOEnabledConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_SMC_CHECKS, SMCChecksConfig());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_ABI_LOCAL_FLAGS, ABILocalFlags());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_ABI_NO_PF, AbiNoPF());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_DUMPIR, DumpIR());
  FEXCore::Config::Set(FEXCore::Config::CONFIG_APP_FILENAME, std::filesystem::canonical(Program));
  FEXCore::Config::Set(FEXCore::Config::CONFIG_IS64BIT_MODE, Loader.Is64BitMode() ? "1" : "0");
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_AOTIR_GENERATE, AOTIRCapture());
  FEXCore::Config::SetConfig(CTX, FEXCore::Config::CONFIG_AOTIR_LOAD, AOTIRLoad());

  std::unique_ptr<FEX::HLE::SignalDelegator> SignalDelegation = std::make_unique<FEX::HLE::SignalDelegator>();
  std::unique_ptr<FEX::HLE::SyscallHandler> SyscallHandler{
    FEX::HLE::CreateHandler(
      Loader.Is64BitMode() ? FEXCore::Context::OperatingMode::MODE_64BIT : FEXCore::Context::OperatingMode::MODE_32BIT,
      CTX,
      SignalDelegation.get(),
      &Loader)};
  auto BRKInfo = Loader.GetBRKInfo();
  SyscallHandler->DefaultProgramBreak(BRKInfo.Base, BRKInfo.Size);

  FEXCore::Context::SetSignalDelegator(CTX, SignalDelegation.get());
  FEXCore::Context::SetSyscallHandler(CTX, SyscallHandler.get());
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

  std::string base_filename = Program.substr(Program.find_last_of("/\\") + 1) + ".fex-emu.iraot";

  if (AOTIRLoad() || AOTIRCapture()) {
    LogMan::Msg::I("Warning: AOTIR is experimental, and might lead to crashes. Capture doesn't work with programs that fork.");
  }

  FEXCore::Context::SetAOTIRLoader(CTX, [](const std::string &fileid) -> std::unique_ptr<std::istream> {
    auto filepath = std::string(getenv("HOME")) + "/.fex-emu/aotir/" + fileid;

    return std::make_unique<std::ifstream>(filepath, std::ios::in | std::ios::binary);
  });

  FEXCore::Context::RunUntilExit(CTX);

  if (AOTIRCapture()) {
    mkdir((std::string(getenv("HOME")) + "/.fex-emu/").c_str(), 0700);
    mkdir((std::string(getenv("HOME")) + "/.fex-emu/aotir").c_str(), 0700);

    auto WroteCache = FEXCore::Context::WriteAOTIR(CTX, [](const std::string& fileid) -> std::unique_ptr<std::ostream> {
      auto filepath = std::string(getenv("HOME")) + "/.fex-emu/aotir/" + fileid;
      auto AOTWrite = std::make_unique<std::ofstream>(filepath, std::ios::out | std::ios::binary);
      if (*AOTWrite) {
        std::filesystem::resize_file(filepath, 0);
        AOTWrite->seekp(0);
        LogMan::Msg::I("AOTIR: Storing %s", fileid.c_str());
      } else {
        LogMan::Msg::I("AOTIR: Failed to store %s", fileid.c_str());
      }
      return AOTWrite;
    });

    if (WroteCache) {
      LogMan::Msg::I("AOTIR Cache Stored");
    } else {
      LogMan::Msg::E("AOTIR Cache Store Failed");
    }
  }

  auto ProgramStatus = FEXCore::Context::GetProgramStatus(CTX);

  SyscallHandler.reset();
  SignalDelegation.reset();
  FEXCore::Context::DestroyContext(CTX);

  FEXCore::Config::Shutdown();

  LogMan::Throw::UnInstallHandlers();
  LogMan::Msg::UnInstallHandlers();

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
