// SPDX-License-Identifier: MIT
/*
$info$
tags: Bin|FEXLoader
desc: Glues the ELF loader, FEXCore and LinuxSyscalls to launch an elf under fex
$end_info$
*/

#include "AOT/AOTGenerator.h"
#include "Common/ArgumentLoader.h"
#include "Common/FEXServerClient.h"
#include "Common/Config.h"
#include "Common/HostFeatures.h"
#include "ELFCodeLoader.h"
#include "VDSO_Emulation.h"
#include "LinuxSyscalls/GdbServer.h"
#include "LinuxSyscalls/LinuxAllocator.h"
#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/Utils/Threads.h"
#include "LinuxSyscalls/x32/Syscalls.h"
#include "LinuxSyscalls/x64/Syscalls.h"
#include "LinuxSyscalls/SignalDelegator.h"
#include "Linux/Utils/ELFContainer.h"
#include "Thunks.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/FileLoading.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/Telemetry.h>
#include <FEXCore/Utils/Threads.h>
#include <FEXCore/Utils/Profiler.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/sstream.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>
#include <FEXHeaderUtils/Filesystem.h>

#include <atomic>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <elf.h>
#include <fcntl.h>
#include <mutex>
#include <queue>
#include <set>
#include <sys/auxv.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <system_error>
#include <thread>
#include <unistd.h>
#include <utility>

#include <sys/sysinfo.h>
#include <sys/signal.h>

namespace {
static bool SilentLog;
static int OutputFD {-1};

void MsgHandler(LogMan::DebugLevels Level, const char* Message) {
  if (SilentLog) {
    return;
  }

  const auto Output = fextl::fmt::format("[{}] {}\n", LogMan::DebugLevelStr(Level), Message);
  write(OutputFD, Output.c_str(), Output.size());
  fsync(OutputFD);
}

void AssertHandler(const char* Message) {
  if (SilentLog) {
    return;
  }

  const auto Output = fextl::fmt::format("[ASSERT] {}\n", Message);
  write(OutputFD, Output.c_str(), Output.size());
  fsync(OutputFD);
}

} // Anonymous namespace

namespace FEXServerLogging {
int FEXServerFD {-1};
void MsgHandler(LogMan::DebugLevels Level, const char* Message) {
  FEXServerClient::MsgHandler(FEXServerFD, Level, Message);
}

void AssertHandler(const char* Message) {
  FEXServerClient::AssertHandler(FEXServerFD, Message);
}
} // namespace FEXServerLogging

namespace AOTIR {
class AOTIRWriterFD final : public FEXCore::Context::AOTIRWriter {
public:
  AOTIRWriterFD(const fextl::string& Path) {
    // Create and truncate if exists.
    constexpr int USER_PERMS = S_IRWXU | S_IRWXG | S_IRWXO;
    FD = open(Path.c_str(), O_CREAT | O_WRONLY | O_TRUNC | O_CLOEXEC, USER_PERMS);
  }

  operator bool() const {
    return FD != -1;
  }

  void Write(const void* Data, size_t Size) override {
    write(FD, Data, Size);
  }

  size_t Offset() override {
    return lseek(FD, 0, SEEK_CUR);
  }

  void Close() override {
    if (FD != -1) {
      close(FD);
      FD = -1;
    }
  }

  virtual ~AOTIRWriterFD() {
    Close();
  }
private:
  int FD {-1};
};
} // namespace AOTIR

