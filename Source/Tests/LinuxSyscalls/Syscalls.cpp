/*
$info$
category: LinuxSyscalls ~ Linux syscall emulation, marshaling and passthrough
tags: LinuxSyscalls|common
desc: Glue logic, brk allocations
$end_info$
*/

#include "Linux/Utils/ELFContainer.h"

#include "Tests/LinuxSyscalls/LinuxAllocator.h"
#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/Syscalls/Thread.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/CodeLoader.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/HLE/Linux/ThreadManagement.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/MathUtils.h>
#include <FEXCore/Utils/Threads.h>
#include <FEXHeaderUtils/Syscalls.h>

#include <algorithm>
#include <alloca.h>
#include <functional>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <system_error>
#include <syscall.h>
#include <sys/mman.h>
#include <sys/utsname.h>
#include <unistd.h>

namespace FEXCore::Context {
  struct Context;
}

namespace FEX::HLE {
class SignalDelegator;
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

uint64_t ExecveHandler(const char *pathname, char* const* argv, char* const* envp, ExecveAtArgs *Args) {
  std::string Filename{};

  std::error_code ec;
  std::string RootFS = FEX::HLE::_SyscallHandler->RootFSPath();

  // Check the rootfs if it is available first
  if (pathname[0] == '/') {
    auto Path = FEX::HLE::_SyscallHandler->FM.GetEmulatedPath(pathname, true);
    if (!Path.empty() && std::filesystem::exists(Path, ec)) {
      Filename = Path;
    }
    else {
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

  // If we don't have the interpreter installed we need to be extra careful for ENOEXEC
  // Reasoning is that if we try executing a file from FEXLoader then this process loses the ENOEXEC flag
  // Kernel does its own checks for file format support for this
  // We can only call execve directly if we both have an interpreter installed AND were ran with the interpreter
  // If the user ran FEX through FEXLoader then we must go down the emulated path
  ELFLoader::ELFContainer::ELFType Type = ELFLoader::ELFContainer::GetELFType(Filename);
  uint64_t Result{};
  if (FEX::HLE::_SyscallHandler->IsInterpreterInstalled() &&
      FEX::HLE::_SyscallHandler->IsInterpreter() &&
      (Type == ELFLoader::ELFContainer::ELFType::TYPE_X86_32 ||
       Type == ELFLoader::ELFContainer::ELFType::TYPE_X86_64)) {
    // If the FEX interpreter is installed then just execve the ELF file
    // This will stay inside of our emulated environment since binfmt_misc will capture it
    if (Args) {
      Result = ::syscall(SYS_execveat, Args->dirfd, Filename.c_str(), argv, envp, Args->flags);
    }
    else {
      Result = execve(Filename.c_str(), argv, envp);
    }
    SYSCALL_ERRNO();
  }

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
      Result = ::syscall(SYS_execveat, Args->dirfd, Filename.c_str(), argv, envp, Args->flags);
    }
    else {
      Result = execve(Filename.c_str(), argv, envp);
    }
    SYSCALL_ERRNO();
  }

  // We don't have an interpreter installed or we are executing a non-ELF executable
  // We now need to munge the arguments
  std::vector<const char *> ExecveArgs{};
  FEX::HLE::_SyscallHandler->GetCodeLoader()->GetExecveArguments(&ExecveArgs);
  if (!FEX::HLE::_SyscallHandler->IsInterpreter()) {
    // If we were launched from FEXLoader then we need to make sure to split arguments from FEXLoader and guest
    ExecveArgs.emplace_back("--");
  }

  if (argv) {
    // Overwrite the filename with the new one we are redirecting to
    ExecveArgs.emplace_back(Filename.c_str());

    auto OldArgv = argv;

    // Skip filename argument
    ++OldArgv;
    while (*OldArgv) {
      // Append the arguments together
      ExecveArgs.emplace_back(*OldArgv);
      ++OldArgv;
    }

    // Emplace nullptr at the end to stop
    ExecveArgs.emplace_back(nullptr);
  }

  if (Args) {
    Result = ::syscall(SYS_execveat, Args->dirfd, "/proc/self/exe",
                       const_cast<char *const *>(ExecveArgs.data()), envp, Args->flags);
  }
  else {
    Result = execve("/proc/self/exe", const_cast<char *const *>(ExecveArgs.data()), envp);
  }

  SYSCALL_ERRNO();
}

static bool AnyFlagsSet(uint64_t Flags, uint64_t Mask) {
  return (Flags & Mask) != 0;
}

static bool AllFlagsSet(uint64_t Flags, uint64_t Mask) {
  return (Flags & Mask) == Mask;
}

struct StackFrameData {
  FEXCore::Core::InternalThreadState *Thread{};
  FEXCore::Context::Context *CTX{};
  FEXCore::Core::CpuStateFrame NewFrame{};
  FEX::HLE::kernel_clone3_args GuestArgs{};
  void *NewStack;
  size_t StackSize;
};

struct StackFramePlusRet {
  uint64_t Ret;
  StackFrameData Data;
  uint64_t Pad;
};

[[noreturn]]
static void Clone3HandlerRet() {
  StackFrameData *Data = (StackFrameData*)alloca(0);
  uint64_t Result = FEX::HLE::HandleNewClone(Data->Thread, Data->CTX, &Data->NewFrame, &Data->GuestArgs);
  FEXCore::Threads::DeallocateStackObject(Data->NewStack, Data->StackSize);
  // To behave like a real clone, we now just need to call exit here
  exit(Result);
  FEX_UNREACHABLE;
}

static int Clone2HandlerRet(void *arg) {
  StackFrameData *Data = (StackFrameData*)arg;
  uint64_t Result = FEX::HLE::HandleNewClone(Data->Thread, Data->CTX, &Data->NewFrame, &Data->GuestArgs);
  FEXCore::Threads::DeallocateStackObject(Data->NewStack, Data->StackSize);
  FEXCore::Allocator::free(arg);
  return Result;
}

// Clone3 flags
#ifndef CLONE_CLEAR_SIGHAND
#define CLONE_CLEAR_SIGHAND 0x100000000ULL
#endif
#ifndef CLONE_INTO_CGROUP
#define CLONE_INTO_CGROUP 0x200000000ULL
#endif
#ifndef CLONE_NEWTIME
// Overlaps CSIGNAL, can only be used with clone3 and not clone2
#define CLONE_NEWTIME 0x00000080ULL
#endif

static void PrintFlags(uint64_t Flags){
#define FLAGPRINT(x, y) if (Flags & (y)) LogMan::Msg::IFmt("\tFlag: " #x)
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

static uint64_t Clone2Handler(FEXCore::Core::CpuStateFrame *Frame, FEX::HLE::clone3_args *args) {
  StackFrameData *Data = (StackFrameData *)FEXCore::Allocator::malloc(sizeof(StackFrameData));
  Data->Thread = Frame->Thread;
  Data->CTX = Frame->Thread->CTX;
  Data->GuestArgs = args->args;

  // In the case of thread, we need a new stack
  Data->StackSize = 8 * 1024 * 1024;
  Data->NewStack = FEXCore::Threads::AllocateStackObject(Data->StackSize);

  // Create a copy of the parent frame
  memcpy(&Data->NewFrame, Frame, sizeof(FEXCore::Core::CpuStateFrame));

  // Remove flags that will break us
  constexpr uint64_t INVALID_FOR_HOST =
    CLONE_SETTLS;
  uint64_t Flags = args->args.flags & ~INVALID_FOR_HOST;
  uint64_t Result = ::clone(
    Clone2HandlerRet, // To be called function
    (void*)((uint64_t)Data->NewStack + Data->StackSize), // Stack
    Flags, //Flags
    Data, //Argument
    (pid_t*)args->args.parent_tid, // parent_tid
    0, // XXX: What is correct for this? tls
    (pid_t*)args->args.child_tid); // child_tid

  // Only parent will get here
  SYSCALL_ERRNO();
}

static uint64_t Clone3Handler(FEXCore::Core::CpuStateFrame *Frame, FEX::HLE::clone3_args *args) {
  // In the case of thread, we need a new stack
  uint64_t StackSize = 8 * 1024 * 1024;
  void *NewStack = FEXCore::Threads::AllocateStackObject(StackSize);

  constexpr size_t Offset = sizeof(StackFramePlusRet);
  StackFramePlusRet *Data = (StackFramePlusRet*)(reinterpret_cast<uint64_t>(NewStack) + StackSize - Offset);
  Data->Ret = (uint64_t)Clone3HandlerRet;
  Data->Data.Thread = Frame->Thread;
  Data->Data.CTX = Frame->Thread->CTX;
  Data->Data.GuestArgs = args->args;

  Data->Data.StackSize = StackSize;
  Data->Data.NewStack = NewStack;

  FEX::HLE::kernel_clone3_args HostArgs{};
  HostArgs.flags       = args->args.flags;
  HostArgs.pidfd       = args->args.pidfd;
  HostArgs.child_tid   = args->args.child_tid;
  HostArgs.parent_tid  = args->args.parent_tid;
  HostArgs.exit_signal = args->args.exit_signal;
  // Host stack is always created
  HostArgs.stack       = reinterpret_cast<uint64_t>(NewStack);
  HostArgs.stack_size  = StackSize - Offset; // Needs to be 16 byte aligned
  HostArgs.tls         = 0; // XXX: What is correct for this?
  HostArgs.set_tid     = args->args.set_tid;
  HostArgs.set_tid_size= args->args.set_tid_size;
  HostArgs.cgroup      = args->args.cgroup;

  // Create a copy of the parent frame
  memcpy(&Data->Data.NewFrame, Frame, sizeof(FEXCore::Core::CpuStateFrame));
  uint64_t Result = ::syscall(SYSCALL_DEF(clone3), &HostArgs, sizeof(HostArgs));

  // Only parent will get here
  SYSCALL_ERRNO();
};

uint64_t CloneHandler(FEXCore::Core::CpuStateFrame *Frame, FEX::HLE::clone3_args *args) {
  uint64_t flags = args->args.flags;

  auto HasUnhandledFlags = [](FEX::HLE::clone3_args *args) -> bool {
    constexpr uint64_t UNHANDLED_FLAGS =
      CLONE_NEWNS |
      // CLONE_UNTRACED |
      CLONE_NEWCGROUP |
      CLONE_NEWUTS |
      CLONE_NEWUTS |
      CLONE_NEWIPC |
      CLONE_NEWUSER |
      CLONE_NEWPID |
      CLONE_NEWNET |
      CLONE_IO |
      CLONE_CLEAR_SIGHAND |
      CLONE_INTO_CGROUP;

    if ((args->args.flags & UNHANDLED_FLAGS) != 0) {
      // Basic unhandled flags
      return true;
    }

    if (args->args.set_tid_size > 0) {
      // set_tid isn't exposed through anything other than clone3
      return true;
    }

    if (args->Type == TypeOfClone::TYPE_CLONE3) {
      if (AnyFlagsSet(args->args.flags, CLONE_NEWTIME)) {
        // New time namespace overlaps with CSIGNAL, only available in clone3
        return true;
      }
    }

    if (AnyFlagsSet(args->args.flags, CLONE_THREAD)) {
      if (!AllFlagsSet(args->args.flags, CLONE_SYSVSEM | CLONE_FS |  CLONE_FILES | CLONE_SIGHAND)) {
        LogMan::Msg::IFmt("clone: CLONE_THREAD: Unsuported flags w/ CLONE_THREAD (Shared Resources), {:X}", args->args.flags);
        return false;
      }
    }
    else {
      if (AnyFlagsSet(args->args.flags, CLONE_SYSVSEM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_VM)) {
        // CLONE_VM is particularly nasty here
        // Memory regions at the point of clone(More similar to a fork) are shared
        LogMan::Msg::IFmt("clone: Unsuported flags w/o CLONE_THREAD (Shared Resources), {:X}", args->args.flags);
        return false;
      }
    }

    // We support everything here
    return false;
  };

  // If there are flags that can't be handled regularly then we need to hand off to the true clone handler
  if (HasUnhandledFlags(args)) {
    if (!AnyFlagsSet(flags, CLONE_THREAD)) {
      // Has an unsupported flag
      // Fall to a handler that can handle this case
      if (args->Type == TYPE_CLONE2) {
        return Clone2Handler(Frame, args);
      }
      else {
        return Clone3Handler(Frame, args);
      }
    }
    else {
      LogMan::Msg::IFmt("Unsupported flag with CLONE_THREAD. This breaks TLS, falling down classic thread path");
      PrintFlags(flags);
    }
  }

  constexpr uint64_t TASK_MAX = (1ULL << 48); // 48-bits until we can query the host side VA sanely. AArch64 doesn't expose this in cpuinfo
  if (args->args.tls &&
      args->args.tls >= TASK_MAX) {
    return -EPERM;
  }

  auto Thread = Frame->Thread;

  if (AnyFlagsSet(flags, CLONE_PTRACE)) {
    PrintFlags(flags);
    LogMan::Msg::DFmt("clone: Ptrace* not supported");
  }

  if (!(flags & CLONE_THREAD)) {
    if (flags & CLONE_VFORK) {
      PrintFlags(flags);
      flags &= ~CLONE_VM;
      LogMan::Msg::DFmt("clone: WARNING: CLONE_VFORK w/o CLONE_THREAD");
    }

    // CLONE_PARENT is ignored (Implied by CLONE_THREAD)
    return FEX::HLE::ForkGuest(Thread, Frame, flags,
      reinterpret_cast<void*>(args->args.stack),
      reinterpret_cast<pid_t*>(args->args.parent_tid),
      reinterpret_cast<pid_t*>(args->args.child_tid),
      reinterpret_cast<void*>(args->args.tls));
  } else {
    auto NewThread = FEX::HLE::CreateNewThread(Thread->CTX, Frame, &args->args);

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
      uint64_t NewSizeAligned = FEXCore::AlignUp(NewSize, 4096);

      if (NewSizeAligned < DataSpaceMaxSize) {
        // If we are shrinking the brk then munmap the ranges
        // That way we gain the memory back and also give the application zero pages if it allocates again
        // DataspaceMaxSize is always page aligned

        uint64_t RemainingSize = DataSpaceMaxSize - NewSizeAligned;
        // We have pages we can unmap
        auto ok = GuestMunmap(reinterpret_cast<void*>(DataSpace + NewSizeAligned), RemainingSize);
        LOGMAN_THROW_A_FMT(ok != -1, "Munmap failed");
                
        DataSpaceMaxSize = NewSizeAligned;
      }
      else if (NewSize > DataSpaceMaxSize) {
        constexpr static uint64_t SizeAlignment = 8 * 1024 * 1024;
        uint64_t AllocateNewSize = FEXCore::AlignUp(NewSize, SizeAlignment) - DataSpaceMaxSize;
        if (!Is64BitMode() &&
          (DataSpace + DataSpaceMaxSize + AllocateNewSize > 0x1'0000'0000ULL)) {
          // If we are 32bit and we tried going about the 32bit limit then out of memory
          return DataSpace + DataSpaceSize;
        }

        uint64_t NewBRK{};
        NewBRK = (uint64_t)GuestMmap((void*)(DataSpace + DataSpaceMaxSize), AllocateNewSize, PROT_READ | PROT_WRITE, MAP_FIXED_NOREPLACE | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);


        if (NewBRK != ~0ULL && NewBRK != (DataSpace + DataSpaceMaxSize)) {
          // Couldn't allocate that the region we wanted
          // Can happen if MAP_FIXED_NOREPLACE isn't understood by the kernel
          int ok = GuestMunmap(reinterpret_cast<void*>(NewBRK), AllocateNewSize);
          LOGMAN_THROW_A_FMT(ok != -1, "Munmap failed");
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

SyscallHandler::SyscallHandler(FEXCore::Context::Context *_CTX, FEX::HLE::SignalDelegator *_SignalDelegation)
  : FM {_CTX}
  , CTX {_CTX}
  , SignalDelegation {_SignalDelegation} {
  FEX::HLE::_SyscallHandler = this;
  HostKernelVersion = CalculateHostKernelVersion();
  GuestKernelVersion = CalculateGuestKernelVersion();
  Alloc32Handler = FEX::HLE::Create32BitAllocator();
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
  // We currently only emulate a kernel between the ranges of Kernel 5.0.0 and 5.17.0
  return std::max(KernelVersion(5, 0), std::min(KernelVersion(5, 17), GetHostKernelVersion()));
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
    LOGMAN_MSG_A_FMT("Unhandled syscall: {}", Args->Argument[0]);
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
  ERROR_AND_DIE_FMT("Unhandled system call: {}", SyscallNumber);
  return -ENOSYS;
}

uint64_t UnimplementedSyscallSafe(FEXCore::Core::CpuStateFrame *Frame, uint64_t SyscallNumber) {
  return -ENOSYS;
}


//// VMA Tracking ////
static std::string get_fdpath(int fd)
{
      std::error_code ec;
      return std::filesystem::canonical(std::filesystem::path("/proc/self/fd") / std::to_string(fd), ec).string();
}

void SyscallHandler::TrackMmap(uintptr_t Base, uintptr_t Size, int Prot, int Flags, int fd, off_t Offset) {
  if (!(Flags & MAP_ANONYMOUS)) {
    auto filename = get_fdpath(fd);

    FEXCore::Context::AddNamedRegion(CTX, Base, Size, Offset, filename);
  }
  if (SMCChecks == FEXCore::Config::CONFIG_SMC_MMAN) {
    FEXCore::Context::InvalidateGuestCodeRange(CTX, (uintptr_t)Base, Size);
  }
}

void SyscallHandler::TrackMunmap(uintptr_t Base, uintptr_t Size) {  
  if (SMCChecks == FEXCore::Config::CONFIG_SMC_MMAN) {
    FEXCore::Context::InvalidateGuestCodeRange(CTX, (uintptr_t)Base, Size);
  }
}

void SyscallHandler::TrackMprotect(uintptr_t Base, uintptr_t Size, int Prot) {
  if (SMCChecks == FEXCore::Config::CONFIG_SMC_MMAN && Prot & PROT_EXEC) {
    FEXCore::Context::InvalidateGuestCodeRange(CTX, (uintptr_t)Base, Size);
  }
}

void SyscallHandler::TrackMremap(uintptr_t OldAddress, size_t OldSize, size_t NewSize, int flags, uintptr_t NewAddress) {
  if (SMCChecks == FEXCore::Config::CONFIG_SMC_MMAN) {
    FEXCore::Context::InvalidateGuestCodeRange(CTX, OldAddress, OldSize);
    FEXCore::Context::InvalidateGuestCodeRange(CTX, NewAddress, NewSize);
  }
}

void SyscallHandler::TrackShmat(int shmid, uintptr_t Base, int shmflg) {
  // TODO
}

void SyscallHandler::TrackShmdt(uintptr_t Base) {
  // TODO
}

void SyscallHandler::TrackMadvise(uintptr_t Base, uintptr_t Size, int advice) {
  // TODO
}

}
