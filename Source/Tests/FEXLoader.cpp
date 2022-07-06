/*
$info$
tags: Bin|FEXLoader
desc: Glues the ELF loader, FEXCore and LinuxSyscalls to launch an elf under fex
$end_info$
*/

#include "AOT/AOTGenerator.h"
#include "Common/ArgumentLoader.h"
#include "Common/FEXServerClient.h"
#include "ELFCodeLoader2.h"
#include "Tests/LinuxSyscalls/LinuxAllocator.h"
#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include "Tests/LinuxSyscalls/SignalDelegator.h"
#include "Linux/Utils/ELFContainer.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/Telemetry.h>
#include <FEXCore/Utils/Threads.h>

#include <atomic>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <elf.h>
#include <fcntl.h>
#include <fstream>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <sys/auxv.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <system_error>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <sys/sysinfo.h>
#include <sys/signal.h>

namespace {
static bool SilentLog;
static int OutputFD {STDERR_FILENO};
static bool ExecutedWithFD {false};

void MsgHandler(LogMan::DebugLevels Level, char const *Message) {
  if (SilentLog) {
    return;
  }

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

  const auto Output = fmt::format("[{}] {}\n", CharLevel, Message);
  write(OutputFD, Output.c_str(), Output.size());
  fsync(OutputFD);
}

void AssertHandler(char const *Message) {
  if (SilentLog) {
    return;
  }

  const auto Output = fmt::format("[ASSERT] {}\n", Message);
  write(OutputFD, Output.c_str(), Output.size());
  fsync(OutputFD);
}

} // Anonymous namespace

namespace FEXServerLogging {
  int FEXServerFD{};
  void MsgHandler(LogMan::DebugLevels Level, char const *Message) {
    FEXServerClient::MsgHandler(FEXServerFD, Level, Message);
  }

  void AssertHandler(char const *Message) {
    FEXServerClient::AssertHandler(FEXServerFD, Message);
  }
}

