/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "FEXCore/IR/IR.h"
#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/Syscalls/Thread.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Thread.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Thread.h"

#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/IR/IR.h>

#include <FEXHeaderUtils/Syscalls.h>

#include <grp.h>
#include <limits.h>
#include <linux/futex.h>
#include <stdint.h>
#include <sched.h>
#include <sys/personality.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/fsuid.h>

ARG_TO_STR(idtype_t, "%u")

namespace FEX::HLE {
  FEXCore::Core::InternalThreadState *CreateNewThread(FEXCore::Context:: Context *CTX, FEXCore::Core::CpuStateFrame *Frame, FEX::HLE::kernel_clone3_args *args) {
    uint64_t flags = args->flags;
    FEXCore::Core::CPUState NewThreadState{};
    // Clone copies the parent thread's state
    memcpy(&NewThreadState, Frame, sizeof(FEXCore::Core::CPUState));

    NewThreadState.gregs[FEXCore::X86State::REG_RAX] = 0;
    NewThreadState.gregs[FEXCore::X86State::REG_RBX] = 0;
    NewThreadState.gregs[FEXCore::X86State::REG_RBP] = 0;
    NewThreadState.gregs[FEXCore::X86State::REG_RSP] = args->stack;

    auto NewThread = FEXCore::Context::CreateThread(CTX, &NewThreadState, args->parent_tid);
    FEXCore::Context::InitializeThread(CTX, NewThread);

    if (FEX::HLE::_SyscallHandler->Is64BitMode()) {
      if (flags & CLONE_SETTLS) {
        x64::SetThreadArea(NewThread->CurrentFrame, reinterpret_cast<void*>(args->tls));
      }
      // Set us to start just after the syscall instruction
      x64::AdjustRipForNewThread(NewThread->CurrentFrame);
    }
    else {
      if (flags & CLONE_SETTLS) {
        x32::SetThreadArea(NewThread->CurrentFrame, reinterpret_cast<void*>(args->tls));
      }
      x32::AdjustRipForNewThread(NewThread->CurrentFrame);
    }

    // Return the new threads TID
    uint64_t Result = NewThread->ThreadManager.GetTID();

    // Sets the child TID to pointer in ParentTID
    if (flags & CLONE_PARENT_SETTID) {
      *reinterpret_cast<pid_t*>(args->parent_tid) = Result;
    }

    // Sets the child TID to the pointer in ChildTID
    if (flags & CLONE_CHILD_SETTID) {
      NewThread->ThreadManager.set_child_tid = reinterpret_cast<int32_t*>(args->child_tid);
      *reinterpret_cast<pid_t*>(args->child_tid) = Result;
    }

    // When the thread exits, clear the child thread ID at ChildTID
    // Additionally wakeup a futex at that address
    // Address /may/ be changed with SET_TID_ADDRESS syscall
    if (flags & CLONE_CHILD_CLEARTID) {
      NewThread->ThreadManager.clear_child_tid = reinterpret_cast<int32_t*>(args->child_tid);
    }

    // clone3 flag
    if (flags & CLONE_PIDFD) {
      // Use pidfd_open to emulate this flag
      const int pidfd = ::syscall(SYSCALL_DEF(pidfd_open), Result, 0);
      if (Result == ~0ULL) {
        LogMan::Msg::EFmt("Couldn't get pidfd of TID {}\n", Result);
      }
      else {
        *reinterpret_cast<int*>(args->pidfd) = pidfd;
      }
    }

    return NewThread;
  }