void InterpreterHandler(fextl::string* Filename, const fextl::string& RootFS, fextl::vector<fextl::string>* args) {
  // Open the Filename to determine if it is a shebang file.
  int FD = open(Filename->c_str(), O_RDONLY | O_CLOEXEC);
  if (FD == -1) {
    return;
  }

  std::array<char, 257> Header;
  const auto ChunkSize = 257l;
  const auto ReadSize = pread(FD, &Header.at(0), ChunkSize, 0);

  const auto Data = std::span<char>(Header.data(), ReadSize);

  // Is the file large enough for shebang
  if (ReadSize <= 2) {
    close(FD);
    return;
  }

  // Handle shebang files
  if (Data[0] == '#' && Data[1] == '!') {
    fextl::string InterpreterLine {Data.begin() + 2, // strip off "#!" prefix
                                   std::find(Data.begin(), Data.end(), '\n')};
    fextl::vector<fextl::string> ShebangArguments {};

    // Shebang line can have a single argument
    fextl::istringstream InterpreterSS(InterpreterLine);
    fextl::string Argument;
    while (std::getline(InterpreterSS, Argument, ' ')) {
      if (Argument.empty()) {
        continue;
      }
      ShebangArguments.push_back(std::move(Argument));
    }

    // Executable argument
    fextl::string& ShebangProgram = ShebangArguments[0];

    // If the filename is absolute then prepend the rootfs
    // If it is relative then don't append the rootfs
    if (ShebangProgram[0] == '/') {
      ShebangProgram = RootFS + ShebangProgram;
    }
    *Filename = ShebangProgram;

    // Insert all the arguments at the start
    args->insert(args->begin(), ShebangArguments.begin(), ShebangArguments.end());
  }
  close(FD);
}

void RootFSRedirect(fextl::string* Filename, const fextl::string& RootFS) {
  auto RootFSLink = ELFCodeLoader::ResolveRootfsFile(*Filename, RootFS);

  if (FHU::Filesystem::Exists(RootFSLink)) {
    *Filename = RootFSLink;
  }
}

FEX::Config::PortableInformation ReadPortabilityInformation() {
  const FEX::Config::PortableInformation BadResult {false, {}};
  const char* PortableConfig = getenv("FEX_PORTABLE");
  if (!PortableConfig) {
    return BadResult;
  }

  uint32_t Value {};
  std::string_view PortableView {PortableConfig};

  if (std::from_chars(PortableView.data(), PortableView.data() + PortableView.size(), Value).ec != std::errc {} || Value == 0) {
    return BadResult;
  }

  // Read the FEXInterpreter path from `/proc/self/exe` which is always a symlink to the absolute path of the executable running.
  // This way we can get the parent path that the application is executing from.
  char SelfPath[PATH_MAX];
  auto Result = readlink("/proc/self/exe", SelfPath, PATH_MAX);
  if (Result == -1) {
    return BadResult;
  }

  std::string_view SelfPathView {SelfPath, std::min<size_t>(PATH_MAX, Result)};

  // Extract the absolute path from the FEXInterpreter path
  return {true, fextl::string {SelfPathView.substr(0, SelfPathView.find_last_of('/') + 1)}};
}

bool RanAsInterpreter(bool ExecutedWithFD) {
  return ExecutedWithFD || FEXLOADER_AS_INTERPRETER;
}

/**
 * @brief Queries if FEX is installed as a binfmt_misc interpreter
 *
 * @param ExecutedWithFD If FEXInterpreter was executed using a binfmt_misc FD handle from the kernel
 * @param Portable Portability information about FEX being run in portable mode
 *
 * @return true if the binfmt_misc handlers are installed and being used
 */
bool QueryInterpreterInstalled(bool ExecutedWithFD, const FEX::Config::PortableInformation& Portable) {
  if (Portable.IsPortable) {
    // Don't use binfmt interpreter even if it's installed
    return false;
  }

  // Check if FEX's binfmt_misc handlers are both installed.
  // The explicit check can be omitted if FEX was executed from an FD,
  // since this only happens if the kernel launched FEX through binfmt_misc
  return ExecutedWithFD || (access("/proc/sys/fs/binfmt_misc/FEX-x86", F_OK) == 0 && access("/proc/sys/fs/binfmt_misc/FEX-x86_64", F_OK) == 0);
}

