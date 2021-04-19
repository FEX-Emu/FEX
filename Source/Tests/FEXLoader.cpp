/*
$info$
tags: Bin|FEXLoader
desc: Glues the ELF loader, FEXCore and LinuxSyscalls to launch an elf under fex
$end_info$
*/

#include "Common/ArgumentLoader.h"
#include "Common/EnvironmentLoader.h"
#include "Common/Config.h"
#include "ELFCodeLoader.h"
#include "ELFCodeLoader2.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include "Tests/LinuxSyscalls/SignalDelegator.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CodeLoader.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/ELFContainer.h>
#include <FEXCore/Utils/LogManager.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <unistd.h>
#include <vector>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <set>

extern uint64_t SectionMaxAddress;
extern std::set<uint64_t> ExternalBranches;

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

  FEX_CONFIG_OPT(SilentLog, SILENTLOG);
  FEX_CONFIG_OPT(AOTIRCapture, AOTIRCAPTURE);
  FEX_CONFIG_OPT(AOTIRGenerate, AOTIRGENERATE);
  FEX_CONFIG_OPT(AOTIRLoad, AOTIRLOAD);
  FEX_CONFIG_OPT(OutputLog, OUTPUTLOG);
  FEX_CONFIG_OPT(LDPath, ROOTFS);
  FEX_CONFIG_OPT(Environment, ENV);
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
    return -ENOEXEC;
  }


  uint32_t KernelVersion = FEX::HLE::SyscallHandler::CalculateHostKernelVersion();
  if (KernelVersion < FEX::HLE::SyscallHandler::KernelVersion(4, 17)) {
    // We require 4.17 minimum for MAP_FIXED_NOREPLACE
    LogMan::Msg::E("FEXLoader requires kernel 4.17 minimum. Expect problems.");
  }

  ELFCodeLoader2 Loader{Program, LDPath(), Args, ParsedArgs, envp, &Environment};
  //FEX::HarnessHelper::ELFCodeLoader Loader{Program, LDPath(), Args, ParsedArgs, envp, &Environment};

  if (!Loader.ELFWasLoaded()) {
    // Loader couldn't load this program for some reason
    LogMan::Msg::E("Invalid or Unsupported elf file.");
    return -ENOEXEC;
  }

  FEXCore::Config::Set(FEXCore::Config::CONFIG_APP_FILENAME, std::filesystem::canonical(Program));
  FEXCore::Config::Set(FEXCore::Config::CONFIG_IS64BIT_MODE, Loader.Is64BitMode() ? "1" : "0");


  FEX::HLE::x32::MemAllocator *Allocator = nullptr;

  if (Loader.Is64BitMode()) {
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
    Allocator = new FEX::HLE::x32::MemAllocator();
    if (!Loader.MapMemory([Allocator](void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
      return Allocator->mmap(addr, length, prot, flags, fd, offset);
    }, [Allocator](void *addr, size_t length) {
      return Allocator->munmap(addr, length);
    })) {
      // failed to map
      LogMan::Msg::E("Failed to map 32-bit elf file.");
      return -ENOEXEC;
    }
  }

  FEXCore::Context::InitializeStaticTables(Loader.Is64BitMode() ? FEXCore::Context::MODE_64BIT : FEXCore::Context::MODE_32BIT);

  auto CTX = FEXCore::Context::CreateNewContext();
  FEXCore::Context::InitializeContext(CTX);

  std::unique_ptr<FEX::HLE::SignalDelegator> SignalDelegation = std::make_unique<FEX::HLE::SignalDelegator>();

  std::unique_ptr<FEX::HLE::SyscallHandler> SyscallHandler{
    Loader.Is64BitMode() ?
      FEX::HLE::x64::CreateHandler(CTX, SignalDelegation.get()) :
      FEX::HLE::x32::CreateHandler(CTX, SignalDelegation.get(), Allocator)
  };

  SyscallHandler->SetCodeLoader(&Loader);

  auto BRKInfo = Loader.GetBRKInfo();
  //fprintf(stderr, "BRK %lX - %ld\n", BRKInfo.Base, BRKInfo.Size);

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
    auto filepath = std::filesystem::path(FEXCore::Config::GetDataDirectory()) / "aotir" / fileid;

    return open(filepath.c_str(), O_RDONLY);
  });

  for(auto Section: Loader.Sections) {
    FEXCore::Context::AddNamedRegion(CTX, Section.Base, Section.Size, Section.Offs, Section.Filename);
  }

  if (AOTIRGenerate()) {
    for(auto Section: Loader.Sections) {
      if (Section.Executable && Section.Size > 16) {
        ELFLoader::ELFContainer container{Section.Filename, "", false};

        std::set<uintptr_t> BranchTargets;

        container.AddSymbols([&](ELFLoader::ELFSymbol* sym) {
          auto Destination = sym->Address + Section.ElfBase;

          if (! (Destination >= Section.Base && Destination <= (Section.Base + Section.Size)) ) {
            //printf("Sym : %lx %lx out of range\n", sym->Address, Destination);
            return; // outside of current section, unlikely to be real code
          }

          BranchTargets.insert(Destination);
        });


        printf("Symbol + Unwind seed: %ld\n", BranchTargets.size());

        for (size_t Offset = 0; Offset < (Section.Size - 16); Offset++) {
          uint8_t *pCode = (uint8_t *)(Section.Base + Offset);

          if (*pCode == 0xE8) {
            uintptr_t Destination = (int)(pCode[1] | (pCode[2] << 8) | (pCode[3] << 16) | (pCode[4] << 24));
            Destination += (uintptr_t)pCode + 5;

            auto DestinationPtr = (uint8_t*)Destination;
            
            if (! (Destination >= Section.Base && Destination <= (Section.Base + Section.Size)) )
              continue; // outside of current section, unlikely to be real code

            if (DestinationPtr[0] == 0 && DestinationPtr[1] == 0)
              continue; // add al, [rax], unlikely to be real code
/*
            if (DestinationPtr[0] == 0x44 && DestinationPtr[1] == 0x0f && DestinationPtr[2] == 0x6f)
              continue; // REX.W + movq leads to frontend bugs
*/
            BranchTargets.insert(Destination);
          }

          if (pCode[0] == 0xf3 && pCode[1] == 0x0f && pCode[2] == 0x1e && pCode[3] == 0xfa) {
            BranchTargets.insert((uintptr_t)pCode);
          }
        }

        SectionMaxAddress = Section.Base + Section.Size;

        std::set<uint64_t> Compiled;
        int counter = 0;
        do {        
          fprintf(stderr, "Discovered %ld Branch Targets in this pass\n", BranchTargets.size());
          for (auto RIP: BranchTargets) {
            if ((counter++) % 1000 == 0)
              fprintf(stderr, "\rCompiling %d %lX ", counter, RIP - Section.ElfBase);
            FEXCore::Context::CompileRIP(CTX, RIP);
            Compiled.insert(RIP);
          }
          fprintf(stderr, "\nPass Done \n");
          BranchTargets.clear();
          for (auto Destination: ExternalBranches) {
            if (! (Destination >= Section.Base && Destination <= (Section.Base + Section.Size)) )
              continue;
            if (Compiled.contains(Destination))
              continue;
            BranchTargets.insert(Destination);
          }
          ExternalBranches.clear();
        } while (BranchTargets.size() > 0);
        fprintf(stderr, "\nAll Done: %d\n", counter);
      }
    }
  } else {
    FEXCore::Context::RunUntilExit(CTX);
  }

  if (AOTIRCapture() || AOTIRGenerate()) {
    std::filesystem::create_directories(std::filesystem::path(FEXCore::Config::GetDataDirectory()) / "aotir");

    auto WroteCache = FEXCore::Context::WriteAOTIR(CTX, [](const std::string& fileid) -> std::unique_ptr<std::ostream> {
      auto filepath = std::filesystem::path(FEXCore::Config::GetDataDirectory()) / "aotir" / fileid;
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
