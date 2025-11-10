// SPDX-License-Identifier: MIT
/*
$info$
tags: Bin|FEX
desc: Glues the ELF loader, FEXCore and LinuxSyscalls to launch an elf under fex
$end_info$
*/

#include "Common/ArgumentLoader.h"
#include "Common/FEXServerClient.h"
#include "Common/Config.h"
#include "Common/HostFeatures.h"
#include "PortabilityInfo.h"
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
#include <FEXCore/Utils/PrctlUtils.h>
#include <FEXCore/Utils/Profiler.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/sstream.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>
#include <FEXHeaderUtils/Filesystem.h>
#include <FEXHeaderUtils/StringArgumentParser.h>

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
static bool SilentLog {};
static int OutputFD {STDERR_FILENO};

// Set an empty style to disable coloring when FEXServer output is e.g. piped to a file
static bool DisableOutputColors {};

void MsgHandler(LogMan::DebugLevels Level, const char* Message) {
  if (SilentLog) {
    return;
  }

  const auto Style = DisableOutputColors ? fmt::text_style {} : LogMan::DebugLevelStyle(Level);
  const auto Output = fextl::fmt::format("{} {}\n", fmt::styled(LogMan::DebugLevelStr(Level), Style), Message);
  write(OutputFD, Output.c_str(), Output.size());
  fsync(OutputFD);
}

