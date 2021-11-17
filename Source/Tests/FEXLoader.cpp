/*
$info$
tags: Bin|FEXLoader
desc: Glues the ELF loader, FEXCore and LinuxSyscalls to launch an elf under fex
$end_info$
*/

#include "Common/ArgumentLoader.h"
#include "Common/RootFSSetup.h"
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
#include <cstdint>
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <filesystem>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <sys/auxv.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <system_error>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

#include <sys/sysinfo.h>

namespace {
static bool SilentLog;
static int OutputFD {STDERR_FILENO};
static bool ExecutedWithFD {false};

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
    std::ostringstream Output;
    Output << "[" << CharLevel << "] " << Message << std::endl;
    write(OutputFD, Output.str().c_str(), Output.str().size());
    fsync(OutputFD);
  }
}

void AssertHandler(char const *Message) {
  if (!SilentLog) {
    std::ostringstream Output;
    Output << "[ASSERT] " << Message << std::endl;
    write(OutputFD, Output.str().c_str(), Output.str().size());
    fsync(OutputFD);
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
  return ExecutedWithFD || strstr(Program, "FEXInterpreter") != nullptr;
}

bool IsInterpreterInstalled() {
  // The interpreter is installed if both the binfmt_misc handlers are available
  // Or if we were originally executed with FD. Which means the interpreter is installed

  std::error_code ec{};
  return ExecutedWithFD ||
         (std::filesystem::exists("/proc/sys/fs/binfmt_misc/FEX-x86", ec) &&
         std::filesystem::exists("/proc/sys/fs/binfmt_misc/FEX-x86_64", ec));
}

void AOTGenSection(FEXCore::Context::Context *CTX, ELFCodeLoader2::LoadedSection &Section) {

  // Make sure this section is executable and big enough
  if (!Section.Executable || Section.Size < 16)
    return;

  std::set<uintptr_t> InitialBranchTargets;

  // Load the ELF again with symbol parsing this time
  ELFLoader::ELFContainer container{Section.Filename, "", false};

  // Add symbols to the branch targets list
  container.AddSymbols([&](ELFLoader::ELFSymbol* sym) {
    auto Destination = sym->Address + Section.ElfBase;

    if (! (Destination >= Section.Base && Destination <= (Section.Base + Section.Size)) ) {
      return; // outside of current section, unlikely to be real code
    }

    InitialBranchTargets.insert(Destination);
  });

  LogMan::Msg::I("Symbol seed: %ld", InitialBranchTargets.size());

  // Add unwind entries to the branch target list
  container.AddUnwindEntries([&](uintptr_t Entry) {
    auto Destination = Entry + Section.ElfBase;

    if (! (Destination >= Section.Base && Destination <= (Section.Base + Section.Size)) ) {
      return; // outside of current section, unlikely to be real code
    }

    InitialBranchTargets.insert(Destination);
  });

  LogMan::Msg::I("Symbol + Unwind seed: %ld", InitialBranchTargets.size());

  // Scan the executable section and try to find function entries
  for (size_t Offset = 0; Offset < (Section.Size - 16); Offset++) {
    uint8_t *pCode = (uint8_t *)(Section.Base + Offset);

    // Possible CALL <disp32>
    if (*pCode == 0xE8) {
      uintptr_t Destination = (int)(pCode[1] | (pCode[2] << 8) | (pCode[3] << 16) | (pCode[4] << 24));
      Destination += (uintptr_t)pCode + 5;

      auto DestinationPtr = (uint8_t*)Destination;

      if (! (Destination >= Section.Base && Destination <= (Section.Base + Section.Size)) )
        continue; // outside of current section, unlikely to be real code

      if (DestinationPtr[0] == 0 && DestinationPtr[1] == 0)
        continue; // add al, [rax], unlikely to be real code

      InitialBranchTargets.insert(Destination);
    }

    // endbr64 marker marks an indirect branch destination
    if (pCode[0] == 0xf3 && pCode[1] == 0x0f && pCode[2] == 0x1e && pCode[3] == 0xfa) {
      InitialBranchTargets.insert((uintptr_t)pCode);
    }
  }

  uint64_t SectionMaxAddress = Section.Base + Section.Size;

  std::set<uint64_t> Compiled;
  std::atomic<int> counter = 0;

  std::queue<uint64_t> BranchTargets;

  // Setup BranchTargets, Compiled sets from InitiaBranchTargets

  Compiled.insert(InitialBranchTargets.begin(), InitialBranchTargets.end());
  for (auto BranchTarget: InitialBranchTargets) {
    BranchTargets.push(BranchTarget);
  }

  InitialBranchTargets.clear();


  std::mutex QueueMutex;
  std::vector<std::thread> ThreadPool;

  for (int i = 0; i < get_nprocs_conf(); i++) {
    std::thread thd([&BranchTargets, CTX, &counter, &Compiled, &Section, &QueueMutex, SectionMaxAddress]() {
      // Set the priority of the thread so it doesn't overwhelm the system when running in the background
      setpriority(PRIO_PROCESS, ::gettid(), 19);

      // Setup thread - Each compilation thread uses its own backing FEX thread
      FEXCore::Core::CPUState state;
      auto Thread = FEXCore::Context::CreateThread(CTX, &state, gettid());
      std::set<uint64_t> ExternalBranchesLocal;
      FEXCore::Context::ConfigureAOTGen(Thread, &ExternalBranchesLocal, SectionMaxAddress);


      for (;;) {
        uint64_t BranchTarget;

        // Get a entrypoint to process from the queue
        QueueMutex.lock();
        if (BranchTargets.empty()) {
          QueueMutex.unlock();
          break; // no entrypoint to process - exit
        }

        BranchTarget = BranchTargets.front();
        BranchTargets.pop();
        QueueMutex.unlock();

        // Compile entrypoint
        counter++;
        FEXCore::Context::CompileRIP(Thread, BranchTarget);

        // Are there more branches?
        if (ExternalBranchesLocal.size() > 0) {
          // Add them to the "to process" list
          QueueMutex.lock();
          for(auto Destination: ExternalBranchesLocal) {
              if (! (Destination >= Section.Base && Destination <= (Section.Base + Section.Size)) )
                continue;
              if (Compiled.contains(Destination))
                continue;
              Compiled.insert(Destination);
            BranchTargets.push(Destination);
          }
          QueueMutex.unlock();
          ExternalBranchesLocal.clear();
        }
      }

      // All entryproints processed, cleanup this thread
      FEXCore::Context::DestroyThread(CTX, Thread);
    });

    // Add to the thread pool
    ThreadPool.push_back(std::move(thd));
  }

  // Make sure all threads are finished
  for (auto & Thread: ThreadPool) {
    Thread.join();
  }

  ThreadPool.clear();

  LogMan::Msg::I("\nAll Done: %d", counter.load());
}

int main(int argc, char **argv, char **const envp) {
  bool IsInterpreter = RanAsInterpreter(argv[0]);

  ExecutedWithFD = getauxval(AT_EXECFD) != 0;

  LogMan::Throw::InstallHandler(AssertHandler);
  LogMan::Msg::InstallHandler(MsgHandler);

#if !(defined(ENABLE_ASAN) && ENABLE_ASAN)
  // LLVM ASAN maps things to the lower 32bits
  // Valgrind also places us in the lower 32-bits
  if (!getenv("VALGRIND_LAUNCHER") &&
      !CheckMemMapping()) {
    LogMan::Msg::E("[Unsupported] FEX mapped to lower 32bits! Exiting!");
    return -1;
  }
#endif

  FEXCore::Config::Initialize();
  FEXCore::Config::AddLayer(FEXCore::Config::CreateMainLayer());

  if (IsInterpreter) {
    FEX::ArgLoader::LoadWithoutArguments(argc, argv);
  }
  else {
    FEXCore::Config::AddLayer(std::make_unique<FEX::ArgLoader::ArgLoader>(argc, argv));
  }

  FEXCore::Config::AddLayer(FEXCore::Config::CreateEnvironmentLayer(envp));
  FEXCore::Config::Load();

  auto Args = FEX::ArgLoader::Get();
  auto ParsedArgs = FEX::ArgLoader::GetParsedArgs();

  if (Args.empty()) {
    // Early exit if we weren't passed an argument
    return 0;
  }

  std::string Program = Args[0];

  // These layers load on initialization
  auto ProgramName = std::filesystem::path(Program).filename();
  FEXCore::Config::AddLayer(FEXCore::Config::CreateAppLayer(ProgramName, true));
  FEXCore::Config::AddLayer(FEXCore::Config::CreateAppLayer(ProgramName, false));

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

  // Ensure RootFS is setup before config options try to pull CONFIG_ROOTFS
  if (!FEX::RootFS::Setup(envp)) {
    LogMan::Msg::E("RootFS failure");
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

  if (!::SilentLog) {
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
    else if (!LogFile.empty()) {
      OutputFD = open(LogFile.c_str(), O_CREAT | O_CLOEXEC | O_WRONLY);
    }
  }

  FEXCore::Telemetry::Initialize();
  InterpreterHandler(&Program, LDPath(), &Args);

  std::error_code ec{};
  if (!std::filesystem::exists(Program, ec)) {
    // Early exit if the program passed in doesn't exist
    // Will prevent a crash later
    fprintf(stderr, "%s: command not found\n", Program.c_str());
    return -ENOEXEC;
  }


  uint32_t KernelVersion = FEX::HLE::SyscallHandler::CalculateHostKernelVersion();
  if (KernelVersion < FEX::HLE::SyscallHandler::KernelVersion(4, 17)) {
    // We require 4.17 minimum for MAP_FIXED_NOREPLACE
    LogMan::Msg::E("FEXLoader requires kernel 4.17 minimum. Expect problems.");
  }

  // Before we go any further, set all of our host environment variables that the config has provided
  for (auto &HostEnv : HostEnvironment.All()) {
    // We are going to keep these alive in memory.
    // No need to split the string with setenv
    putenv(HostEnv.data());
  }

  ELFCodeLoader2 Loader{Program, LDPath(), Args, ParsedArgs, envp, &Environment};
  //FEX::HarnessHelper::ELFCodeLoader Loader{Program, LDPath(), Args, ParsedArgs, envp, &Environment};

  if (!Loader.ELFWasLoaded()) {
    // Loader couldn't load this program for some reason
    fprintf(stderr, "Invalid or Unsupported elf file.\n");
#ifdef _M_ARM_64
    fprintf(stderr, "This is likely due to a misconfigured x86-64 RootFS\n");
    fprintf(stderr, "Current RootFS path set to '%s'\n", LDPath().c_str());
    std::error_code ec;
    if (LDPath().size() == 0 ||
        std::filesystem::exists(LDPath(), ec) == false) {
      fprintf(stderr, "RootFS path doesn't exist. This is required on AArch64 hosts\n");
    }
#endif
    return -ENOEXEC;
  }

  FEXCore::Config::Set(FEXCore::Config::CONFIG_APP_FILENAME, std::filesystem::canonical(Program).string());
  FEXCore::Config::Set(FEXCore::Config::CONFIG_IS64BIT_MODE, Loader.Is64BitMode() ? "1" : "0");

  std::unique_ptr<FEX::HLE::MemAllocator> Allocator;
  FEXCore::Allocator::PtrCache *Base48Bit{};

  if (Loader.Is64BitMode()) {
    // Destroy the 48th bit if it exists
    Base48Bit = FEXCore::Allocator::Steal48BitVA();
    if (!Loader.MapMemory([](void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
      return FEXCore::Allocator::mmap(addr, length, prot, flags, fd, offset);
    }, [](void *addr, size_t length) {
      return FEXCore::Allocator::munmap(addr, length);
    })) {
      // failed to map
      LogMan::Msg::E("Failed to map 64-bit elf file.");
      return -ENOEXEC;
    }
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

    if (!Loader.MapMemory([&Allocator](void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
      return Allocator->mmap(addr, length, prot, flags, fd, offset);
    }, [&Allocator](void *addr, size_t length) {
      return Allocator->munmap(addr, length);
    })) {
      // failed to map
      LogMan::Msg::E("Failed to map 32-bit elf file.");
      return -ENOEXEC;
    }
  }

  // System allocator is now system allocator or FEX
  FEXCore::Context::InitializeStaticTables(Loader.Is64BitMode() ? FEXCore::Context::MODE_64BIT : FEXCore::Context::MODE_32BIT);

  auto CTX = FEXCore::Context::CreateNewContext();
  FEXCore::Context::InitializeContext(CTX);

  auto SignalDelegation = std::make_unique<FEX::HLE::SignalDelegator>();
  auto SyscallHandler = Loader.Is64BitMode() ? FEX::HLE::x64::CreateHandler(CTX, SignalDelegation.get())
                                             : FEX::HLE::x32::CreateHandler(CTX, SignalDelegation.get(), std::move(Allocator));

  SyscallHandler->SetCodeLoader(&Loader);

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

  if (AOTIRLoad() || AOTIRCapture() || AOTIRGenerate()) {
    LogMan::Msg::I("Warning: AOTIR is experimental, and might lead to crashes. Capture doesn't work with programs that fork.");
  }

  FEXCore::Context::SetAOTIRLoader(CTX, [](const std::string &fileid) -> int {
    auto filepath = std::filesystem::path(FEXCore::Config::GetDataDirectory()) / "aotir" / (fileid + ".aotir");

    return open(filepath.c_str(), O_RDONLY);
  });

  FEXCore::Context::SetAOTIRWriter(CTX, [](const std::string& fileid) -> std::unique_ptr<std::ostream> {
    auto filepath = std::filesystem::path(FEXCore::Config::GetDataDirectory()) / "aotir" / (fileid + ".aotir");
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

  for(const auto &Section: *Loader.Sections) {
    FEXCore::Context::AddNamedRegion(CTX, Section.Base, Section.Size, Section.Offs, Section.Filename);
  }

  if (AOTIRGenerate()) {
    for(auto &Section: *Loader.Sections) {
      AOTGenSection(CTX, Section);
    }
  } else {
    FEXCore::Context::RunUntilExit(CTX);
  }

  if (std::filesystem::create_directories(std::filesystem::path(FEXCore::Config::GetDataDirectory()) / "aotir", ec)) {
    FEXCore::Context::WriteFilesWithCode(CTX, [](const std::string& fileid, const std::string& filename) {
      auto filepath = std::filesystem::path(FEXCore::Config::GetDataDirectory()) / "aotir" / (fileid + ".path");
      int fd = open(filepath.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0644);
      if (fd != -1) {
        write(fd, filename.c_str(), filename.size());
        close(fd);
      }
    });
  }

  if (AOTIRCapture() || AOTIRGenerate()) {


    FEXCore::Context::FinalizeAOTIRCache(CTX);

    LogMan::Msg::I("AOTIR Cache Stored");
  }

  auto ProgramStatus = FEXCore::Context::GetProgramStatus(CTX);

  SyscallHandler.reset();
  SignalDelegation.reset();
  FEXCore::Context::DestroyContext(CTX);

  FEXCore::Context::ShutdownStaticTables();
  FEXCore::Threads::Shutdown();

  Loader.FreeSections();

  FEX::RootFS::Shutdown();

  FEXCore::Config::Shutdown();

  LogMan::Throw::UnInstallHandlers();
  LogMan::Msg::UnInstallHandlers();

  FEXCore::Allocator::ClearHooks();
  FEXCore::Allocator::ReclaimMemoryRegion(Base48Bit);
  // Allocator is now original system allocator

  FEXCore::Telemetry::Shutdown(ProgramName);
  if (ShutdownReason == FEXCore::Context::ExitReason::EXIT_SHUTDOWN) {
    return ProgramStatus;
  }
  else {
    return -64 | ShutdownReason;
  }
}
