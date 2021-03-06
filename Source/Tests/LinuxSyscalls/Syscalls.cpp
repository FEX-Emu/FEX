/*
$info$
category: LinuxSyscalls ~ Linux syscall emulation, marshaling and passthrough
tags: LinuxSyscalls|common
desc: Glue logic, brk allocations
$end_info$
*/

#include <FEXCore/Utils/LogManager.h>
#include "Common/MathUtils.h"
#include "Linux/Utils/ELFContainer.h"

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/Syscalls/Thread.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"

#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Core/CodeLoader.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/Utils/Allocator.h>
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

uint64_t ExecveHandler(const char *pathname, std::vector<const char*> &argv, std::vector<const char*> &envp, ExecveAtArgs *Args) {
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

  int pid = getpid();

  char PidSelfPath[50];
  snprintf(PidSelfPath, 50, "/proc/%i/exe", pid);

  if (strcmp(pathname, "/proc/self/exe") == 0 ||
      strcmp(pathname, "/proc/thread-self/exe") == 0 ||
      strcmp(pathname, PidSelfPath) == 0) {
    // If pointing to self then redirect to the application
    // JRE and shapez.io does this
    Filename = FEX::HLE::_SyscallHandler->Filename();
  }

  uint64_t Result{};
  if (FEX::HLE::_SyscallHandler->IsInterpreterInstalled()) {
    // If the FEX interpreter is installed then just execve the thing
    if (Args) {
      Result = ::syscall(SYS_execveat, Args->dirfd, Filename.c_str(), const_cast<char *const *>(&argv.at(0)), const_cast<char *const *>(&envp.at(0)), Args->flags);
    }
    else {
      Result = execve(Filename.c_str(), const_cast<char *const *>(&argv.at(0)), const_cast<char *const *>(&envp.at(0)));
    }
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
    if (Args) {
      Result = ::syscall(SYS_execveat, Args->dirfd, Filename.c_str(), const_cast<char *const *>(&argv.at(0)), const_cast<char *const *>(&envp.at(0)), Args->flags);
    }
    else {
      Result = execve(Filename.c_str(), const_cast<char *const *>(&argv.at(0)), const_cast<char *const *>(&envp.at(0)));
    }
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

  if (Args) {
    Result = ::syscall(SYS_execveat, Args->dirfd, "/proc/self/exe", const_cast<char *const *>(&ExecveArgs.at(0)), const_cast<char *const *>(&envp.at(0)), Args->flags);
  }
  else {
    Result = execve("/proc/self/exe", const_cast<char *const *>(&ExecveArgs.at(0)), const_cast<char *const *>(&envp.at(0)));
  }

  SYSCALL_ERRNO();
}

static bool AnyFlagsSet(uint64_t Flags, uint64_t Mask) {
  return (Flags & Mask) != 0;
}

static bool AllFlagsSet(uint64_t Flags, uint64_t Mask) {
  return (Flags & Mask) == Mask;
}

uint64_t CloneHandler(FEXCore::Core::CpuStateFrame *Frame, FEX::HLE::clone3_args *args) {
  uint64_t flags = args->flags;
  auto PrintFlags = [](uint64_t Flags) -> void {
#define FLAGPRINT(x, y) if (Flags & (y)) LogMan::Msg::I("\tFlag: " #x)
    FLAGPRINT(CSIGNAL,              0x000000FF);
    FLAGPRINT(CLONE_VM,             0x00000100);
    FLAGPRINT(CLONE_FS,             0x00000200);
    FLAGPRINT(CLONE_FILES,          0x00000400);
    FLAGPRINT(CLONE_SIGHAND,        0x00000800);
    FLAGPRINT(CLONE_PTRACE,         0x00002000);
    FLAGPRINT(CLONE_VFORK,          0x00004000);
    FLAGPRINT(CLONE_PARENT,         0x00008000);
    FLAGPRINT(CLONE_THREAD,         0x00010000);
    FLAGPRINT(CLONE_NEWNS,          0x00020000);
    FLAGPRINT(CLONE_SYSVSEM,        0x00040000);
    FLAGPRINT(CLONE_SETTLS,         0x00080000);
    FLAGPRINT(CLONE_PARENT_SETTID,  0x00100000);
    FLAGPRINT(CLONE_CHILD_CLEARTID, 0x00200000);
    FLAGPRINT(CLONE_DETACHED,       0x00400000);
    FLAGPRINT(CLONE_UNTRACED,       0x00800000);
    FLAGPRINT(CLONE_CHILD_SETTID,   0x01000000);
    FLAGPRINT(CLONE_NEWCGROUP,      0x02000000);
    FLAGPRINT(CLONE_NEWUTS,         0x04000000);
    FLAGPRINT(CLONE_NEWIPC,         0x08000000);
    FLAGPRINT(CLONE_NEWUSER,        0x10000000);
    FLAGPRINT(CLONE_NEWPID,         0x20000000);
    FLAGPRINT(CLONE_NEWNET,         0x40000000);
    FLAGPRINT(CLONE_IO,             0x80000000);
#undef FLAGPRINT
  };

  auto Thread = Frame->Thread;

  if (AnyFlagsSet(flags, CLONE_UNTRACED | CLONE_PTRACE)) {
    PrintFlags(flags);
    LogMan::Msg::D("clone: Ptrace* not supported");
  }

  // Clone3 flags
#ifndef CLONE_CLEAR_SIGHAND
#define CLONE_CLEAR_SIGHAND 0x100000000ULL
#endif
#ifndef CLONE_INTO_CGROUP
#define CLONE_INTO_CGROUP 0x200000000ULL
#endif

  if (AnyFlagsSet(flags, CLONE_CLEAR_SIGHAND)) {
    PrintFlags(flags);
    LogMan::Msg::D("clone3: CLONE_CLEAR_SIGHAND unsupported");
  }

  if (AnyFlagsSet(flags, CLONE_INTO_CGROUP)) {
    PrintFlags(flags);
    LogMan::Msg::D("clone3: CLONE_INTO_CGROUP unsupported");
    return -EOPNOTSUPP;
  }

  if (args->set_tid_size > 0) {
    LogMan::Msg::D("clone3: set_tid unsupported");
    return -EPERM;
  }

  if (AnyFlagsSet(flags, CLONE_NEWNS | CLONE_NEWCGROUP | CLONE_NEWUTS | CLONE_NEWIPC | CLONE_NEWUSER | CLONE_NEWPID | CLONE_NEWNET)) {
    // NEWUSER doesn't need any privileges from 3.8 onward
    // We just don't support it yet
    PrintFlags(flags);
    LogMan::Msg::I("Unconditionally returning EPERM on clone namespace");
    return -EPERM;
  }

  if (!(flags & CLONE_THREAD)) {

    if (flags & CLONE_VFORK) {
      PrintFlags(flags);
      flags &= ~CLONE_VFORK;
      flags &= ~CLONE_VM;
      LogMan::Msg::D("clone: WARNING: CLONE_VFORK w/o CLONE_THREAD");
    }

    if (AnyFlagsSet(flags, CLONE_SYSVSEM | CLONE_FS |  CLONE_FILES | CLONE_SIGHAND | CLONE_VM)) {
      PrintFlags(flags);
      LogMan::Msg::I("clone: Unsuported flags w/o CLONE_THREAD (Shared Resources), %X", flags);
      return -EPERM;
    }

    // CLONE_PARENT is ignored (Implied by CLONE_THREAD)
    return FEX::HLE::ForkGuest(Thread, Frame, flags,
      reinterpret_cast<void*>(args->stack),
      reinterpret_cast<pid_t*>(args->parent_tid),
      reinterpret_cast<pid_t*>(args->child_tid),
      reinterpret_cast<void*>(args->tls));
  } else {
    if (!AllFlagsSet(flags, CLONE_SYSVSEM | CLONE_FS |  CLONE_FILES | CLONE_SIGHAND)) {
      PrintFlags(flags);
      LogMan::Msg::I("clone: CLONE_THREAD: Unsuported flags w/ CLONE_THREAD (Shared Resources), %X", flags);
      return -EPERM;
    }

    auto NewThread = FEX::HLE::CreateNewThread(Thread->CTX, Frame, args);

    // Return the new threads TID
    uint64_t Result = NewThread->ThreadManager.GetTID();

    if (flags & CLONE_VFORK) {
      NewThread->DestroyedByParent = true;
    }

    // Actually start the thread
    FEXCore::Context::RunThread(Thread->CTX, NewThread);

    if (flags & CLONE_VFORK) {
      // If VFORK is set then the calling process is suspended until the thread exits with execve or exit
      NewThread->ExecutionThread->join(nullptr);

      // Normally a thread cleans itself up on exit. But because we need to join, we are now responsible
      FEXCore::Context::DestroyThread(Thread->CTX, NewThread);
    }
    SYSCALL_ERRNO();
  }
};

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

        uint64_t NewBRK{};
        if (Is64BitMode()) {
          NewBRK = (uint64_t)FEXCore::Allocator::mmap((void*)(DataSpace + DataSpaceMaxSize), AllocateNewSize, PROT_READ | PROT_WRITE, MAP_FIXED_NOREPLACE | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        }
        else {
          NewBRK = (uint64_t)static_cast<FEX::HLE::x32::x32SyscallHandler*>(FEX::HLE::_SyscallHandler)->GetAllocator()->
            mmap((void*)(DataSpace + DataSpaceMaxSize), AllocateNewSize, PROT_READ | PROT_WRITE, MAP_FIXED_NOREPLACE | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        }

        if (NewBRK != ~0ULL && NewBRK != (DataSpace + DataSpaceMaxSize)) {
          // Couldn't allocate that the region we wanted
          // Can happen if MAP_FIXED_NOREPLACE isn't understood by the kernel
          if (Is64BitMode()) {
            FEXCore::Allocator::munmap(reinterpret_cast<void*>(NewBRK), AllocateNewSize);
          }
          else {
            static_cast<FEX::HLE::x32::x32SyscallHandler*>(FEX::HLE::_SyscallHandler)->GetAllocator()->
              munmap(reinterpret_cast<void*>(NewBRK), AllocateNewSize);
          }
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
  GuestKernelVersion = CalculateGuestKernelVersion();

}

SyscallHandler::~SyscallHandler() {
  FEXCore::Allocator::munmap(reinterpret_cast<void*>(DataSpace), DataSpaceMaxSize);
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

uint32_t SyscallHandler::CalculateGuestKernelVersion() {
  // We currently only emulate a kernel between the ranges of Kernel 5.0.0 and 5.12.0
  return std::max(KernelVersion(5, 0), std::min(KernelVersion(5, 12), GetHostKernelVersion()));
}

uint64_t SyscallHandler::HandleSyscall(FEXCore::Core::CpuStateFrame *Frame, FEXCore::HLE::SyscallArguments *Args) {
  if (Args->Argument[0] >= Definitions.size()) {
    return -ENOSYS;
  }

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