void InterpreterHandler(std::string *Filename, std::string const &RootFS, std::vector<std::string> *args) {
  // Open the file pointer to the filename and see if we need to find an interpreter
  std::fstream File(*Filename, std::fstream::in | std::fstream::binary);

  if (!File.is_open()) {
    return;
  }

  File.seekg(0, std::fstream::end);
  const auto FileSize = File.tellg();
  File.seekg(0, std::fstream::beg);

  // Is the file large enough for shebang
  if (FileSize <= 2) {
    return;
  }

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
      ShebangArguments.push_back(std::move(Argument));
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

void RootFSRedirect(std::string *Filename, std::string const &RootFS) {
  auto RootFSLink = ELFCodeLoader2::ResolveRootfsFile(*Filename, RootFS);

  std::error_code ec{};
  if (std::filesystem::exists(RootFSLink, ec)) {
    *Filename = RootFSLink;
  }
}

bool RanAsInterpreter(const char *Program) {
  return ExecutedWithFD || strstr(Program, "FEXInterpreter") != nullptr;
}

bool RanAsAOTGen(const char *Program) {
  return strstr(Program, "AOTGen") != nullptr;
}

bool IsInterpreterInstalled() {
  // The interpreter is installed if both the binfmt_misc handlers are available
  // Or if we were originally executed with FD. Which means the interpreter is installed

  std::error_code ec{};
  return ExecutedWithFD ||
         (std::filesystem::exists("/proc/sys/fs/binfmt_misc/FEX-x86", ec) &&
         std::filesystem::exists("/proc/sys/fs/binfmt_misc/FEX-x86_64", ec));
}

int main(int argc, char **argv, char **const envp) {
  const bool IsInterpreter = RanAsInterpreter(argv[0]);
  const bool IsAOTGen = RanAsAOTGen(argv[0]);

  ExecutedWithFD = getauxval(AT_EXECFD) != 0;

  LogMan::Throw::InstallHandler(AssertHandler);
  LogMan::Msg::InstallHandler(MsgHandler);

  auto Program = FEX::Config::LoadConfig(
    IsInterpreter,
    true,
    argc, argv, envp
  );

  if (Program.first.empty()) {
    if (IsAOTGen) {
      printf("Usage: %s [options] path/to/executable\n", argv[0]);
    }
    // Early exit if we weren't passed an argument
    return 0;
  }

  auto Args = FEX::ArgLoader::Get();
  auto ParsedArgs = FEX::ArgLoader::GetParsedArgs();

  // Reload the meta layer
  FEXCore::Config::ReloadMetaLayer();
  FEXCore::Config::Set(FEXCore::Config::CONFIG_IS_INTERPRETER, IsInterpreter ? "1" : "0");
  FEXCore::Config::Set(FEXCore::Config::CONFIG_INTERPRETER_INSTALLED, IsInterpreterInstalled() ? "1" : "0");

  // Early check for process stall
  // Doesn't use CONFIG_ROOTFS and we don't want it to spin up a squashfs instance
  FEX_CONFIG_OPT(StallProcess, STALLPROCESS);
  if (StallProcess) {
    while (1) {
      // Stall this process out forever
      select(0, nullptr, nullptr, nullptr, nullptr);
    }
  }

  // Ensure FEXServer is setup before config options try to pull CONFIG_ROOTFS
  if (!FEXServerClient::SetupClient(argv[0])) {
    LogMan::Msg::EFmt("FEXServerClient: Failure to setup client");
    return -1;
  }

  FEX_CONFIG_OPT(SilentLog, SILENTLOG);
  FEX_CONFIG_OPT(IRCache, IRCACHE);
  FEX_CONFIG_OPT(ObjCache, OBJCACHE);
  FEX_CONFIG_OPT(OutputLog, OUTPUTLOG);
  FEX_CONFIG_OPT(LDPath, ROOTFS);
  FEX_CONFIG_OPT(Environment, ENV);
  FEX_CONFIG_OPT(HostEnvironment, HOSTENV);
  ::SilentLog = SilentLog();

  if (::SilentLog) {
    LogMan::Throw::UnInstallHandlers();
    LogMan::Msg::UnInstallHandlers();
  }
  else {
    auto LogFile = OutputLog();
    // If stderr or stdout then we need to dup the FD
    // In some cases some applications will close stderr and stdout
    // then redirect the FD to either a log OR some cases just not use
    // stderr/stdout and the FD will be reused for regular FD ops.
    //
    // We want to maintain the original output location otherwise we
    // can run in to problems of writing to some file
    if (LogFile == "stderr") {
      OutputFD = dup(STDERR_FILENO);
    }
    else if (LogFile == "stdout") {
      OutputFD = dup(STDOUT_FILENO);
    }
    else if (LogFile == "server") {
      LogMan::Throw::UnInstallHandlers();
      LogMan::Msg::UnInstallHandlers();

      FEXServerLogging::FEXServerFD = FEXServerClient::RequestLogFD(FEXServerClient::GetServerFD());
      if (FEXServerLogging::FEXServerFD != -1) {
        LogMan::Throw::InstallHandler(FEXServerLogging::AssertHandler);
        LogMan::Msg::InstallHandler(FEXServerLogging::MsgHandler);
      }
    }
    else if (!LogFile.empty()) {
      OutputFD = open(LogFile.c_str(), O_CREAT | O_CLOEXEC | O_WRONLY);
    }
  }

  FEXCore::Telemetry::Initialize();

  RootFSRedirect(&Program.first, LDPath());
  InterpreterHandler(&Program.first, LDPath(), &Args);

  std::error_code ec{};
  if (!std::filesystem::exists(Program.first, ec)) {
    // Early exit if the program passed in doesn't exist
    // Will prevent a crash later
    fmt::print(stderr, "{}: command not found\n", Program.first);
    return -ENOEXEC;
  }

  uint32_t KernelVersion = FEX::HLE::SyscallHandler::CalculateHostKernelVersion();
  if (KernelVersion < FEX::HLE::SyscallHandler::KernelVersion(4, 17)) {
    // We require 4.17 minimum for MAP_FIXED_NOREPLACE
    LogMan::Msg::EFmt("FEXLoader requires kernel 4.17 minimum. Expect problems.");
  }

  // Before we go any further, set all of our host environment variables that the config has provided
  for (auto &HostEnv : HostEnvironment.All()) {
    // We are going to keep these alive in memory.
    // No need to split the string with setenv
    putenv(HostEnv.data());
  }

  ELFCodeLoader2 Loader{Program.first, LDPath(), Args, ParsedArgs, envp, &Environment};
  //FEX::HarnessHelper::ELFCodeLoader Loader{Program.first, LDPath(), Args, ParsedArgs, envp, &Environment};

  if (!Loader.ELFWasLoaded()) {
    // Loader couldn't load this program for some reason
    fmt::print(stderr, "Invalid or Unsupported elf file.\n");
#ifdef _M_ARM_64
    fmt::print(stderr, "This is likely due to a misconfigured x86-64 RootFS\n");
    fmt::print(stderr, "Current RootFS path set to '{}'\n", LDPath());
    std::error_code ec;
    if (LDPath().empty() ||
        std::filesystem::exists(LDPath(), ec) == false) {
      fmt::print(stderr, "RootFS path doesn't exist. This is required on AArch64 hosts\n");
      fmt::print(stderr, "Use FEXRootFSFetcher to download a RootFS\n");
    }
#endif
    return -ENOEXEC;
  }

  FEXCore::Config::Set(FEXCore::Config::CONFIG_APP_FILENAME, std::filesystem::canonical(Program.first).string());
  FEXCore::Config::Set(FEXCore::Config::CONFIG_APP_CONFIG_NAME, Program.second);
  FEXCore::Config::Set(FEXCore::Config::CONFIG_IS64BIT_MODE, Loader.Is64BitMode() ? "1" : "0");

  std::unique_ptr<FEX::HLE::MemAllocator> Allocator;
  std::vector<FEXCore::Allocator::MemoryRegion> Base48Bit;

  if (Loader.Is64BitMode()) {
    // Destroy the 48th bit if it exists
    Base48Bit = FEXCore::Allocator::Steal48BitVA();
  } else {
    FEX_CONFIG_OPT(Use32BitAllocator, FORCE32BITALLOCATOR);
    if (KernelVersion < FEX::HLE::SyscallHandler::KernelVersion(4, 17)) {
      Use32BitAllocator = true;
    }

    // Setup our userspace allocator
    if (!Use32BitAllocator &&
        KernelVersion >= FEX::HLE::SyscallHandler::KernelVersion(4, 17)) {
      FEXCore::Allocator::SetupHooks();
    }

    if (Use32BitAllocator) {
      Allocator = FEX::HLE::Create32BitAllocator();
    }
    else {
      Allocator = FEX::HLE::CreatePassthroughAllocator();
    }
  }

  // System allocator is now system allocator or FEX
  FEXCore::Context::InitializeStaticTables(Loader.Is64BitMode() ? FEXCore::Context::MODE_64BIT : FEXCore::Context::MODE_32BIT);

  auto CTX = FEXCore::Context::CreateNewContext();
  FEXCore::Context::InitializeContext(CTX);

  auto SignalDelegation = std::make_unique<FEX::HLE::SignalDelegator>();

  SignalDelegation->RegisterFrontendHostSignalHandler(SIGILL, [&SignalDelegation](FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) -> bool {
    ucontext_t* _context = (ucontext_t*)ucontext;
    auto &mcontext = _context->uc_mcontext;
    uint64_t PC{};
#ifdef _M_ARM_64
    PC = mcontext.pc;
#else
    PC = mcontext.gregs[REG_RIP];
#endif
    if (PC == reinterpret_cast<uint64_t>(&FEXCore::Assert::ForcedAssert)) {
      // This is a host side assert. Don't deliver this to the guest
      // We want to actually break here
      SignalDelegation->UninstallHostHandler(Signal);
      return true;
    }
    return false;
  }, true);

  auto SyscallHandler = Loader.Is64BitMode() ? FEX::HLE::x64::CreateHandler(CTX, SignalDelegation.get())
                                             : FEX::HLE::x32::CreateHandler(CTX, SignalDelegation.get(), std::move(Allocator));

  auto Mapper = std::bind_front(&FEX::HLE::SyscallHandler::GuestMmap, SyscallHandler.get());
  auto Unmapper = std::bind_front(&FEX::HLE::SyscallHandler::GuestMunmap, SyscallHandler.get());

  if (!Loader.MapMemory(Mapper, Unmapper)) {
    // failed to map
    LogMan::Msg::EFmt("Failed to map {}-bit elf file.", Loader.Is64BitMode() ? 64 : 32);
    return -ENOEXEC;
  }

  SyscallHandler->SetCodeLoader(&Loader);

  auto BRKInfo = Loader.GetBRKInfo();

  SyscallHandler->DefaultProgramBreak(BRKInfo.Base, BRKInfo.Size);

  FEXCore::Context::SetSignalDelegator(CTX, SignalDelegation.get());
  FEXCore::Context::SetSyscallHandler(CTX, SyscallHandler.get());
  FEXCore::Context::InitCore(CTX, Loader.DefaultRIP(), Loader.GetStackPointer());

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

  if (IRCache() || ObjCache()) {
    LogMan::Msg::IFmt("Warning: OBJ/IR Caches are experimental, and might lead to crashes.");
  }

  if (IsAOTGen) {
    LogMan::Msg::IFmt("Warning: AOT Generation is experimental and might not work.");
  }

  auto GetCacheReader = [](bool IsIR) {

    auto JitCacheFolder = std::filesystem::path(FEXCore::Config::GetDataDirectory()) / "JitCache";

    return [JitCacheFolder=std::move(JitCacheFolder), IsIR](const std::string &fileid, const std::string &filename) -> std::optional<FEXCore::Context::CacheFDSet> {
      auto EntryFolder = JitCacheFolder / fileid;

      std::error_code ec;
      std::filesystem::create_directories(EntryFolder, ec);

      if (ec) {
        return std::nullopt;
      }

      auto EntryPath = EntryFolder / "Path";
      auto EntryIRIndex = EntryFolder / (IsIR? "IRIndex" : "OBJIndex");
      auto EntryIRData = EntryFolder / (IsIR? "IRData" : "OBJData");
      
      // Generate Path entry
      int PathFD = open(EntryPath.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0644);
      if (PathFD != -1) {
        write(PathFD, filename.c_str(), filename.size());
        close(PathFD);
      }

      // Create or open cache files
      int IndexFD = open(EntryIRIndex.c_str(), O_CREAT | O_RDWR | O_CLOEXEC, 0644);
      int DataFD = open(EntryIRData.c_str(), O_CREAT | O_RDWR | O_CLOEXEC, 0644);

      if (IndexFD == -1 || DataFD == -1) {

        if (IndexFD != -1) {
          close(IndexFD);
        }

        if (DataFD != -1) {
          close(DataFD);
        }

        return std::nullopt;
      } else {
        return FEXCore::Context::CacheFDSet {
          .IndexFD = IndexFD, .DataFD = DataFD
        };
      }
    };
  };

  if (ObjCache()) {
    FEXCore::Context::SetObjCacheOpener(CTX, GetCacheReader(false));
  }

  if (IRCache()) {
    FEXCore::Context::SetIRCacheOpener(CTX, GetCacheReader(true));
  }


  if (IsAOTGen) {
    for(auto &Section: Loader.Sections) {
      FEX::AOT::AOTGenSection(CTX, Section);
    }
  } else {
    FEXCore::Context::RunUntilExit(CTX);
  }

  auto ProgramStatus = FEXCore::Context::GetProgramStatus(CTX);

  SyscallHandler.reset();
  SignalDelegation.reset();
  FEXCore::Context::DestroyContext(CTX);

  FEXCore::Context::ShutdownStaticTables();
  FEXCore::Threads::Shutdown();

  Loader.FreeSections();

  FEXCore::Config::Shutdown();

  LogMan::Throw::UnInstallHandlers();
  LogMan::Msg::UnInstallHandlers();

  FEXCore::Allocator::ClearHooks();
  FEXCore::Allocator::ReclaimMemoryRegion(Base48Bit);
  // Allocator is now original system allocator
  FEXCore::Telemetry::Shutdown(Program.second);
  if (ShutdownReason == FEXCore::Context::ExitReason::EXIT_SHUTDOWN) {
    return ProgramStatus;
  }
  else {
    return -64 | ShutdownReason;
  }
}
