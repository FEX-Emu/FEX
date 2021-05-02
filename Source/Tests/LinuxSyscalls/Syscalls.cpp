/*
$info$
category: LinuxSyscalls ~ Linux syscall emulation, marshaling and passthrough
tags: LinuxSyscalls|common
desc: Glue logic, brk allocations
$end_info$
*/

#include <FEXCore/Utils/LogManager.h>
#include "Common/MathUtils.h"

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"

#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Core/CodeLoader.h>
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/ELFContainer.h>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <limits.h>
#include <linux/futex.h>
#include <poll.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/random.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <sys/shm.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace FEX::HLE {
SyscallHandler *_SyscallHandler{};

static bool IsSupportedByInterpreter(std::string const &Filename) {
  // If it is a supported ELF then we can
  if (ELFLoader::ELFContainer::IsSupportedELF(Filename.c_str())) {
    return true;
  }

  // If it is a shebang then we also can
  std::fstream File;
  size_t FileSize{0};
  File.open(Filename, std::fstream::in | std::fstream::binary);

  if (!File.is_open())
    return false;

  File.seekg(0, File.end);
  FileSize = File.tellg();
  File.seekg(0, File.beg);

  // Is the file large enough for shebang
  if (FileSize <= 2)
    return false;

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
      std::string RootFS = FEX::HLE::_SyscallHandler->RootFSPath();
      ShebangProgram = RootFS + ShebangProgram;
    }

    std::error_code ec;
    bool exists = std::filesystem::exists(ShebangProgram, ec);
    if (ec || !exists) {
      return false;
    }
    return true;
  }

  return false;
}

uint64_t ExecveHandler(const char *pathname, std::vector<const char*> &argv, std::vector<const char*> &envp) {
  std::string Filename{};

  std::error_code ec;
  std::string RootFS = FEX::HLE::_SyscallHandler->RootFSPath();
  // Check the rootfs if it is available first
  if (pathname[0] == '/') {
    Filename = RootFS  + pathname;

    bool exists = std::filesystem::exists(Filename, ec);
    if (ec || !exists) {
      Filename = pathname;
    }
  }
  else {
    Filename = pathname;
  }

  bool exists = std::filesystem::exists(Filename, ec);
  if (ec || !exists) {
    return -ENOENT;
  }

  uint64_t Result{};
  if (FEX::HLE::_SyscallHandler->IsInterpreterInstalled()) {
    // If the FEX interpreter is installed then just execve the thing
    Result = execve(Filename.c_str(), const_cast<char *const *>(&argv.at(0)), const_cast<char *const *>(&envp.at(0)));
    SYSCALL_ERRNO();
  }

  // If we don't have the interpreter installed we need to be extra careful for ENOEXEC
  // Reasoning is that if we try executing a file from FEXLoader then this process loses the ENOEXEC flag
  // Kernel does its own checks for file format support for this
  ELFLoader::ELFContainer::ELFType Type = ELFLoader::ELFContainer::GetELFType(Filename);
  if (!IsSupportedByInterpreter(Filename) && Type == ELFLoader::ELFContainer::ELFType::TYPE_NONE) {
    // If our interpeter doesn't support this file format AND ELF format is NONE then ENOEXEC
    // binfmt_misc could end up handling this case but we can't know that without parsing binfmt_misc ourselves
    // Return -ENOEXEC until proven otherwise
    return -ENOEXEC;
  }

  if (Type == ELFLoader::ELFContainer::ELFType::TYPE_OTHER_ELF) {
    // We are trying to execute an ELF of a different architecture
    // We can't know if we can support this without architecture specific checks and binfmt_misc parsing
    // Just execve it and let the kernel handle the process
    Result = execve(Filename.c_str(), const_cast<char *const *>(&argv.at(0)), const_cast<char *const *>(&envp.at(0)));
    SYSCALL_ERRNO();
  }

  // We don't have an interpreter installed
  // We now need to munge the arguments
  std::vector<const char *> ExecveArgs{};
  FEX::HLE::_SyscallHandler->GetCodeLoader()->GetExecveArguments(&ExecveArgs);
  if (!FEX::HLE::_SyscallHandler->IsInterpreter()) {
    // If we were launched from FEXLoader then we need to make sure to split arguments from FEXLoader and guest
    ExecveArgs.emplace_back("--");
  }

  // Overwrite the filename with the new one we are redirecting to
  argv[0] = Filename.c_str();

  // Append the arguments together
  ExecveArgs.insert(ExecveArgs.end(), argv.begin(), argv.end());

  Result = execve("/proc/self/exe", const_cast<char *const *>(&ExecveArgs.at(0)), const_cast<char *const *>(&envp.at(0)));

  SYSCALL_ERRNO();
}