void AssertHandler(const char* Message) {
  return MsgHandler(LogMan::ASSERT, Message);
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

bool InterpreterHandler(fextl::string* Filename, const fextl::string& RootFS, fextl::vector<fextl::string>* args) {
  int FD {-1};

  // Attempt to open the filename from the rootfs first.
  FD = open(fextl::fmt::format("{}{}", RootFS, *Filename).c_str(), O_RDONLY | O_CLOEXEC);
  if (FD == -1) {
    // Failing that, attempt to open the filename directly.
    FD = open(Filename->c_str(), O_RDONLY | O_CLOEXEC);
    if (FD == -1) {
      return false;
    }
  }

  std::array<char, 257> Header;
  const auto ChunkSize = 257l;
  const auto ReadSize = pread(FD, Header.data(), ChunkSize, 0);
  close(FD);

  const auto Data = std::span<char>(Header.data(), ReadSize);

  // Is the file large enough for shebang
  if (ReadSize <= 2) {
    return false;
  }

  // Handle shebang files
  if (Data[0] == '#' && Data[1] == '!') {
    std::string_view InterpreterLine {Data.begin() + 2, // strip off "#!" prefix
                                      std::find(Data.begin(), Data.end(), '\n')};
    const auto ShebangArguments = FHU::ParseArgumentsFromString(InterpreterLine);

    // Executable argument
    *Filename = ShebangArguments.at(0);

    // Insert all the arguments at the start
    args->insert(args->begin(), ShebangArguments.begin(), ShebangArguments.end());
  }
  return true;
}

/**
 * @brief Queries if FEX is installed as a binfmt_misc interpreter
 *
 * @param ExecutedWithFD If FEX was executed using a binfmt_misc FD handle from the kernel
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

namespace FEX::CompatInput {
void SetupCompatInput(bool enable) {
  // Check to see if this is supported.
  auto Result = prctl(PR_GET_COMPAT_INPUT, 0, 0, 0, 0);
  if (Result == -1) {
    // Unsupported, early exit.
    return;
  }

  if (enable) {
    prctl(PR_SET_COMPAT_INPUT, PR_SET_COMPAT_INPUT_ENABLE, 0, 0, 0);
  } else {
    prctl(PR_SET_COMPAT_INPUT, PR_SET_COMPAT_INPUT_DISABLE, 0, 0, 0);
  }
}
} // namespace FEX::CompatInput

namespace FEX::GCS {
void CheckForGCS() {
  uint64_t ShadowStackWord {};
  if (prctl(PR_GET_SHADOW_STACK_STATUS, &ShadowStackWord, 0, 0, 0) == -1) {
    return;
  }

  // Kernel supports shadow stack.
  if (ShadowStackWord & PR_SHADOW_STACK_ENABLE) {
    // Welp.
    ERROR_AND_DIE_FMT("Shadow stack is enabled which FEX is incompatible with!");
  }

  // Disable if we've gotten this far, to ensure guest can't try.
  prctl(PR_LOCK_SHADOW_STACK_STATUS, ~0ULL, 0, 0, 0);
}
} // namespace FEX::GCS

namespace FEX::UnalignedAtomic {
void SetupKernelUnalignedAtomics() {
#ifndef PR_ARM64_SET_UNALIGN_ATOMIC
#define PR_ARM64_SET_UNALIGN_ATOMIC 0x46455849
#define PR_ARM64_UNALIGN_ATOMIC_EMULATE (1UL << 0)
#define PR_ARM64_UNALIGN_ATOMIC_BACKPATCH (1UL << 1)
#define PR_ARM64_UNALIGN_ATOMIC_STRICT_SPLIT_LOCKS (1UL << 2)
#endif

  // Interfaces with downstream FEX kernel patches to control unaligned atomic handling
  FEX_CONFIG_OPT(StrictInProcessSplitLocks, STRICTINPROCESSSPLITLOCKS);
  FEX_CONFIG_OPT(KernelUnalignedAtomicBackpatching, KERNELUNALIGNEDATOMICBACKPATCHING);

  uint64_t Flags = (StrictInProcessSplitLocks() ? PR_ARM64_UNALIGN_ATOMIC_STRICT_SPLIT_LOCKS : 0) |
                   (KernelUnalignedAtomicBackpatching() ? PR_ARM64_UNALIGN_ATOMIC_BACKPATCH : 0) | PR_ARM64_UNALIGN_ATOMIC_EMULATE;

  prctl(PR_ARM64_SET_UNALIGN_ATOMIC, Flags, 0, 0, 0);
}
} // namespace FEX::UnalignedAtomic

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
  const auto PortableInfo = FEX::ReadPortabilityInformation();
  const bool InterpreterInstalled = QueryInterpreterInstalled(ExecutedWithFD, PortableInfo);

  int FEXFD {StealFEXFDFromEnv("FEX_EXECVEFD")};
  int FEXSeccompFD {StealFEXFDFromEnv("FEX_SECCOMPFD")};

  LogMan::Throw::InstallHandler(AssertHandler);
  LogMan::Msg::InstallHandler(MsgHandler);

  auto ArgsLoader = fextl::make_unique<FEX::ArgLoader::ArgLoader>(argc, argv);
  auto Args = ArgsLoader->Get();
  auto ParsedArgs = ArgsLoader->GetParsedArgs();
  auto Program = FEX::Config::GetApplicationNames(Args, ExecutedWithFD, FEXFD);
  if (Program.ProgramPath.empty() && FEXFD == -1) {
    // Early exit if we weren't passed an argument
    return 0;
  }

  FEX::GCS::CheckForGCS();

  FEX::Config::LoadConfig(Program.ProgramName, envp, PortableInfo);

  // Reload the meta layer
  FEXCore::Config::ReloadMetaLayer();
  FEXCore::Config::Set(FEXCore::Config::CONFIG_INTERPRETER_INSTALLED, InterpreterInstalled ? "1" : "0");
#ifdef VIXL_SIMULATOR
  // If running under the vixl simulator, ensure that indirect runtime calls are enabled.
  FEXCore::Config::Set(FEXCore::Config::CONFIG_DISABLE_VIXL_INDIRECT_RUNTIME_CALLS, "0");
#endif

  // Early check for process stall
  // Doesn't use CONFIG_ROOTFS and we don't want it to spin up a squashfs instance
  FEX_CONFIG_OPT(StallProcess, STALLPROCESS);
  FEX_CONFIG_OPT(StartupSleep, STARTUPSLEEP);
  FEX_CONFIG_OPT(StartupSleepProcName, STARTUPSLEEPPROCNAME);
  if (StallProcess) {
    while (1) {
      // Stall this process out forever
      select(0, nullptr, nullptr, nullptr, nullptr);
    }
  }

  // Ensure FEXServer is setup before config options try to pull CONFIG_ROOTFS
  auto SelfPath = FEX::GetSelfPath();
  if (!FEXServerClient::SetupClient(SelfPath.value_or(argv[0]))) {
    LogMan::Msg::EFmt("FEXServerClient: Failure to setup client");
    return -1;
  }

  FEX_CONFIG_OPT(SilentLog, SILENTLOG);
  FEX_CONFIG_OPT(OutputLog, OUTPUTLOG);
  FEX_CONFIG_OPT(LDPath, ROOTFS);
  FEX_CONFIG_OPT(Environment, ENV);
  FEX_CONFIG_OPT(HostEnvironment, HOSTENV);
  ::SilentLog = SilentLog();

  if (::SilentLog) {
    LogMan::Throw::UnInstallHandler();
    LogMan::Msg::UnInstallHandler();
  } else {
    const auto& LogFile = OutputLog();
    // If stderr or stdout then we need to dup the FD
    // In some cases some applications will close stderr and stdout
    // then redirect the FD to either a log OR some cases just not use
    // stderr/stdout and the FD will be reused for regular FD ops.
    //
    // We want to maintain the original output location otherwise we
    // can run in to problems of writing to some file
    auto LogFD = OutputFD;
    if (LogFile == "stderr") {
      LogFD = dup(STDERR_FILENO);
    } else if (LogFile == "stdout") {
      LogFD = dup(STDOUT_FILENO);
    } else if (LogFile == "server") {
      FEXServerLogging::FEXServerFD = FEXServerClient::RequestLogFD(FEXServerClient::GetServerFD());
      if (FEXServerLogging::FEXServerFD != -1) {
        LogMan::Throw::InstallHandler(FEXServerLogging::AssertHandler);
        LogMan::Msg::InstallHandler(FEXServerLogging::MsgHandler);
      }
    } else if (!LogFile.empty()) {
      constexpr int USER_PERMS = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
      LogFD = open(LogFile.c_str(), O_CREAT | O_CLOEXEC | O_WRONLY, USER_PERMS);
    }

    if (LogFD == -1) {
      LogMan::Msg::EFmt("Couldn't open log file. Going Silent.");
      ::SilentLog = true;
    } else {
      OutputFD = LogFD;
    }
  }
  DisableOutputColors = !isatty(OutputFD);

  if (StartupSleep() && (StartupSleepProcName().empty() || Program.ProgramName == StartupSleepProcName())) {
    LogMan::Msg::IFmt("[{}][{}] Sleeping for {} seconds", ::getpid(), Program.ProgramName, StartupSleep());
    std::this_thread::sleep_for(std::chrono::seconds(StartupSleep()));
  }

  FEXCore::Telemetry::Initialize();

  if (!LDPath().empty() && Program.ProgramPath.starts_with(LDPath())) {
    // From this point on, ProgramPath needs to not have the LDPath prefixed on to it.
    auto RootFSLength = LDPath().size();
    if (Program.ProgramPath.at(RootFSLength) != '/') {
      // Ensure the modified path starts as an absolute path.
      // This edge case can occur when ROOTFS ends with '/' and passed a path like `<ROOTFS>usr/bin/true`.
      --RootFSLength;
    }

    Program.ProgramPath.erase(0, RootFSLength);
  }

  bool ProgramExists = InterpreterHandler(&Program.ProgramPath, LDPath(), &Args);

  if (!ExecutedWithFD && FEXFD == -1 && !ProgramExists) {
    // Early exit if the program passed in doesn't exist
    // Will prevent a crash later
    fextl::fmt::print(stderr, "{}: command not found\n", Program.ProgramPath);
    return -ENOEXEC;
  }

  uint32_t KernelVersion = FEX::HLE::SyscallHandler::CalculateHostKernelVersion();
  if (KernelVersion < FEX::HLE::SyscallHandler::KernelVersion(5, 15)) {
    LogMan::Msg::EFmt("FEX requires kernel 5.15 minimum. Expect problems.");
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
    FEXCore::Config::Set(FEXCore::Config::CONFIG_APP_FILENAME, Program.ProgramPath);
    FEXCore::Config::Set(FEXCore::Config::CONFIG_APP_CONFIG_NAME, Program.ProgramName);
  } else if (FEXFD != -1) {
    // Anonymous program.
    FEXCore::Config::Set(FEXCore::Config::CONFIG_APP_FILENAME, "<Anonymous>");
    FEXCore::Config::Set(FEXCore::Config::CONFIG_APP_CONFIG_NAME, "<Anonymous>");
  } else {
    {
      char ExistsTempPath[PATH_MAX];
      char* RealPath = realpath(Program.ProgramPath.c_str(), ExistsTempPath);
      if (RealPath) {
        FEXCore::Config::Set(FEXCore::Config::CONFIG_APP_FILENAME, fextl::string(RealPath));
      }
    }
    FEXCore::Config::Set(FEXCore::Config::CONFIG_APP_CONFIG_NAME, Program.ProgramName);
  }

  // Setup Thread handlers, so FEXCore can create threads.
  auto StackTracker = FEX::LinuxEmulation::Threads::SetupThreadHandlers();

  FEXCore::Config::Set(FEXCore::Config::CONFIG_IS64BIT_MODE, Loader.Is64BitMode() ? "1" : "0");

  fextl::unique_ptr<FEX::HLE::MemAllocator> Allocator;
  fextl::vector<FEXCore::Allocator::MemoryRegion> Base48Bit;
  fextl::vector<FEXCore::Allocator::MemoryRegion> Low4GB;

  if (Loader.Is64BitMode()) {
    // Destroy the 48th bit if it exists
    Base48Bit = FEXCore::Allocator::Setup48BitAllocatorIfExists();
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

  FEXCore::Profiler::Init(Program.ProgramName, Program.ProgramPath);

  bool SupportsAVX {};
  fextl::unique_ptr<FEXCore::Context::Context> CTX;
  {
    auto HostFeatures = FEX::FetchHostFeatures();
    CTX = FEXCore::Context::Context::CreateNewContext(HostFeatures);
    SupportsAVX = HostFeatures.SupportsAVX;
  }

  // Setup TSO hardware emulation immediately after initializing the context.
  FEX::TSO::SetupTSOEmulation(CTX.get());
  FEX::UnalignedAtomic::SetupKernelUnalignedAtomics();

  if (!Loader.Is64BitMode()) {
    // Tell the kernel we want to use the compat input syscalls even though we're
    // a 64 bit process.
    FEX::CompatInput::SetupCompatInput(true);
  } else {
    // Our parent could be an instance running a 32 bit application, so we need
    // to disable compat input if we're running a 64 bit one ourselves.
    FEX::CompatInput::SetupCompatInput(false);
  }

  auto SignalDelegation = FEX::HLE::CreateSignalDelegator(CTX.get(), Program.ProgramName, SupportsAVX);
  auto ThunkHandler = FEX::HLE::CreateThunkHandler();

  auto SyscallHandler = Loader.Is64BitMode() ?
                          FEX::HLE::x64::CreateHandler(CTX.get(), SignalDelegation.get(), ThunkHandler.get()) :
                          FEX::HLE::x32::CreateHandler(CTX.get(), SignalDelegation.get(), ThunkHandler.get(), std::move(Allocator));

  // Load VDSO in to memory prior to mapping our ELFs.
  auto VDSOMapping = FEX::VDSO::LoadVDSOThunks(Loader.Is64BitMode(), SyscallHandler.get());

  // Pass in our VDSO thunks
  ThunkHandler->AppendThunkDefinitions(FEX::VDSO::GetVDSOThunkDefinitions(Loader.Is64BitMode()));
  SignalDelegation->SetVDSOSymbols();

  // Now that we have the syscall handler. Track some FDs that are FEX owned.
  if (OutputFD > 2) {
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
  ThunkHandler->RegisterTLSState(ParentThread);

  SyscallHandler->DeserializeSeccompFD(ParentThread, FEXSeccompFD);

  CTX->ExecuteThread(ParentThread->Thread);

  DebugServer.reset();
  SyscallHandler->TM.Stop();

  auto ProgramStatus = ParentThread->StatusCode;

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

  return ProgramStatus;
}
