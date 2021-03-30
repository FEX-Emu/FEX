/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/Syscalls/Thread.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Thread.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Thread.h"

#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Debug/InternalThreadState.h>

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
  FEXCore::Core::InternalThreadState *CreateNewThread(FEXCore::Context:: Context *CTX, FEXCore::Core::CpuStateFrame *Frame, uint32_t flags, void *stack, pid_t *parent_tid, pid_t *child_tid, void *tls) {
    FEXCore::Core::CPUState NewThreadState{};
    // Clone copies the parent thread's state
    memcpy(&NewThreadState, Frame, sizeof(FEXCore::Core::CPUState));

    NewThreadState.gregs[FEXCore::X86State::REG_RAX] = 0;
    NewThreadState.gregs[FEXCore::X86State::REG_RBX] = 0;
    NewThreadState.gregs[FEXCore::X86State::REG_RBP] = 0;
    NewThreadState.gregs[FEXCore::X86State::REG_RSP] = reinterpret_cast<uint64_t>(stack);

    auto NewThread = FEXCore::Context::CreateThread(CTX, &NewThreadState, reinterpret_cast<uint64_t>(parent_tid));
    FEXCore::Context::InitializeThread(CTX, NewThread);

    if (FEX::HLE::_SyscallHandler->Is64BitMode()) {
      if (flags & CLONE_SETTLS) {
        x64::SetThreadArea(NewThread->CurrentFrame, tls);
      }
      // Set us to start just after the syscall instruction
      x64::AdjustRipForNewThread(NewThread->CurrentFrame);
    }
    else {
      if (flags & CLONE_SETTLS) {
        x32::SetThreadArea(NewThread->CurrentFrame, tls);
      }
      x32::AdjustRipForNewThread(NewThread->CurrentFrame);
    }

    // Return the new threads TID
    uint64_t Result = NewThread->ThreadManager.GetTID();

    // Sets the child TID to pointer in ParentTID
    if (flags & CLONE_PARENT_SETTID) {
      *parent_tid = Result;
    }

    // Sets the child TID to the pointer in ChildTID
    if (flags & CLONE_CHILD_SETTID) {
      NewThread->ThreadManager.set_child_tid = child_tid;
      *child_tid = Result;
    }

    // When the thread exits, clear the child thread ID at ChildTID
    // Additionally wakeup a futex at that address
    // Address /may/ be changed with SET_TID_ADDRESS syscall
    if (flags & CLONE_CHILD_CLEARTID) {
      NewThread->ThreadManager.clear_child_tid = child_tid;
    }

    return NewThread;
  }

  uint64_t ForkGuest(FEXCore::Core::InternalThreadState *Thread, FEXCore::Core::CpuStateFrame *Frame, uint32_t flags, void *stack, pid_t *parent_tid, pid_t *child_tid, void *tls) {
    pid_t Result = fork();

    if (Result == 0) {
      // Child
      // update the internal TID
      Thread->ThreadManager.TID = ::gettid();
      Thread->ThreadManager.PID = ::getpid();
      Thread->ThreadManager.clear_child_tid = nullptr;

      // Clear all the other threads that are being tracked
      FEXCore::Context::CleanupAfterFork(Thread->CTX, Frame->Thread);

      // only a  single thread running so no need to remove anything from the thread array

      // Handle child setup now
      if (stack != nullptr) {
        // use specified stack
        LogMan::Msg::D("@@@@@@@ Fork uses custom stack");
        Frame->State.gregs[FEXCore::X86State::REG_RSP] = reinterpret_cast<uint64_t>(stack);
      } else {
        // In the case of fork and nullptr stack then the child uses the same stack space as the parent
        // Same virtual address, different addressspace
        LogMan::Msg::D("@@@@@@@ Fork uses parent stack");
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

  void RegisterThread() {
    REGISTER_SYSCALL_IMPL(getpid, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      uint64_t Result = ::getpid();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(fork, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      return ForkGuest(Frame->Thread, Frame, 0, 0, 0, 0, 0);
    });

    REGISTER_SYSCALL_IMPL(vfork, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      return ForkGuest(Frame->Thread, Frame, 0, 0, 0, 0, 0);
    });

    REGISTER_SYSCALL_IMPL(exit, [](FEXCore::Core::CpuStateFrame *Frame, int status) -> uint64_t {
      auto Thread = Frame->Thread;
      if (Thread->ThreadManager.clear_child_tid) {
        std::atomic<uint32_t> *Addr = reinterpret_cast<std::atomic<uint32_t>*>(Thread->ThreadManager.clear_child_tid);
        Addr->store(0);
        syscall(SYS_futex,
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

    REGISTER_SYSCALL_IMPL(kill, [](FEXCore::Core::CpuStateFrame *Frame, pid_t pid, int sig) -> uint64_t {
      uint64_t Result = ::kill(pid, sig);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(tkill, [](FEXCore::Core::CpuStateFrame *Frame, int tid, int sig) -> uint64_t {
      uint64_t Result = ::tgkill(-1, tid, sig);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(tgkill, [](FEXCore::Core::CpuStateFrame *Frame, int tgid, int tid, int sig) -> uint64_t {
      uint64_t Result = ::tgkill(tgid, tid, sig);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(getuid, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      uint64_t Result = ::getuid();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(getgid, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      uint64_t Result = ::getgid();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(setuid, [](FEXCore::Core::CpuStateFrame *Frame, uid_t uid) -> uint64_t {
      uint64_t Result = ::setuid(uid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(setgid, [](FEXCore::Core::CpuStateFrame *Frame, gid_t gid) -> uint64_t {
      uint64_t Result = ::setgid(gid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(geteuid, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      uint64_t Result = ::geteuid();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(getegid, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      uint64_t Result = ::getegid();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(getppid, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      uint64_t Result = ::getppid();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(getpgrp, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      uint64_t Result = ::getpgrp();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(setsid, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      uint64_t Result = ::setsid();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(setreuid, [](FEXCore::Core::CpuStateFrame *Frame, uid_t ruid, uid_t euid) -> uint64_t {
      uint64_t Result = ::setreuid(ruid, euid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(setregid, [](FEXCore::Core::CpuStateFrame *Frame, gid_t rgid, gid_t egid) -> uint64_t {
      uint64_t Result = ::setregid(rgid, egid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(getgroups, [](FEXCore::Core::CpuStateFrame *Frame, int size, gid_t list[]) -> uint64_t {
      uint64_t Result = ::getgroups(size, list);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(setgroups, [](FEXCore::Core::CpuStateFrame *Frame, size_t size, const gid_t *list) -> uint64_t {
      uint64_t Result = ::setgroups(size, list);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(setresuid, [](FEXCore::Core::CpuStateFrame *Frame, uid_t ruid, uid_t euid, uid_t suid) -> uint64_t {
      uint64_t Result = ::setresuid(ruid, euid, suid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(getresuid, [](FEXCore::Core::CpuStateFrame *Frame, uid_t *ruid, uid_t *euid, uid_t *suid) -> uint64_t {
      uint64_t Result = ::getresuid(ruid, euid, suid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(setresgid, [](FEXCore::Core::CpuStateFrame *Frame, gid_t rgid, gid_t egid, gid_t sgid) -> uint64_t {
      uint64_t Result = ::setresgid(rgid, egid, sgid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(getresgid, [](FEXCore::Core::CpuStateFrame *Frame, gid_t *rgid, gid_t *egid, gid_t *sgid) -> uint64_t {
      uint64_t Result = ::getresgid(rgid, egid, sgid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(personality, [](FEXCore::Core::CpuStateFrame *Frame, uint64_t persona) -> uint64_t {
      uint64_t Result = ::personality(persona);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(prctl, [](FEXCore::Core::CpuStateFrame *Frame, int option, unsigned long arg2, unsigned long arg3, unsigned long arg4, unsigned long arg5) -> uint64_t {
      uint64_t Result = ::prctl(option, arg2, arg3, arg4, arg5);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(arch_prctl, [](FEXCore::Core::CpuStateFrame *Frame, int code, unsigned long addr) -> uint64_t {
      uint64_t Result{};
      switch (code) {
        case 0x1001: // ARCH_SET_GS
          Frame->State.gs = addr;
          Result = 0;
        break;
        case 0x1002: // ARCH_SET_FS
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
        default:
          LogMan::Msg::E("Unknown prctl: 0x%x", code);
          Result = -EINVAL;
        break;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(gettid, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      uint64_t Result = ::gettid();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(set_tid_address, [](FEXCore::Core::CpuStateFrame *Frame, int *tidptr) -> uint64_t {
      auto Thread = Frame->Thread;
      Thread->ThreadManager.clear_child_tid = tidptr;
      return Thread->ThreadManager.GetTID();
    });

    REGISTER_SYSCALL_IMPL(exit_group, [](FEXCore::Core::CpuStateFrame *Frame, int status) -> uint64_t {
      auto Thread = Frame->Thread;
      Thread->StatusCode = status;
      FEXCore::Context::Stop(Thread->CTX);
      // This will never be reached
      std::unexpected();
    });

    REGISTER_SYSCALL_IMPL(prlimit64, [](FEXCore::Core::CpuStateFrame *Frame, pid_t pid, int resource, const struct rlimit *new_limit, struct rlimit *old_limit) -> uint64_t {
      uint64_t Result = ::prlimit(pid, (enum __rlimit_resource)(resource), new_limit, old_limit);
      SYSCALL_ERRNO();
    });

    /*
    REGISTER_SYSCALL_IMPL(setpgid, [](FEXCore::Core::CpuStateFrame *Frame, pid_t pid, pid_t pgid) -> uint64_t {
      SYSCALL_STUB(setpgid);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(setpgid);

    /*REGISTER_SYSCALL_IMPL(getpgid, [](FEXCore::Core::CpuStateFrame *Frame, pid_t pid) -> uint64_t {
      SYSCALL_STUB(getpgid);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(getpgid);

    /*REGISTER_SYSCALL_IMPL(setfsuid, [](FEXCore::Core::CpuStateFrame *Frame, uid_t fsuid) -> uint64_t {
      SYSCALL_STUB(setfsuid);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(setfsuid);

    /*REGISTER_SYSCALL_IMPL(setfsgid, [](FEXCore::Core::CpuStateFrame *Frame, uid_t fsgid) -> uint64_t {
      SYSCALL_STUB(setfsgid);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(setfsgid);

    /*REGISTER_SYSCALL_IMPL(getsid, [](FEXCore::Core::CpuStateFrame *Frame, pid_t pid) -> uint64_t {
      SYSCALL_STUB(getsid);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(getsid);

    REGISTER_SYSCALL_IMPL(waitid, [](FEXCore::Core::CpuStateFrame *Frame, idtype_t idtype, id_t id, siginfo_t *infop, int options) -> uint64_t {
      uint64_t Result = ::waitid(idtype, id, infop, options);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(unshare, [](FEXCore::Core::CpuStateFrame *Frame, int flags) -> uint64_t {
      uint64_t Result = ::unshare(flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(setns, [](FEXCore::Core::CpuStateFrame *Frame, int fd, int nstype) -> uint64_t {
      uint64_t Result = ::setns(fd, nstype);
      SYSCALL_ERRNO();
    });
  }
}
