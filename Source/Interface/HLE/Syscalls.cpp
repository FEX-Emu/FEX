#include "Common/MathUtils.h"

#include "Interface/Context/Context.h"
#include "Interface/Core/InternalThreadState.h"
#include "Interface/HLE/Syscalls.h"

#include "LogManager.h"

#include <FEXCore/Core/X86Enums.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/random.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <unistd.h>

constexpr uint64_t PAGE_SIZE = 4096;

namespace FEXCore {
#ifdef DEBUG_STRACE
void SyscallHandler::Strace(FEXCore::HLE::SyscallArguments *Args, uint64_t Ret) {
  switch (Args->Argument[0]) {
  case SYSCALL_BRK: LogMan::Msg::D("brk(%p)                               = 0x%lx", Args->Argument[1], Ret); break;
  case SYSCALL_ACCESS:
    LogMan::Msg::D("access(\"%s\", %d) = %ld", CTX->MemoryMapper.GetPointer<char const*>(Args->Argument[1]), Args->Argument[2], Ret);
    break;
  case SYSCALL_PIPE:
    LogMan::Msg::D("pipe({...}) = %ld", Ret);
    break;
  case SYSCALL_SELECT:
    LogMan::Msg::D("select(%ld, 0x%lx, 0x%lx, 0x%lx, 0x%lx) = %ld",
      Args->Argument[1],
      Args->Argument[2],
      Args->Argument[3],
      Args->Argument[4],
      Args->Argument[5],
      Ret);
    break;
  case SYSCALL_NANOSLEEP:
    LogMan::Msg::D("nanosleep(0x%ld, 0x%lx) = %ld",
      Args->Argument[1],
      Args->Argument[2],
      Ret);
    break;
  case SYSCALL_OPENAT:
    LogMan::Msg::D("openat(%ld, \"%s\", %d) = %ld", Args->Argument[1], CTX->MemoryMapper.GetPointer<char const*>(Args->Argument[2]), Args->Argument[3], Ret);
    break;
  case SYSCALL_STAT:
    LogMan::Msg::D("stat(\"%s\", {...}) = %ld", CTX->MemoryMapper.GetPointer<char const*>(Args->Argument[1]), Ret);
    break;
  case SYSCALL_FSTAT:
    LogMan::Msg::D("fstat(%ld, {...}) = %ld", Args->Argument[1], Ret);
    break;
  case SYSCALL_LSTAT:
    LogMan::Msg::D("lstat(\"%s\", {...}) = %ld", CTX->MemoryMapper.GetPointer<char const*>(Args->Argument[1]), Ret);
    break;
  case SYSCALL_POLL:
    LogMan::Msg::D("poll(0x%lx, %ld, %ld) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret);
    break;
  case SYSCALL_MMAP:
    LogMan::Msg::D("mmap(%p, %ld, %ld, %ld, %d, %p) = %p", Args->Argument[1], Args->Argument[2], Args->Argument[3], Args->Argument[4], static_cast<int32_t>(Args->Argument[5]), Args->Argument[6], Ret);
    break;
  case SYSCALL_CLOSE:
    LogMan::Msg::D("close(%ld) = %ld", Args->Argument[1], Ret);
    break;
  case SYSCALL_READ:
    LogMan::Msg::D("read(%ld, {...}, %ld) = %ld", Args->Argument[1], Args->Argument[3], Ret);
    break;
  case SYSCALL_ARCH_PRCTL:
    LogMan::Msg::D("arch_prctl(%ld, %p) = %ld", Args->Argument[1], Args->Argument[2], Ret);
    break;
  case SYSCALL_MPROTECT:
    LogMan::Msg::D("mprotect(%p, %ld, %ld) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret);
    break;
  case SYSCALL_MUNMAP:
    LogMan::Msg::D("munmap(%p, %ld) = %ld", Args->Argument[1], Args->Argument[2], Ret);
    break;
  case SYSCALL_UNAME:
    LogMan::Msg::D("uname ({...}) = %ld", Ret);
    break;
  case SYSCALL_FCNTL:
    LogMan::Msg::D("fcntl (%ld, %ld, 0x%lx) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret);
    break;
  case SYSCALL_UMASK:
    LogMan::Msg::D("umask(0x%lx) = %ld", Args->Argument[1], Ret);
    break;
  case SYSCALL_GETTIMEOFDAY:
    LogMan::Msg::D("gettimeofday(0x%lx, 0x%lx) = %ld", Args->Argument[1], Args->Argument[2], Ret);
    break;
  case SYSCALL_SYSINFO:
    LogMan::Msg::D("sysinfo(0x%lx) = %ld", Args->Argument[1], Ret);
    break;
  case SYSCALL_GETCWD:
    LogMan::Msg::D("getcwd(\"%s\", %ld) = %ld", CTX->MemoryMapper.GetPointer<char const*>(Args->Argument[1]), Args->Argument[2], Ret);
    break;
  case SYSCALL_CHDIR:
    LogMan::Msg::D("chdir(\"%s\") = %ld", CTX->MemoryMapper.GetPointer<char const*>(Args->Argument[1]), Ret);
    break;
  case SYSCALL_MKDIR:
    LogMan::Msg::D("mkdir(\"%s\", 0x%lx) = %ld", CTX->MemoryMapper.GetPointer<char const*>(Args->Argument[1]), Args->Argument[2], Ret);
    break;
  case SYSCALL_UNLINK:
    LogMan::Msg::D("unlink(\"%s\", 0x%lx) = %ld", CTX->MemoryMapper.GetPointer<char const*>(Args->Argument[1]), Ret);
    break;
  case SYSCALL_EXIT:
    LogMan::Msg::D("exit(0x%lx)", Args->Argument[1]);
    break;
  case SYSCALL_WRITE:
    LogMan::Msg::D("write(%ld, %p, %ld) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret);
    break;
  case SYSCALL_WRITEV:
    LogMan::Msg::D("writev(%ld, %p, %ld) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret);
    break;
  case SYSCALL_EXIT_GROUP:
    LogMan::Msg::D("exit_group(0x%lx)", Args->Argument[1]);
    break;
  case SYSCALL_GETDENTS64:
    LogMan::Msg::D("getdents(%ld, 0x%lx, %ld) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret);
    break;
  case SYSCALL_SET_TID_ADDRESS:
    LogMan::Msg::D("set_tid_address(%p) = %ld", Args->Argument[1], Ret);
    break;
  case SYSCALL_SET_ROBUST_LIST:
    LogMan::Msg::D("set_robust_list(%p, %ld) = %ld", Args->Argument[1], Args->Argument[2], Ret);
    break;
  case SYSCALL_EPOLL_CREATE1:
    LogMan::Msg::D("epoll_create1(%ld) = %ld", Args->Argument[1], Ret);
    break;
  case SYSCALL_PIPE2:
    LogMan::Msg::D("pipe2({...}, %d) = %ld", Args->Argument[2], Ret);
    break;
  case SYSCALL_RT_SIGACTION:
    LogMan::Msg::D("rt_sigaction(%ld, %p, %p) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret);
    break;
  case SYSCALL_RT_SIGPROCMASK:
    LogMan::Msg::D("rt_sigprocmask(%ld, %p, %p, %ld) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Args->Argument[4], Ret);
    break;
  case SYSCALL_PRLIMIT64:
    LogMan::Msg::D("prlimit64(%ld, %ld, %p, %p) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Args->Argument[4], Ret);
    break;
  case SYSCALL_SENDMMSG:
    LogMan::Msg::D("sendmmsg(%ld, 0x%lx, %ld, %ld) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Args->Argument[4], Ret);
    break;
  case SYSCALL_GETPID:
    LogMan::Msg::D("getpid() = %ld", Ret);
    break;
  case SYSCALL_SOCKET:
    LogMan::Msg::D("socket(%ld, %ld, %ld) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret);
    break;
  case SYSCALL_CONNECT:
    LogMan::Msg::D("connect(%ld, 0x%lx, %ld) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret);
    break;
  case SYSCALL_RECVFROM:
    LogMan::Msg::D("recvfrom(%ld, 0x%lx, %ld, %ld, 0x%lx, 0x%lx) = %ld",
        Args->Argument[1],
        Args->Argument[2],
        Args->Argument[3],
        Args->Argument[4],
        Args->Argument[5],
        Args->Argument[6],
        Ret);
    break;
  case SYSCALL_RECVMSG:
    LogMan::Msg::D("recvmsg(%ld, 0x%lx, %ld) = %ld",
        Args->Argument[1],
        Args->Argument[2],
        Args->Argument[3],
        Ret);
    break;
  case SYSCALL_SHUTDOWN:
    LogMan::Msg::D("shutdown(%ld, %ld) = %ld", Args->Argument[1], Args->Argument[2], Ret);
    break;
  case SYSCALL_GETSOCKNAME:
    LogMan::Msg::D("getsockname(%ld, 0x%lx, 0x%lx) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret);
    break;
  case SYSCALL_GETPEERNAME:
    LogMan::Msg::D("getpeername(%ld, 0x%lx, 0x%lx) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret);
    break;
  case SYSCALL_SETSOCKOPT:
    LogMan::Msg::D("setsockopt(%ld, %ld, %ld, 0x%lx, %ld) = %ld",
        Args->Argument[1],
        Args->Argument[2],
        Args->Argument[3],
        Args->Argument[4],
        Args->Argument[5],
        Ret);
    break;
  case SYSCALL_GETSOCKOPT:
    LogMan::Msg::D("getsockopt(%ld, %ld, %ld, 0x%lx, 0x%lx) = %ld",
        Args->Argument[1],
        Args->Argument[2],
        Args->Argument[3],
        Args->Argument[4],
        Args->Argument[5],
        Ret);
    break;
  case SYSCALL_CLONE:
    LogMan::Msg::I("clone(%lx,\n\t%lx,\n\t%lx,\n\t%lx,\n\t%lx,\n\t%lx,\n\t%lx)",
        Args->Argument[1],
        Args->Argument[2],
        Args->Argument[3],
        Args->Argument[4],
        Args->Argument[5],
        Args->Argument[6]);
    break;
  case SYSCALL_GETUID:
    LogMan::Msg::D("getuid() = %ld", Ret);
    break;
  case SYSCALL_GETGID:
    LogMan::Msg::D("getgid() = %ld", Ret);
    break;
  case SYSCALL_SETUID:
    LogMan::Msg::D("setuid(%ld) = %ld", Args->Argument[1], Ret);
    break;
  case SYSCALL_GETEUID:
    LogMan::Msg::D("geteuid() = %ld", Ret);
    break;
  case SYSCALL_GETEGID:
    LogMan::Msg::D("getegid() = %ld", Ret);
    break;
  case SYSCALL_SETREGID:
    LogMan::Msg::D("setregid(%ld, %ld) = %ld", Args->Argument[1], Args->Argument[2], Ret);
    break;
  case SYSCALL_SETRESUID:
    LogMan::Msg::D("setresuid(%ld, %ld, %ld) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret);
    break;
  case SYSCALL_GETTID:
    LogMan::Msg::D("gettid() = %ld", Ret);
    break;
  case SYSCALL_TGKILL:
    LogMan::Msg::D("tgkill(%ld, %ld) = %ld", Args->Argument[1], Args->Argument[2], Ret);
    break;
  case SYSCALL_LSEEK:
    LogMan::Msg::D("lseek(%ld, %ld, %ld) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret);
    break;
  case SYSCALL_IOCTL:
    LogMan::Msg::D("ioctl(%ld, %ld, %p) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret);
    break;
  case SYSCALL_PREAD64:
    LogMan::Msg::D("pread64(%ld, 0x%lx, %ld, %ld) = %ld", Args->Argument[1], Args->Argument[2], Args->Argument[3], Args->Argument[4], Ret);
    break;
  case SYSCALL_TIME:
    LogMan::Msg::D("time(%p) = %ld", Args->Argument[1], Ret);
    break;
  case SYSCALL_FUTEX:
    LogMan::Msg::D("futex(%p, %ld, %ld, 0x%lx, 0x%lx, %ld) = %ld",
      Args->Argument[1],
      Args->Argument[2],
      Args->Argument[3],
      Args->Argument[4],
      Args->Argument[5],
      Args->Argument[6],
      Ret);
    break;
  case SYSCALL_CLOCK_GETTIME:
    LogMan::Msg::D("clock_gettime(%ld, %p) = %ld", Args->Argument[1], Args->Argument[2], Ret);
    break;
  case SYSCALL_CLOCK_GETRES:
    LogMan::Msg::D("clock_getres(%ld, %p) = %ld", Args->Argument[1], Args->Argument[2], Ret);
    break;
  case SYSCALL_READLINK:
    LogMan::Msg::D("readlink(\"%s\") = %ld", CTX->MemoryMapper.GetPointer<char const*>(Args->Argument[1]), Ret);
    break;
  case SYSCALL_GETRANDOM:
    LogMan::Msg::D("getrandom(0x%lx, 0x%lx, 0x%lx) = %ld",
        Args->Argument[1],
        Args->Argument[2],
        Args->Argument[3],
        Ret);
    break;
  default: LogMan::Msg::D("Unknown strace: %d", Args->Argument[0]);
  }
}
#endif
void SyscallHandler::DefaultProgramBreak(FEXCore::Core::InternalThreadState *Thread, uint64_t Addr) {
  DataSpaceSize = 0;
  DataSpace = Addr;
  DefaultProgramBreakAddress = Addr;

  // Just allocate 1GB of data memory past the default program break location at this point
  CTX->MapRegion(Thread, Addr, 0x1000'0000, true);
}

uint64_t SyscallHandler::HandleSyscall(FEXCore::Core::InternalThreadState *Thread, FEXCore::HLE::SyscallArguments *Args) {
  uint64_t Result = 0;

  switch (Args->Argument[0]) {
  case SYSCALL_UNAME: {
    struct _utsname {
      char sysname[65];
      char nodename[65];
      char release[65];
      char version[65];
      char machine[65];
    };
    _utsname *Local = CTX->MemoryMapper.GetPointer<_utsname*>(Args->Argument[1]);
    strcpy(Local->sysname, "Linux");
    strcpy(Local->nodename, "FEXCore");
    strcpy(Local->release, "5.0.0");
    strcpy(Local->version, "#1");
    strcpy(Local->machine, "x86_64");
    Result = 0;
  break;
  }
  // Memory management
  case SYSCALL_BRK: {
    if (Args->Argument[1] == 0) { // Just wants to get the location of the program break atm
      if (DataSpace == 0) {
        // XXX: We need to setup our default BRK space first
        DefaultProgramBreak(Thread, 0xe000'0000);
      }
      Result = DataSpace + DataSpaceSize;
    }
    else {
      // Allocating out data space
      uint64_t NewEnd = Args->Argument[1];
      if (NewEnd < DataSpace) {
        // Not allowed to move brk end below original start
        // Set the size to zero
        DataSpaceSize = 0;
      }
      else {
        uint64_t NewSize = NewEnd - DataSpace;
        DataSpaceSize = NewSize;
      }
      Result = DataSpace + DataSpaceSize;
    }
  break;
  }
  case SYSCALL_MMAP: {
    int Flags = Args->Argument[4];
    int GuestFD = static_cast<int32_t>(Args->Argument[5]);
    int HostFD = -1;

    if (GuestFD != -1) {
      HostFD = FM.FindHostFD(GuestFD);
    }

    uint64_t Base = AlignDown(LastMMAP, PAGE_SIZE);
    uint64_t Size = AlignUp(Args->Argument[2], PAGE_SIZE);
    uint64_t FileSizeToUse = Args->Argument[2];
    uint64_t Prot = Args->Argument[3];

    if (Flags & MAP_FIXED) {
      Base = Args->Argument[1];
      void *HostPtr = CTX->MemoryMapper.GetPointerSizeCheck(Base, FileSizeToUse);
      if (!HostPtr) {
        HostPtr = CTX->MapRegion(Thread, Base, Size, true);
      }

      if (HostFD != -1) {
        void *FileMem = mmap(HostPtr, FileSizeToUse, Prot, MAP_DENYWRITE | MAP_PRIVATE | MAP_FIXED, HostFD, Args->Argument[6]);
        if (FileMem == MAP_FAILED) {
          LogMan::Msg::A("Couldn't map file to %p\n", HostPtr);
        }
      }
      else {
        LogMan::Throw::A(Args->Argument[6] == 0, "Don't understand a fixed offset mmap");
        mmap(HostPtr, Size, Prot, Flags, HostFD, Args->Argument[6]);
      }

      Result = Base;
    }
    else {
      // XXX: MMAP should map memory regions for all threads
      void *HostPtr = CTX->MemoryMapper.GetPointerSizeCheck(Base, FileSizeToUse);
      if (!HostPtr) {
        HostPtr = CTX->MapRegion(Thread, Base, Size, true);
      }

      if (HostFD != -1) {
        void *FileMem = mmap(HostPtr, FileSizeToUse, Prot, MAP_PRIVATE | MAP_FIXED, HostFD, Args->Argument[6]);
        if (FileMem == MAP_FAILED) {
          LogMan::Msg::A("Couldn't map file to %p\n", HostPtr);
        }
      }
      else {
        mmap(HostPtr, Size, Prot, Flags, HostFD, Args->Argument[6]);
      }

      LastMMAP += Size;
      Result = Base;
    }
  break;
  }
  case SYSCALL_MPROTECT: {
    void *HostPtr = CTX->MemoryMapper.GetPointer<void*>(Args->Argument[1]);
    Result = mprotect(HostPtr, Args->Argument[2], Args->Argument[3]);
  break;
  }
  case SYSCALL_ARCH_PRCTL: {
    switch (Args->Argument[1]) {
    case 0x1001: // ARCH_SET_GS
      Thread->State.State.gs = Args->Argument[2];
      Result = 0;
    break;
    case 0x1002: // ARCH_SET_FS
      Thread->State.State.fs = Args->Argument[2];
      Result = 0;
    break;
    case 0x1003: // ARCH_GET_FS
      *CTX->MemoryMapper.GetPointer<uint64_t*>(Args->Argument[2]) = Thread->State.State.fs;
      Result = 0;
    break;
    case 0x1004: // ARCH_GET_GS
      *CTX->MemoryMapper.GetPointer<uint64_t*>(Args->Argument[2]) = Thread->State.State.gs;
      Result = 0;
    break;
    case 0x3001: // ARCH_CET_STATUS
      Result = -22; // We don't support CET, return EINVAL
    break;
    default:
      LogMan::Msg::E("Unknown prctl: 0x%x", Args->Argument[1]);
      CTX->ShouldStop = true;
    break;
    }
  break;
  }
  case SYSCALL_SOCKET:
    Result = FM.Socket(Args->Argument[1], Args->Argument[2], Args->Argument[3]);
  break;
  case SYSCALL_CONNECT:
    Result = FM.Connect(Args->Argument[1],
      CTX->MemoryMapper.GetPointer<const struct sockaddr *>(Args->Argument[2]),
      Args->Argument[3]);
  break;
  case SYSCALL_RECVFROM:
    Result = FM.Recvfrom(Args->Argument[1],
      CTX->MemoryMapper.GetPointer(Args->Argument[2]),
      Args->Argument[3],
      Args->Argument[4],
      CTX->MemoryMapper.GetPointer<struct sockaddr *>(Args->Argument[5]),
      CTX->MemoryMapper.GetPointer<socklen_t*>(Args->Argument[6]));
  break;
  case SYSCALL_RECVMSG:
    Result = FM.Recvmsg(Args->Argument[1],
      CTX->MemoryMapper.GetPointer<struct msghdr*>(Args->Argument[2]),
      Args->Argument[3]);
  break;
  case SYSCALL_SHUTDOWN:
    Result = FM.Shutdown(Args->Argument[1],
      Args->Argument[2]);
  break;
  case SYSCALL_GETSOCKNAME:
    Result = FM.GetSockName(Args->Argument[1],
      CTX->MemoryMapper.GetPointer<struct sockaddr *>(Args->Argument[2]),
      CTX->MemoryMapper.GetPointer<socklen_t *>(Args->Argument[3]));
  break;
  case SYSCALL_GETPEERNAME:
    Result = FM.GetPeerName(Args->Argument[1],
      CTX->MemoryMapper.GetPointer<struct sockaddr *>(Args->Argument[2]),
      CTX->MemoryMapper.GetPointer<socklen_t *>(Args->Argument[3]));
  break;
  case SYSCALL_SETSOCKOPT:
    Result = FM.SetSockOpt(Args->Argument[1],
      Args->Argument[2],
      Args->Argument[3],
      CTX->MemoryMapper.GetPointer<const void*>(Args->Argument[4]),
      Args->Argument[5]);
  break;
  case SYSCALL_GETSOCKOPT:
    Result = FM.GetSockOpt(Args->Argument[1],
      Args->Argument[2],
      Args->Argument[3],
      CTX->MemoryMapper.GetPointer<void*>(Args->Argument[4]),
      CTX->MemoryMapper.GetPointer<socklen_t *>(Args->Argument[5]));
  break;
  case SYSCALL_POLL:
    Result = FM.Poll(CTX->MemoryMapper.GetPointer<struct pollfd*>(Args->Argument[1]), Args->Argument[2], Args->Argument[3]);
  break;
  // Thread management
  case SYSCALL_GETUID:
    Result = Thread->State.ThreadManager.GetUID();
  break;
  case SYSCALL_GETGID:
    Result = Thread->State.ThreadManager.GetGID();
  break;
  case SYSCALL_SETUID:
    // XXX: Not a real result
    Result = -1;
  break;
  case SYSCALL_GETEUID:
    Result = Thread->State.ThreadManager.GetEUID();
  break;
  case SYSCALL_GETEGID:
    Result = Thread->State.ThreadManager.GetEGID();
  break;
  case SYSCALL_SETREGID:
    // XXX: Not a real result
    Result = 0;
  break;
  case SYSCALL_SETRESUID:
    // XXX: Not a real result
    Result = -1;
  break;
  case SYSCALL_GETTID:
    Result = Thread->State.ThreadManager.GetTID();
  break;
  case SYSCALL_GETPID:
    Result = Thread->State.ThreadManager.GetPID();
  break;
  case SYSCALL_EXIT:
    LogMan::Msg::D("Thread exit with: %zd\n", Args->Argument[1]);
    Thread->State.RunningEvents.ShouldStop = true;
  break;
  case SYSCALL_WAIT4:
    LogMan::Msg::I("wait4(%lx,\n\t%lx,\n\t%lx,\n\t%lx)",
      Args->Argument[1],
      Args->Argument[2],
      Args->Argument[3],
      Args->Argument[4]);
  break;

  // Futexes
  case SYSCALL_FUTEX: {

  // 0 : uaddr
  // 1 : op
  // 2: val
  // 3: utime
  // 4: uaddr2
  // 5: val3
  LogMan::Msg::I("futex(%lx,\n\t%lx,\n\t%lx,\n\t%lx,\n\t%lx,\n\t%lx)",
      Args->Argument[1],
      Args->Argument[2],
      Args->Argument[3],
      Args->Argument[4],
      Args->Argument[5],
      Args->Argument[6]);

  uint8_t Command = Args->Argument[2] & 0xF;
  Result = 0;
  switch (Command) {
  case 0: { // WAIT
    LogMan::Throw::A(!Args->Argument[4], "Can't handle timed futexes");
    Futex *futex = new Futex{}; // XXX: Definitely a memory leak. When should we free this?
    futex->Addr = CTX->MemoryMapper.GetPointer<std::atomic<uint32_t>*>(Args->Argument[1]);
    futex->Val = Args->Argument[3];
    EmplaceFutex(Args->Argument[1], futex);
    {
      std::unique_lock<std::mutex> lk(futex->Mutex);
      futex->cv.wait(lk, [futex] { return futex->Addr->load() != futex->Val; });
    }
  break;
  }
  case 1: { // WAKE
    Futex *futex = GetFutex(Args->Argument[1]);
    if (!futex) {
      Result = 0;
      break;
    }
    for (uint64_t i = 0; i < Args->Argument[3]; ++i)
      futex->cv.notify_one();
  break;
  }
  default:
    LogMan::Msg::A("Unknown futex command");
  break;
  }
  break;
  }
  case SYSCALL_CLONE: {
    // 0: clone_flags
    // 1: New SP
    // 2: parent tidptr
    // 3: child tidptr
    // 4: TLS
    uint32_t Flags = Args->Argument[1];
    uint64_t NewSP = Args->Argument[2];
    uint64_t ParentTID = Args->Argument[3];
    uint64_t ChildTID = Args->Argument[4];
    uint64_t NewTLS = Args->Argument[5];
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

    FEXCore::Core::CPUState NewThreadState{};
    // Clone copies the parent thread's state
    memcpy(&NewThreadState, &Thread->State.State, sizeof(FEXCore::Core::CPUState));

    NewThreadState.gregs[FEXCore::X86State::REG_RAX] = 0;
    NewThreadState.gregs[FEXCore::X86State::REG_RBX] = 0;
    NewThreadState.gregs[FEXCore::X86State::REG_RBP] = 0;
    NewThreadState.gregs[FEXCore::X86State::REG_RSP] = NewSP;
    NewThreadState.fs = NewTLS;

    // Set us to start just after the syscall instruction
    NewThreadState.rip += 2;

    auto NewThread = CTX->CreateThread(&NewThreadState, ParentTID, ChildTID);
    CTX->CopyMemoryMapping(Thread, NewThread);

    // Sets the child TID to pointer in ParentTID
    if (Flags & CLONE_PARENT_SETTID) {
      uint64_t *TIDPtr = CTX->MemoryMapper.GetPointer<uint64_t*>(ParentTID);
      TIDPtr[0] = NewThread->State.ThreadManager.GetTID();
    }

    // Sets the child TID to the pointer in ChildTID
    if (Flags & CLONE_CHILD_SETTID) {
      uint64_t *TIDPtr = CTX->MemoryMapper.GetPointer<uint64_t*>(ChildTID);
      TIDPtr[0] = NewThread->State.ThreadManager.GetTID();
    }

    // When the thread exits, clear the child thread ID at ChildTID
    // Additionally wakeup a futex at that address
    // Address /may/ be changed with SET_TID_ADDRESS syscall
    if (Flags & CLONE_CHILD_CLEARTID) {
    }

    CTX->InitializeThread(NewThread);

    // Actually start the thread
    CTX->RunThread(NewThread);

    // Return the new threads TID
    Result = NewThread->State.ThreadManager.GetTID();
  break;
  }
  // File management
  case SYSCALL_READ:
    Result = FM.Read(Args->Argument[1],
        CTX->MemoryMapper.GetPointer(Args->Argument[2]),
        Args->Argument[3]);
  break;
  case SYSCALL_WRITE:
    Result = FM.Write(Args->Argument[1],
        CTX->MemoryMapper.GetPointer(Args->Argument[2]),
        Args->Argument[3]);
  break;
  case SYSCALL_OPEN:
    Result = FM.Open(CTX->MemoryMapper.GetPointer<char const*>(Args->Argument[1]),
        Args->Argument[2],
        Args->Argument[3]);
  break;
  case SYSCALL_CLOSE:
    Result = FM.Close(Args->Argument[1]);
  break;
  case SYSCALL_STAT:
    Result = FM.Stat(CTX->MemoryMapper.GetPointer<char const*>(Args->Argument[1]),
        CTX->MemoryMapper.GetPointer(Args->Argument[2]));
  break;
  case SYSCALL_FSTAT:
    Result = FM.Fstat(Args->Argument[1],
        CTX->MemoryMapper.GetPointer(Args->Argument[2]));
  break;
  case SYSCALL_LSTAT:
    Result = FM.Lstat(CTX->MemoryMapper.GetPointer<char const*>(Args->Argument[1]),
        CTX->MemoryMapper.GetPointer(Args->Argument[2]));
  break;
  case SYSCALL_LSEEK:
    Result = FM.Lseek(Args->Argument[1],
        Args->Argument[2],
        Args->Argument[3]);
  break;
  case SYSCALL_WRITEV:
    Result = FM.Writev(Args->Argument[1],
        CTX->MemoryMapper.GetPointer(Args->Argument[2]),
        Args->Argument[3]);
  break;
  case SYSCALL_ACCESS:
  Result = FM.Access(
      CTX->MemoryMapper.GetPointer<const char*>(Args->Argument[1]),
      Args->Argument[2]);
  break;
  case SYSCALL_PIPE:
  Result = FM.Pipe(
      CTX->MemoryMapper.GetPointer<int*>(Args->Argument[1]));
  break;
  case SYSCALL_SELECT: {
    fd_set *readfds{};
    fd_set *writefds{};
    fd_set *exceptfds{};
    struct timeval *timeout{};

    if (Args->Argument[2])
      readfds = CTX->MemoryMapper.GetPointer<fd_set*>(Args->Argument[2]);
    if (Args->Argument[3])
      writefds = CTX->MemoryMapper.GetPointer<fd_set*>(Args->Argument[3]);
    if (Args->Argument[4])
      exceptfds = CTX->MemoryMapper.GetPointer<fd_set*>(Args->Argument[4]);
    if (Args->Argument[5])
      timeout = CTX->MemoryMapper.GetPointer<struct timeval*>(Args->Argument[5]);

    Result = select(Args->Argument[1],
      readfds, writefds, exceptfds, timeout);
  break;
  }
  case SYSCALL_READLINK:
    Result = FM.Readlink(
      CTX->MemoryMapper.GetPointer<const char*>(Args->Argument[1]),
      CTX->MemoryMapper.GetPointer<char*>(Args->Argument[2]),
      Args->Argument[3]);
  break;
  case SYSCALL_OPENAT:
    Result = FM.Openat(
      Args->Argument[1],
      CTX->MemoryMapper.GetPointer<const char*>(Args->Argument[2]),
      Args->Argument[3],
      Args->Argument[4]);
  break;
  case SYSCALL_IOCTL:
    Result = FM.Ioctl(
      Args->Argument[1],
      Args->Argument[2],
      CTX->MemoryMapper.GetPointer<void*>(Args->Argument[3]));
  break;
  case SYSCALL_PREAD64:
    Result = FM.PRead64(
      Args->Argument[1],
      CTX->MemoryMapper.GetPointer<void*>(Args->Argument[2]),
      Args->Argument[3],
      Args->Argument[4]);
  break;
  case SYSCALL_TIME: {
    time_t TmpTime;
    TmpTime = time(nullptr);
    if (Args->Argument[1]) {
      time_t *ClockResult = CTX->MemoryMapper.GetPointer<time_t*>(Args->Argument[1]);
      *ClockResult = TmpTime;
#ifdef DEBUG_TIME
      memset(ClockResult, 0, sizeof(time_t));
#endif
    }
    Result = TmpTime;
#ifdef DEBUG_TIME
    Result = 0;
#endif
  }
  break;
  case SYSCALL_CLOCK_GETTIME: {
    timespec *ClockResult = CTX->MemoryMapper.GetPointer<timespec*>(Args->Argument[2]);
    Result = clock_gettime(Args->Argument[1], ClockResult);
#ifdef DEBUG_TIME
    memset(ClockResult, 0, sizeof(timespec));
#endif
  }
  break;
  case SYSCALL_SYSINFO: {
    struct sysinfo *info = CTX->MemoryMapper.GetPointer<struct sysinfo*>(Args->Argument[1]);
    Result = sysinfo(info);
  }
  break;
  case SYSCALL_CLOCK_GETRES: {
    struct timespec *res{};
    if (Args->Argument[2])
      res = CTX->MemoryMapper.GetPointer<struct timespec*>(Args->Argument[2]);
    Result = clock_getres(Args->Argument[1], res);
  }
  break;
  case SYSCALL_NANOSLEEP: {
    timespec const* req = CTX->MemoryMapper.GetPointer<timespec const*>(Args->Argument[1]);
    timespec *rem = CTX->MemoryMapper.GetPointer<timespec*>(Args->Argument[2]);
    Result = nanosleep(req, rem);
  break;
  }
  case SYSCALL_GETDENTS64: {
    Result = FM.GetDents(Args->Argument[1],
      CTX->MemoryMapper.GetPointer(Args->Argument[2]),
      Args->Argument[3]);
  break;
  }
  case SYSCALL_SET_TID_ADDRESS: {
    Thread->State.ThreadManager.child_tid = Args->Argument[1];
    Result = Thread->State.ThreadManager.GetTID();
  break;
  }
  case SYSCALL_SET_ROBUST_LIST: {
    Thread->State.ThreadManager.robust_list_head = Args->Argument[1];
    Result = 0;
  break;
  }
  case SYSCALL_EPOLL_CREATE1: {
    Result = FM.EPoll_Create1(Args->Argument[1]);
  break;
  }
  case SYSCALL_PIPE2:
  Result = FM.Pipe2(
      CTX->MemoryMapper.GetPointer<int*>(Args->Argument[1]),
      Args->Argument[2]);
  break;
  case SYSCALL_PRLIMIT64: {
    LogMan::Throw::A(Args->Argument[3] == 0, "Guest trying to set limit for %d", Args->Argument[2]);
    struct rlimit {
      uint64_t rlim_cur;
      uint64_t rlim_max;
    };
    switch (Args->Argument[2]) {
    case 3: { // Stack limits
      rlimit *old_limit = CTX->MemoryMapper.GetPointer<rlimit*>(Args->Argument[3]);
      // Default size
      old_limit->rlim_cur = 8 * 1024;
      old_limit->rlim_max = ~0ULL;
    break;
    }
    default: LogMan::Msg::A("Unknown PRLimit: %d", Args->Argument[2]);
    }
    Result = 0;
  break;
  }
  case SYSCALL_SENDMMSG:
    Result = FM.Sendmmsg(Args->Argument[1],
      CTX->MemoryMapper.GetPointer<struct mmsghdr*>(Args->Argument[2]),
      Args->Argument[3],
      Args->Argument[4]);
    break;
  case SYSCALL_UMASK:
    // Just say that the mask has always matched what was passed in
    Result = Args->Argument[1];
    break;
  case SYSCALL_GETTIMEOFDAY: {
    struct timeval *tv{};
    struct timezone *tz{};
    if (Args->Argument[1])
      tv = CTX->MemoryMapper.GetPointer<struct timeval*>(Args->Argument[1]);
    if (Args->Argument[2])
      tz = CTX->MemoryMapper.GetPointer<struct timezone*>(Args->Argument[2]);
    Result = gettimeofday(tv, tz);
    break;
  }
  case SYSCALL_GETCWD:
  {
    char *ptr = getcwd(CTX->MemoryMapper.GetPointer<char *>(Args->Argument[1]), Args->Argument[2]);
    if (!ptr) {
      Result = errno;
    } else {
      Result = strlen(ptr) + 1;
    }
    break;
  }
  case SYSCALL_CHDIR:
    Result = chdir(CTX->MemoryMapper.GetPointer<char const*>(Args->Argument[1]));
    break;
  case SYSCALL_MKDIR:
    Result = mkdir(CTX->MemoryMapper.GetPointer<char const*>(Args->Argument[1]), Args->Argument[2]);
    break;
  case SYSCALL_UNLINK:
    Result = unlink(CTX->MemoryMapper.GetPointer<char const*>(Args->Argument[1]));
    break;
  case SYSCALL_FCNTL: {
    int Cmd = Args->Argument[2];
    auto FD = FM.GetFDBacking(Args->Argument[1]);
    auto HostFD = FD->GetHostFD();
    switch (Cmd) {
      case 1: // F_GETFD
        Result = fcntl(HostFD, Cmd);
        break;
      case 2: // F_SETFD
        Result = fcntl(HostFD, Cmd, Args->Argument[3]);
        break;
      case 3: // F_GETFL
        Result = fcntl(HostFD, Cmd);
        break;
      case 4: // F_SETFL
        Result = fcntl(HostFD, Cmd, Args->Argument[3]);
        break;
      default: LogMan::Msg::A("FCNTL: Unknown Command: %ld", Cmd); break;
    }
    break;
  }
  case SYSCALL_GETRANDOM:
    Result = getrandom(CTX->MemoryMapper.GetPointer<void*>(Args->Argument[1]),
      Args->Argument[2],
      Args->Argument[3]);
    break;
  // Currently unhandled
  // Return fake result
  case SYSCALL_RT_SIGACTION:
  case SYSCALL_RT_SIGPROCMASK:
  case SYSCALL_EXIT_GROUP:
  case SYSCALL_TGKILL:
  case SYSCALL_MUNMAP:
    Result = 0;
  break;
  default:
    Result = -1;
    LogMan::Msg::A("Unknown syscall: %d", Args->Argument[0]);
  break;
  }

#ifdef DEBUG_STRACE
  Strace(Args, Result);
#endif
  return Result;
}
}