  uint64_t HandleNewClone(FEXCore::Core::InternalThreadState *Thread, FEXCore::Context::Context *CTX, FEXCore::Core::CpuStateFrame *Frame, FEX::HLE::kernel_clone3_args *GuestArgs) {
    uint64_t flags = GuestArgs->flags;
    auto NewThread = Thread;
    if (flags & CLONE_THREAD) {
      FEXCore::Core::CPUState NewThreadState{};
      // Clone copies the parent thread's state
      memcpy(&NewThreadState, Frame, sizeof(FEXCore::Core::CPUState));

      NewThreadState.gregs[FEXCore::X86State::REG_RAX] = 0;
      NewThreadState.gregs[FEXCore::X86State::REG_RBX] = 0;
      NewThreadState.gregs[FEXCore::X86State::REG_RBP] = 0;
      if (GuestArgs->stack == 0) {
        // Copies in the original thread's stack
      }
      else {
        NewThreadState.gregs[FEXCore::X86State::REG_RSP] = GuestArgs->stack;
      }

      // Overwrite thread
      NewThread = FEXCore::Context::CreateThread(CTX, &NewThreadState, GuestArgs->parent_tid);

      // CLONE_PARENT_SETTID, CLONE_CHILD_SETTID, CLONE_CHILD_CLEARTID, CLONE_PIDFD will be handled by kernel
      // Call execution thread directly since we already are on the new thread
      NewThread->StartRunning.NotifyAll(); // Clear the start running flag
    }
    else{
      // If we don't have CLONE_THREAD then we are effectively a fork
      // Clear all the other threads that are being tracked
      // Frame->Thread is /ONLY/ safe to access when CLONE_THREAD flag is not set
      FEXCore::Context::CleanupAfterFork(CTX, Frame->Thread);

      Thread->CurrentFrame->State.gregs[FEXCore::X86State::REG_RAX] = 0;
      Thread->CurrentFrame->State.gregs[FEXCore::X86State::REG_RBX] = 0;
      Thread->CurrentFrame->State.gregs[FEXCore::X86State::REG_RBP] = 0;
      if (GuestArgs->stack == 0) {
        // Copies in the original thread's stack
      }
      else {
        Thread->CurrentFrame->State.gregs[FEXCore::X86State::REG_RSP] = GuestArgs->stack;
      }
    }

    if (FEX::HLE::_SyscallHandler->Is64BitMode()) {
      if (flags & CLONE_SETTLS) {
        x64::SetThreadArea(NewThread->CurrentFrame, reinterpret_cast<void*>(GuestArgs->tls));
      }
      // Set us to start just after the syscall instruction
      x64::AdjustRipForNewThread(NewThread->CurrentFrame);
    }
    else {
      if (flags & CLONE_SETTLS) {
        x32::SetThreadArea(NewThread->CurrentFrame, reinterpret_cast<void*>(GuestArgs->tls));
      }
      x32::AdjustRipForNewThread(NewThread->CurrentFrame);
    }

    // Depending on clone settings, our TID and PID could have changed
    Thread->ThreadManager.TID = FHU::Syscalls::gettid();
    Thread->ThreadManager.PID = ::getpid();
    FEX::HLE::_SyscallHandler->FM.UpdatePID(Thread->ThreadManager.PID);

    // Start exuting the thread directly
    // Our host clone starts in a new stack space, so it can't return back to the JIT space
    FEXCore::Context::ExecutionThread(CTX, Thread);

    // The rest of the context remains as is and the thread will continue executing
    return Thread->StatusCode;
  }

  uint64_t ForkGuest(FEXCore::Core::InternalThreadState *Thread, FEXCore::Core::CpuStateFrame *Frame, uint32_t flags, void *stack, pid_t *parent_tid, pid_t *child_tid, void *tls) {
    // Just before we fork, we lock all syscall mutexes so that both processes will end up with a locked mutex
    FEX::HLE::_SyscallHandler->LockBeforeFork();
    
    pid_t Result{};
    if (flags & CLONE_VFORK) {
      // XXX: We don't currently support a vfork as it causes problems.
      // Currently behaves like a fork, which isn't correct. Need to find where the problem is
      Result = fork();
    }
    else {
      Result = fork();
    }

    // Unlock the mutexes on both sides of the fork
    FEX::HLE::_SyscallHandler->UnlockAfterFork();

    if (Result == 0) {
      // Child
      // update the internal TID
      Thread->ThreadManager.TID = FHU::Syscalls::gettid();
      Thread->ThreadManager.PID = ::getpid();
      FEX::HLE::_SyscallHandler->FM.UpdatePID(Thread->ThreadManager.PID);
      Thread->ThreadManager.clear_child_tid = nullptr;

      // Clear all the other threads that are being tracked
      FEXCore::Context::CleanupAfterFork(Thread->CTX, Frame->Thread);

      // only a  single thread running so no need to remove anything from the thread array

      // Handle child setup now
      if (stack != nullptr) {
        // use specified stack
        Frame->State.gregs[FEXCore::X86State::REG_RSP] = reinterpret_cast<uint64_t>(stack);
      } else {
        // In the case of fork and nullptr stack then the child uses the same stack space as the parent
        // Same virtual address, different addressspace
      }

      if (FEX::HLE::_SyscallHandler->Is64BitMode()) {
        if (flags & CLONE_SETTLS) {
          x64::SetThreadArea(Frame, tls);
        }
      }
      else {
        // 32bit TLS doesn't just set the fs register
        if (flags & CLONE_SETTLS) {
          x32::SetThreadArea(Frame, tls);
        }
      }

      // Sets the child TID to the pointer in ChildTID
      if (flags & CLONE_CHILD_SETTID) {
        Thread->ThreadManager.set_child_tid = child_tid;
        *child_tid = Thread->ThreadManager.TID;
      }

      // When the thread exits, clear the child thread ID at ChildTID
      // Additionally wakeup a futex at that address
      // Address /may/ be changed with SET_TID_ADDRESS syscall
      if (flags & CLONE_CHILD_CLEARTID) {
        Thread->ThreadManager.clear_child_tid = child_tid;
      }

      // the rest of the context remains as is, this thread will keep executing
      return 0;
    } else {
      if (Result != -1) {
        if (flags & CLONE_PARENT_SETTID) {
          *parent_tid = Result;
        }
      }
      // Parent
      SYSCALL_ERRNO();
    }
  }