namespace FEX::TSO {
void SetupTSOEmulation(FEXCore::Context::Context* CTX) {
  // We need to check if these are defined or not. This is a very fresh feature.
#ifndef PR_GET_MEM_MODEL
#define PR_GET_MEM_MODEL 0x6d4d444c
#endif
#ifndef PR_SET_MEM_MODEL
#define PR_SET_MEM_MODEL 0x4d4d444c
#endif
#ifndef PR_SET_MEM_MODEL_DEFAULT
#define PR_SET_MEM_MODEL_DEFAULT 0
#endif
#ifndef PR_SET_MEM_MODEL_TSO
#define PR_SET_MEM_MODEL_TSO 1
#endif
  // Check to see if this is supported.
  auto Result = prctl(PR_GET_MEM_MODEL, 0, 0, 0, 0);
  if (Result == -1) {
    // Unsupported, early exit.
    return;
  }

  FEX_CONFIG_OPT(TSOEnabled, TSOENABLED);

  if (!TSOEnabled()) {
    // TSO emulation isn't even enabled, early exit.
    return;
  }

  if (Result == PR_SET_MEM_MODEL_DEFAULT) {
    // Try to set the TSO mode if we are currently default.
    Result = prctl(PR_SET_MEM_MODEL, PR_SET_MEM_MODEL_TSO, 0, 0, 0);
    if (Result == 0) {
      // TSO mode successfully enabled. Tell the context to disable TSO emulation through atomics.
      // This flag gets inherited on thread creation, so FEX only needs to set it at the start.
      CTX->SetHardwareTSOSupport(true);
    }
  }
}
} // namespace FEX::TSO

/**
 * @brief Get an FD from an environment variable and then unset the environment variable.
 *
 * @param Env The environment variable to extract the FD from.
 *
 * @return -1 if the variable didn't exist.
 */
static int StealFEXFDFromEnv(const char* Env) {
  int FEXFD {-1};
  const char* FEXFDStr = getenv(Env);
  if (FEXFDStr) {
    const std::string_view FEXFDView {FEXFDStr};
    std::from_chars(FEXFDView.data(), FEXFDView.data() + FEXFDView.size(), FEXFD, 10);
    unsetenv(Env);
  }
  return FEXFD;
}