uint64_t SyscallHandler::HandleBRK(FEXCore::Core::CpuStateFrame *Frame, void *Addr) {
  std::lock_guard<std::mutex> lk(MMapMutex);

  uint64_t Result;

  if (Addr == nullptr) { // Just wants to get the location of the program break atm
    Result = DataSpace + DataSpaceSize;
  }
  else {
    // Allocating out data space
    uint64_t NewEnd = reinterpret_cast<uint64_t>(Addr);
    if (NewEnd < DataSpace) {
      // Not allowed to move brk end below original start
      // Set the size to zero
      DataSpaceSize = 0;
    }
    else {
      uint64_t NewSize = NewEnd - DataSpace;
      uint64_t NewSizeAligned = AlignUp(NewSize, 4096);

      if (NewSizeAligned < DataSpaceMaxSize) {
        // If we are shrinking the brk then munmap the ranges
        // That way we gain the memory back and also give the application zero pages if it allocates again
        // DataspaceMaxSize is always page aligned

        uint64_t RemainingSize = DataSpaceMaxSize - NewSizeAligned;
        // We have pages we can unmap
        FEXCore::Allocator::munmap(reinterpret_cast<void*>(DataSpace + NewSizeAligned), RemainingSize);
        DataSpaceMaxSize = NewSizeAligned;
      }
      else if (NewSize > DataSpaceMaxSize) {
        constexpr static uint64_t SizeAlignment = 8 * 1024 * 1024;
        uint64_t AllocateNewSize = AlignUp(NewSize, SizeAlignment) - DataSpaceMaxSize;
        if (!Is64BitMode() &&
          (DataSpace + DataSpaceMaxSize + AllocateNewSize > 0x1'0000'0000ULL)) {
          // If we are 32bit and we tried going about the 32bit limit then out of memory
          return DataSpace + DataSpaceSize;
        }

        uint64_t NewBRK = (uint64_t)FEXCore::Allocator::mmap((void*)(DataSpace + DataSpaceMaxSize), AllocateNewSize, PROT_READ | PROT_WRITE, MAP_FIXED_NOREPLACE | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if (NewBRK != ~0ULL && NewBRK != (DataSpace + DataSpaceMaxSize)) {
          // Couldn't allocate that the region we wanted
          // Can happen if MAP_FIXED_NOREPLACE isn't understood by the kernel
          FEXCore::Allocator::munmap(reinterpret_cast<void*>(NewBRK), AllocateNewSize);
          NewBRK = ~0ULL;
        }

        if (NewBRK == ~0ULL) {
          // If we couldn't allocate a new region then out of memory
          return DataSpace + DataSpaceSize;
        }
        else {
          // Increase our BRK size
          DataSpaceMaxSize += AllocateNewSize;
        }
      }

      DataSpaceSize = NewSize;
    }
    Result = DataSpace + DataSpaceSize;
  }
  return Result;
}

void SyscallHandler::DefaultProgramBreak(uint64_t Base, uint64_t Size) {
  DataSpace = Base;
  DataSpaceMaxSize = Size;
  DataSpaceStartingSize = Size;
}

SyscallHandler::SyscallHandler(FEXCore::Context::Context *ctx, FEX::HLE::SignalDelegator *_SignalDelegation)
  : FM {ctx}
  , SignalDelegation {_SignalDelegation} {
  FEX::HLE::_SyscallHandler = this;
  HostKernelVersion = CalculateHostKernelVersion();
}

SyscallHandler::~SyscallHandler() {
  FEXCore::Allocator::munmap(reinterpret_cast<void*>(DataSpace + DataSpaceStartingSize), DataSpaceMaxSize - DataSpaceStartingSize);
}

uint32_t SyscallHandler::CalculateHostKernelVersion() {
  struct utsname buf{};
  if (uname(&buf) == -1) {
    return 0;
  }

  int32_t Major{};
  int32_t Minor{};
  int32_t Patch{};
  char Tmp{};
  std::istringstream ss{buf.release};
  ss >> Major;
  ss.read(&Tmp, 1);
  ss >> Minor;
  ss.read(&Tmp, 1);
  ss >> Patch;
  return (Major << 24) | (Minor << 16) | Patch;
}

uint64_t SyscallHandler::HandleSyscall(FEXCore::Core::CpuStateFrame *Frame, FEXCore::HLE::SyscallArguments *Args) {
  auto &Def = Definitions[Args->Argument[0]];
  uint64_t Result{};
  switch (Def.NumArgs) {
  case 0: Result = std::invoke(Def.Ptr0, Frame); break;
  case 1: Result = std::invoke(Def.Ptr1, Frame, Args->Argument[1]); break;
  case 2: Result = std::invoke(Def.Ptr2, Frame, Args->Argument[1], Args->Argument[2]); break;
  case 3: Result = std::invoke(Def.Ptr3, Frame, Args->Argument[1], Args->Argument[2], Args->Argument[3]); break;
  case 4: Result = std::invoke(Def.Ptr4, Frame, Args->Argument[1], Args->Argument[2], Args->Argument[3], Args->Argument[4]); break;
  case 5: Result = std::invoke(Def.Ptr5, Frame, Args->Argument[1], Args->Argument[2], Args->Argument[3], Args->Argument[4], Args->Argument[5]); break;
  case 6: Result = std::invoke(Def.Ptr6, Frame, Args->Argument[1], Args->Argument[2], Args->Argument[3], Args->Argument[4], Args->Argument[5], Args->Argument[6]); break;
  // for missing syscalls
  case 255: return std::invoke(Def.Ptr1, Frame, Args->Argument[0]);
  default:
    LOGMAN_MSG_A("Unhandled syscall: %d", Args->Argument[0]);
    return -1;
  break;
  }
#ifdef DEBUG_STRACE
  Strace(Args, Result);
#endif
  return Result;
}

#ifdef DEBUG_STRACE
void SyscallHandler::Strace(FEXCore::HLE::SyscallArguments *Args, uint64_t Ret) {
  auto &Def = Definitions[Args->Argument[0]];
  switch (Def.NumArgs) {
    case 0: LogMan::Msg::D(Def.StraceFmt.c_str(), Ret); break;
    case 1: LogMan::Msg::D(Def.StraceFmt.c_str(), Args->Argument[1], Ret); break;
    case 2: LogMan::Msg::D(Def.StraceFmt.c_str(), Args->Argument[1], Args->Argument[2], Ret); break;
    case 3: LogMan::Msg::D(Def.StraceFmt.c_str(), Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret); break;
    case 4: LogMan::Msg::D(Def.StraceFmt.c_str(), Args->Argument[1], Args->Argument[2], Args->Argument[3], Args->Argument[4], Ret); break;
    case 5: LogMan::Msg::D(Def.StraceFmt.c_str(), Args->Argument[1], Args->Argument[2], Args->Argument[3], Args->Argument[4], Args->Argument[5], Ret); break;
    case 6: LogMan::Msg::D(Def.StraceFmt.c_str(), Args->Argument[1], Args->Argument[2], Args->Argument[3], Args->Argument[4], Args->Argument[5], Args->Argument[6], Ret); break;
    default: break;
  }
}
#endif

uint64_t UnimplementedSyscall(FEXCore::Core::CpuStateFrame *Frame, uint64_t SyscallNumber) {
  ERROR_AND_DIE("Unhandled system call: %d", SyscallNumber);
  return -ENOSYS;
}

uint64_t UnimplementedSyscallSafe(FEXCore::Core::CpuStateFrame *Frame, uint64_t SyscallNumber) {
  return -ENOSYS;
}

}