  void RegisterThread(FEX::HLE::SyscallHandler *const Handler) {
    using namespace FEXCore::IR;

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(getpid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      uint64_t Result = ::getpid();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_FLAGS(fork, SyscallFlags::DEFAULT, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      return ForkGuest(Frame->Thread, Frame, 0, 0, 0, 0, 0);
    });

    REGISTER_SYSCALL_IMPL_FLAGS(vfork, SyscallFlags::DEFAULT, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      return ForkGuest(Frame->Thread, Frame, CLONE_VFORK, 0, 0, 0, 0);
    });

    REGISTER_SYSCALL_IMPL_FLAGS(clone3, SyscallFlags::DEFAULT, ([](FEXCore::Core::CpuStateFrame *Frame, FEX::HLE::kernel_clone3_args *cl_args, size_t size) -> uint64_t {
      FEX::HLE::clone3_args args{};
      args.Type = TypeOfClone::TYPE_CLONE3;
      memcpy(&args.args, cl_args, std::min(sizeof(FEX::HLE::kernel_clone3_args), size));
      return CloneHandler(Frame, &args);
    }));

    REGISTER_SYSCALL_IMPL_FLAGS(exit, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY | SyscallFlags::NORETURN,
      [](FEXCore::Core::CpuStateFrame *Frame, int status) -> uint64_t {
      auto Thread = Frame->Thread;
      if (Thread->ThreadManager.clear_child_tid) {
        std::atomic<uint32_t> *Addr = reinterpret_cast<std::atomic<uint32_t>*>(Thread->ThreadManager.clear_child_tid);
        Addr->store(0);
        syscall(SYSCALL_DEF(futex),
          Thread->ThreadManager.clear_child_tid,
          FUTEX_WAKE,
          ~0ULL,
          0,
          0,
          0);
      }

      Thread->StatusCode = status;
      FEXCore::Context::StopThread(Thread->CTX, Thread);

      return 0;
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(kill, SyscallFlags::DEFAULT, [](FEXCore::Core::CpuStateFrame *Frame, pid_t pid, int sig) -> uint64_t {
      uint64_t Result = ::kill(pid, sig);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(tkill, SyscallFlags::DEFAULT, [](FEXCore::Core::CpuStateFrame *Frame, int tid, int sig) -> uint64_t {
      // Can't actually use tgkill here, kernel rejects tgkill of tgid == 0
      uint64_t Result = ::syscall(SYSCALL_DEF(tkill), tid, sig);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(tgkill, SyscallFlags::DEFAULT, [](FEXCore::Core::CpuStateFrame *Frame, int tgid, int tid, int sig) -> uint64_t {
      uint64_t Result = FHU::Syscalls::tgkill(tgid, tid, sig);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(getuid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      uint64_t Result = ::getuid();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(getgid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      uint64_t Result = ::getgid();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(setuid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, uid_t uid) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(setuid), uid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(setgid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, gid_t gid) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(setgid), gid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(geteuid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      uint64_t Result = ::geteuid();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(getegid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      uint64_t Result = ::getegid();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(getppid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      uint64_t Result = ::getppid();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(getpgrp, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      uint64_t Result = ::getpgrp();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(setsid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      uint64_t Result = ::setsid();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(setreuid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, uid_t ruid, uid_t euid) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(setreuid), ruid, euid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(setregid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, gid_t rgid, gid_t egid) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(setregid), rgid, egid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(getgroups, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int size, gid_t list[]) -> uint64_t {
      uint64_t Result = ::getgroups(size, list);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(setgroups, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, size_t size, const gid_t *list) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(setgroups), size, list);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(setresuid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, uid_t ruid, uid_t euid, uid_t suid) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(setresuid), ruid, euid, suid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(getresuid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, uid_t *ruid, uid_t *euid, uid_t *suid) -> uint64_t {
      uint64_t Result = ::getresuid(ruid, euid, suid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(setresgid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, gid_t rgid, gid_t egid, gid_t sgid) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(setresgid), rgid, egid, sgid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(getresgid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, gid_t *rgid, gid_t *egid, gid_t *sgid) -> uint64_t {
      uint64_t Result = ::getresgid(rgid, egid, sgid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(personality, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, uint64_t persona) -> uint64_t {
      uint64_t Result = ::personality(persona);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_FLAGS(prctl, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int option, unsigned long arg2, unsigned long arg3, unsigned long arg4, unsigned long arg5) -> uint64_t {
      uint64_t Result{};
      switch (option) {
      case PR_SET_SECCOMP:
      case PR_GET_SECCOMP:
        // FEX doesn't support seccomp
        return -EINVAL;
        break;
      default:
        Result = ::prctl(option, arg2, arg3, arg4, arg5);
      break;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_FLAGS(arch_prctl, SyscallFlags::DEFAULT,
      [](FEXCore::Core::CpuStateFrame *Frame, int code, unsigned long addr) -> uint64_t {
      constexpr uint64_t TASK_MAX = (1ULL << 48); // 48-bits until we can query the host side VA sanely. AArch64 doesn't expose this in cpuinfo
      uint64_t Result{};
      switch (code) {
        case 0x1001: // ARCH_SET_GS
          if (addr >= TASK_MAX) {
            // Ignore a non-canonical address
            return -EPERM;
          }
          Frame->State.gs = addr;
          Result = 0;
        break;
        case 0x1002: // ARCH_SET_FS
          if (addr >= TASK_MAX) {
            // Ignore a non-canonical address
            return -EPERM;
          }
          Frame->State.fs = addr;
          Result = 0;
        break;
        case 0x1003: // ARCH_GET_FS
          *reinterpret_cast<uint64_t*>(addr) = Frame->State.fs;
          Result = 0;
        break;
        case 0x1004: // ARCH_GET_GS
          *reinterpret_cast<uint64_t*>(addr) = Frame->State.gs;
          Result = 0;
        break;
        case 0x3001: // ARCH_CET_STATUS
          Result = -EINVAL; // We don't support CET, return EINVAL
        break;
        case 0x1011: // ARCH_GET_CPUID
          return 1;
        break;
        case 0x1012: // ARCH_SET_CPUID
          return -ENODEV; // Claim we don't support faulting on CPUID
        break;
        default:
          LogMan::Msg::EFmt("Unknown prctl: 0x{:x}", code);
          Result = -EINVAL;
        break;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(gettid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      uint64_t Result = FHU::Syscalls::gettid();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(set_tid_address, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int *tidptr) -> uint64_t {
      auto Thread = Frame->Thread;
      Thread->ThreadManager.clear_child_tid = tidptr;
      return Thread->ThreadManager.GetTID();
    });

    REGISTER_SYSCALL_IMPL_FLAGS(exit_group, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY | SyscallFlags::NORETURN,
      [](FEXCore::Core::CpuStateFrame *Frame, int status) -> uint64_t {
      auto Thread = Frame->Thread;
      Thread->StatusCode = status;
      FEXCore::Context::Stop(Thread->CTX);
      // This will never be reached
      std::terminate();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(prlimit_64, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, pid_t pid, int resource, const struct rlimit *new_limit, struct rlimit *old_limit) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(prlimit_64), pid, resource, new_limit, old_limit);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(setpgid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, pid_t pid, pid_t pgid) -> uint64_t {
      uint64_t Result = ::setpgid(pid, pgid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(getpgid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, pid_t pid) -> uint64_t {
      uint64_t Result = ::getpgid(pid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(setfsuid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, uid_t fsuid) -> uint64_t {
      uint64_t Result = ::setfsuid(fsuid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(setfsgid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, uid_t fsgid) -> uint64_t {
      uint64_t Result = ::setfsgid(fsgid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(getsid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, pid_t pid) -> uint64_t {
      uint64_t Result = ::getsid(pid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(unshare, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int flags) -> uint64_t {
      uint64_t Result = ::unshare(flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_PASS_FLAGS(setns, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int fd, int nstype) -> uint64_t {
      uint64_t Result = ::setns(fd, nstype);
      SYSCALL_ERRNO();
    });

    if (Handler->IsHostKernelVersionAtLeast(5, 16, 0)) {
      REGISTER_SYSCALL_IMPL_PASS_FLAGS(futex_waitv, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
        [](FEXCore::Core::CpuStateFrame *Frame, void *waiters, uint32_t nr_futexes, uint32_t flags, struct timespec *timeout, clockid_t clockid) -> uint64_t {
        uint64_t Result = ::syscall(SYSCALL_DEF(futex_waitv), waiters, nr_futexes, flags, timeout, clockid);
        SYSCALL_ERRNO();
      });
    }
    else {
      REGISTER_SYSCALL_IMPL(futex_waitv, UnimplementedSyscallSafe);
    }
  }
}