int main(int argc, char** argv, char** const envp) {
  auto SBRKPointer = FEXCore::Allocator::DisableSBRKAllocations();
  FEXCore::Allocator::GLIBCScopedFault GLIBFaultScope;

  const bool ExecutedWithFD = getauxval(AT_EXECFD) != 0;
  const bool IsInterpreter = RanAsInterpreter(ExecutedWithFD);
  const auto PortableInfo = ReadPortabilityInformation();
  const bool InterpreterInstalled = QueryInterpreterInstalled(ExecutedWithFD, PortableInfo);

  int FEXFD {StealFEXFDFromEnv("FEX_EXECVEFD")};
  int FEXSeccompFD {StealFEXFDFromEnv("FEX_SECCOMPFD")};

  LogMan::Throw::InstallHandler(AssertHandler);
  LogMan::Msg::InstallHandler(MsgHandler);

  auto ArgsLoader = fextl::make_unique<FEX::ArgLoader::ArgLoader>(
    IsInterpreter ? FEX::ArgLoader::ArgLoader::LoadType::WITHOUT_FEXLOADER_PARSER : FEX::ArgLoader::ArgLoader::LoadType::WITH_FEXLOADER_PARSER,
    argc, argv);
  auto Args = ArgsLoader->Get();
  auto ParsedArgs = ArgsLoader->GetParsedArgs();
  auto Program = FEX::Config::GetApplicationNames(Args, ExecutedWithFD, FEXFD);
  if (Program.ProgramPath.empty() && FEXFD == -1) {
    // Early exit if we weren't passed an argument
    return 0;
  }

  FEX::Config::LoadConfig(std::move(ArgsLoader), Program.ProgramName, envp, PortableInfo);

  // Reload the meta layer
  FEXCore::Config::ReloadMetaLayer();
  FEXCore::Config::Set(FEXCore::Config::CONFIG_IS_INTERPRETER, IsInterpreter ? "1" : "0");
  FEXCore::Config::Set(FEXCore::Config::CONFIG_INTERPRETER_INSTALLED, InterpreterInstalled ? "1" : "0");
#ifdef VIXL_SIMULATOR
  // If running under the vixl simulator, ensure that indirect runtime calls are enabled.
  FEXCore::Config::EraseSet(FEXCore::Config::CONFIG_DISABLE_VIXL_INDIRECT_RUNTIME_CALLS, "0");
#endif

  // Early check for process stall
  // Doesn't use CONFIG_ROOTFS and we don't want it to spin up a squashfs instance
  FEX_CONFIG_OPT(StallProcess, STALLPROCESS);
  FEX_CONFIG_OPT(StartupSleep, STARTUPSLEEP);
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
  FEX_CONFIG_OPT(AOTIRCapture, AOTIRCAPTURE);
  FEX_CONFIG_OPT(AOTIRGenerate, AOTIRGENERATE);
  FEX_CONFIG_OPT(AOTIRLoad, AOTIRLOAD);
  FEX_CONFIG_OPT(OutputLog, OUTPUTLOG);
  FEX_CONFIG_OPT(LDPath, ROOTFS);
  FEX_CONFIG_OPT(Environment, ENV);
  FEX_CONFIG_OPT(HostEnvironment, HOSTENV);
  ::SilentLog = SilentLog();

  if (::SilentLog) {
    LogMan::Throw::UnInstallHandler();
    LogMan::Msg::UnInstallHandler();
  } else {
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
    } else if (LogFile == "stdout") {
      OutputFD = dup(STDOUT_FILENO);
    } else if (LogFile == "server") {
      FEXServerLogging::FEXServerFD = FEXServerClient::RequestLogFD(FEXServerClient::GetServerFD());
      if (FEXServerLogging::FEXServerFD != -1) {
        LogMan::Throw::InstallHandler(FEXServerLogging::AssertHandler);
        LogMan::Msg::InstallHandler(FEXServerLogging::MsgHandler);
      }
    } else if (!LogFile.empty()) {
      constexpr int USER_PERMS = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
      OutputFD = open(LogFile.c_str(), O_CREAT | O_CLOEXEC | O_WRONLY, USER_PERMS);
    }
  }

  if (StartupSleep()) {
    LogMan::Msg::IFmt("[{}][{}] Sleeping for {} seconds", ::getpid(), Program.ProgramName, StartupSleep());
    std::this_thread::sleep_for(std::chrono::seconds(StartupSleep()));
  }

  FEXCore::Profiler::Init();
  FEXCore::Telemetry::Initialize();

  RootFSRedirect(&Program.ProgramPath, LDPath());
  InterpreterHandler(&Program.ProgramPath, LDPath(), &Args);

  if (!ExecutedWithFD && FEXFD == -1 && !FHU::Filesystem::Exists(Program.ProgramPath)) {
    // Early exit if the program passed in doesn't exist
    // Will prevent a crash later
    fextl::fmt::print(stderr, "{}: command not found\n", Program.ProgramPath);
    return -ENOEXEC;
  }

  uint32_t KernelVersion = FEX::HLE::SyscallHandler::CalculateHostKernelVersion();
  if (KernelVersion < FEX::HLE::SyscallHandler::KernelVersion(4, 17)) {
    // We require 4.17 minimum for MAP_FIXED_NOREPLACE
    LogMan::Msg::EFmt("FEXLoader requires kernel 4.17 minimum. Expect problems.");
  }

  // Before we go any further, set all of our host environment variables that the config has provided
  for (auto& HostEnv : HostEnvironment.All()) {
    // We are going to keep these alive in memory.
    // No need to split the string with setenv
    putenv(HostEnv.data());
  }

  ELFCodeLoader Loader {Program.ProgramPath, FEXFD, LDPath(), Args, ParsedArgs, envp, &Environment};

  if (!Loader.ELFWasLoaded()) {
    // Loader couldn't load this program for some reason
    fextl::fmt::print(stderr, "Invalid or Unsupported elf file.\n");
#ifdef _M_ARM_64
    fextl::fmt::print(stderr, "This is likely due to a misconfigured x86-64 RootFS\n");
    fextl::fmt::print(stderr, "Current RootFS path set to '{}'\n", LDPath());
    if (LDPath().empty() || FHU::Filesystem::Exists(LDPath()) == false) {
      fextl::fmt::print(stderr, "RootFS path doesn't exist. This is required on AArch64 hosts\n");
      fextl::fmt::print(stderr, "Use FEXRootFSFetcher to download a RootFS\n");
    }
#endif
    return -ENOEXEC;
  }

  if (ExecutedWithFD) {
    // Don't need to canonicalize Program.ProgramPath, Config loader will have resolved this already.
    FEXCore::Config::EraseSet(FEXCore::Config::CONFIG_APP_FILENAME, Program.ProgramPath);
    FEXCore::Config::EraseSet(FEXCore::Config::CONFIG_APP_CONFIG_NAME, Program.ProgramName);
  } else if (FEXFD != -1) {
    // Anonymous program.
    FEXCore::Config::EraseSet(FEXCore::Config::CONFIG_APP_FILENAME, "<Anonymous>");
    FEXCore::Config::EraseSet(FEXCore::Config::CONFIG_APP_CONFIG_NAME, "<Anonymous>");
  } else {
    {
      char ExistsTempPath[PATH_MAX];
      char* RealPath = realpath(Program.ProgramPath.c_str(), ExistsTempPath);
      if (RealPath) {
        FEXCore::Config::EraseSet(FEXCore::Config::CONFIG_APP_FILENAME, fextl::string(RealPath));
      }
    }
    FEXCore::Config::EraseSet(FEXCore::Config::CONFIG_APP_CONFIG_NAME, Program.ProgramName);
  }

  // Setup Thread handlers, so FEXCore can create threads.
  auto StackTracker = FEX::LinuxEmulation::Threads::SetupThreadHandlers();

  FEXCore::Config::EraseSet(FEXCore::Config::CONFIG_IS64BIT_MODE, Loader.Is64BitMode() ? "1" : "0");

  fextl::unique_ptr<FEX::HLE::MemAllocator> Allocator;
  fextl::vector<FEXCore::Allocator::MemoryRegion> Base48Bit;
  fextl::vector<FEXCore::Allocator::MemoryRegion> Low4GB;

  if (Loader.Is64BitMode()) {
    // Destroy the 48th bit if it exists
    Base48Bit = FEXCore::Allocator::Steal48BitVA();
  } else {
    // Reserve [0x1_0000_0000, 0x2_0000_0000).
    // Safety net if 32-bit address calculation overflows in to 64-bit range.
    constexpr uint64_t First64BitAddr = 0x1'0000'0000ULL;
    Low4GB = FEXCore::Allocator::StealMemoryRegion(First64BitAddr, First64BitAddr + First64BitAddr);

    // Setup our userspace allocator
    FEXCore::Allocator::SetupHooks();
    Allocator = FEX::HLE::CreatePassthroughAllocator();

    // Now that the upper 32-bit address space is blocked for future allocations,
    // exhaust all of jemalloc's remaining internal allocations that it reserved before.
    // TODO: It's unclear how reliably this exhausts those reserves
    FEXCore::Allocator::YesIKnowImNotSupposedToUseTheGlibcAllocator glibc;
    void* data;
    do {
      data = malloc(0x1);
    } while (reinterpret_cast<uintptr_t>(data) >> 32 != 0);
    free(data);
  }

  // System allocator is now system allocator or FEX
  FEXCore::Context::InitializeStaticTables(Loader.Is64BitMode() ? FEXCore::Context::MODE_64BIT : FEXCore::Context::MODE_32BIT);

  bool SupportsAVX {};
  fextl::unique_ptr<FEXCore::Context::Context> CTX;
  {
    auto HostFeatures = FEX::FetchHostFeatures();
    CTX = FEXCore::Context::Context::CreateNewContext(HostFeatures);
    SupportsAVX = HostFeatures.SupportsAVX;
  }

  // Setup TSO hardware emulation immediately after initializing the context.
  FEX::TSO::SetupTSOEmulation(CTX.get());

  auto SignalDelegation = FEX::HLE::CreateSignalDelegator(CTX.get(), Program.ProgramName, SupportsAVX);

  auto SyscallHandler = Loader.Is64BitMode() ? FEX::HLE::x64::CreateHandler(CTX.get(), SignalDelegation.get()) :
                                               FEX::HLE::x32::CreateHandler(CTX.get(), SignalDelegation.get(), std::move(Allocator));

  auto ThunkHandler = FEX::HLE::CreateThunkHandler();

  // Load VDSO in to memory prior to mapping our ELFs.
  auto VDSOMapping = FEX::VDSO::LoadVDSOThunks(Loader.Is64BitMode(), SyscallHandler.get());

  // Now that we have the syscall handler. Track some FDs that are FEX owned.
  if (OutputFD != -1) {
    SyscallHandler->FM.TrackFEXFD(OutputFD);
  }
  SyscallHandler->FM.TrackFEXFD(FEXServerClient::GetServerFD());
  if (FEXServerLogging::FEXServerFD != -1) {
    SyscallHandler->FM.TrackFEXFD(FEXServerLogging::FEXServerFD);
  }

  {
    Loader.SetVDSOBase(VDSOMapping.VDSOBase);
    Loader.CalculateHWCaps(CTX.get());

    if (!Loader.MapMemory(SyscallHandler.get())) {
      // failed to map
      LogMan::Msg::EFmt("Failed to map {}-bit elf file.", Loader.Is64BitMode() ? 64 : 32);
      return -ENOEXEC;
    }
  }

  SyscallHandler->SetCodeLoader(&Loader);

  auto BRKInfo = Loader.GetBRKInfo();

  SyscallHandler->DefaultProgramBreak(BRKInfo.Base, BRKInfo.Size);

  CTX->SetSignalDelegator(SignalDelegation.get());
  CTX->SetSyscallHandler(SyscallHandler.get());
  CTX->SetThunkHandler(ThunkHandler.get());

  FEX_CONFIG_OPT(GdbServer, GDBSERVER);
  fextl::unique_ptr<FEX::GdbServer> DebugServer;
  if (GdbServer) {
    DebugServer = fextl::make_unique<FEX::GdbServer>(CTX.get(), SignalDelegation.get(), SyscallHandler.get());
  }

  if (!CTX->InitCore()) {
    return 1;
  }

  auto ParentThread = SyscallHandler->TM.CreateThread(Loader.DefaultRIP(), Loader.GetStackPointer());
  SyscallHandler->TM.TrackThread(ParentThread);
  SignalDelegation->RegisterTLSState(ParentThread);
  ThunkHandler->RegisterTLSState(CTX.get(), ParentThread);

  // Pass in our VDSO thunks
  ThunkHandler->AppendThunkDefinitions(FEX::VDSO::GetVDSOThunkDefinitions());
  SignalDelegation->SetVDSOSigReturn();

  SyscallHandler->DeserializeSeccompFD(ParentThread, FEXSeccompFD);

  FEXCore::Context::ExitReason ShutdownReason = FEXCore::Context::ExitReason::EXIT_SHUTDOWN;

  // There might already be an exit handler, leave it installed
  if (!CTX->GetExitHandler()) {
    CTX->SetExitHandler([&](FEXCore::Core::InternalThreadState* Thread, FEXCore::Context::ExitReason reason) {
      if (reason != FEXCore::Context::ExitReason::EXIT_DEBUG) {
        ShutdownReason = reason;
        SyscallHandler->TM.Stop();
      }
    });
  }

  const bool AOTEnabled = AOTIRLoad() || AOTIRCapture() || AOTIRGenerate();
  if (AOTEnabled) {
    LogMan::Msg::IFmt("Warning: AOTIR is experimental, and might lead to crashes. "
                      "Capture doesn't work with programs that fork.");

    CTX->SetAOTIRLoader([](const fextl::string& fileid) -> int {
      const auto filepath = fextl::fmt::format("{}/aotir/{}.aotir", FEXCore::Config::GetDataDirectory(), fileid);
      return open(filepath.c_str(), O_RDONLY);
    });

    CTX->SetAOTIRWriter([](const fextl::string& fileid) -> fextl::unique_ptr<AOTIR::AOTIRWriterFD> {
      const auto filepath = fextl::fmt::format("{}/aotir/{}.aotir.tmp", FEXCore::Config::GetDataDirectory(), fileid);
      auto AOTWrite = fextl::make_unique<AOTIR::AOTIRWriterFD>(filepath);
      if (*AOTWrite) {
        LogMan::Msg::IFmt("AOTIR: Storing {}", fileid);
      } else {
        LogMan::Msg::IFmt("AOTIR: Failed to store {}", fileid);
      }
      return AOTWrite;
    });

    CTX->SetAOTIRRenamer([](const fextl::string& fileid) -> void {
      const auto TmpFilepath = fextl::fmt::format("{}/aotir/{}.aotir.tmp", FEXCore::Config::GetDataDirectory(), fileid);
      const auto NewFilepath = fextl::fmt::format("{}/aotir/{}.aotir", FEXCore::Config::GetDataDirectory(), fileid);

      // Rename the temporary file to atomically update the file
      if (!FHU::Filesystem::RenameFile(TmpFilepath, NewFilepath)) {
        LogMan::Msg::IFmt("Couldn't rename aotir");
      }
    });
  }

  if (AOTIRGenerate()) {
    for (auto& Section : Loader.Sections) {
      FEX::AOT::AOTGenSection(CTX.get(), Section);
    }
  } else {
    CTX->RunUntilExit(ParentThread->Thread);
  }

  if (AOTEnabled) {
    if (FHU::Filesystem::CreateDirectories(fextl::fmt::format("{}/aotir", FEXCore::Config::GetDataDirectory()))) {
      CTX->WriteFilesWithCode([](const fextl::string& fileid, const fextl::string& filename) {
        const auto filepath = fextl::fmt::format("{}/aotir/{}.path", FEXCore::Config::GetDataDirectory(), fileid);
        int fd = open(filepath.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0644);
        if (fd != -1) {
          write(fd, filename.c_str(), filename.size());
          close(fd);
        }
      });
    }

    if (AOTIRCapture() || AOTIRGenerate()) {
      CTX->FinalizeAOTIRCache();
      LogMan::Msg::IFmt("AOTIR Cache Stored");
    }
  }

  auto ProgramStatus = ParentThread->Thread->StatusCode;

  SignalDelegation->UninstallTLSState(ParentThread);
  SyscallHandler->TM.DestroyThread(ParentThread);

  FEX::VDSO::UnloadVDSOMapping(VDSOMapping);

  DebugServer.reset();
  SyscallHandler.reset();
  SignalDelegation.reset();

  FEX::LinuxEmulation::Threads::Shutdown(std::move(StackTracker));

  Loader.FreeSections();

  FEXCore::Config::Shutdown();

  LogMan::Throw::UnInstallHandler();
  LogMan::Msg::UnInstallHandler();

  FEXCore::Allocator::ClearHooks();
  FEXCore::Allocator::ReclaimMemoryRegion(Base48Bit);
  FEXCore::Allocator::ReclaimMemoryRegion(Low4GB);

  // Allocator is now original system allocator
  FEXCore::Telemetry::Shutdown(Program.ProgramName);
  FEXCore::Profiler::Shutdown();

  FEXCore::Allocator::ReenableSBRKAllocations(SBRKPointer);

  if (ShutdownReason == FEXCore::Context::ExitReason::EXIT_SHUTDOWN) {
    return ProgramStatus;
  } else {
    return -64 | ShutdownReason;
  }
}
