#include "Interface/Context/Context.h"
#include "Interface/Core/InternalThreadState.h"
#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"

#include <FEXCore/Core/CodeLoader.h>
#include <FEXCore/Core/X86Enums.h>

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

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {

  FEXCore::Core::InternalThreadState *CreateNewThread(FEXCore::Core::InternalThreadState *Thread, uint32_t flags, void *stack, pid_t *parent_tid, pid_t *child_tid, void *tls) {
    FEXCore::Core::CPUState NewThreadState{};
    // Clone copies the parent thread's state
    memcpy(&NewThreadState, &Thread->State.State, sizeof(FEXCore::Core::CPUState));

    NewThreadState.gregs[FEXCore::X86State::REG_RAX] = 0;
    NewThreadState.gregs[FEXCore::X86State::REG_RBX] = 0;
    NewThreadState.gregs[FEXCore::X86State::REG_RBP] = 0;
    NewThreadState.gregs[FEXCore::X86State::REG_RSP] = reinterpret_cast<uint64_t>(stack);

    if (flags & CLONE_SETTLS) {
      NewThreadState.fs = reinterpret_cast<uint64_t>(tls);
    }

    // Set us to start just after the syscall instruction
    NewThreadState.rip += 2;

    auto NewThread = Thread->CTX->CreateThread(&NewThreadState, reinterpret_cast<uint64_t>(parent_tid));
    Thread->CTX->CopyMemoryMapping(Thread, NewThread);

    Thread->CTX->InitializeThread(NewThread);

    // Return the new threads TID
    uint64_t Result = NewThread->State.ThreadManager.GetTID();

    // Sets the child TID to pointer in ParentTID
    if (flags & CLONE_PARENT_SETTID) {
      *parent_tid = Result;
    }

    // Sets the child TID to the pointer in ChildTID
    if (flags & CLONE_CHILD_SETTID) {
      NewThread->State.ThreadManager.set_child_tid = child_tid;
      *child_tid = Result;
    }

    // When the thread exits, clear the child thread ID at ChildTID
    // Additionally wakeup a futex at that address
    // Address /may/ be changed with SET_TID_ADDRESS syscall
    if (flags & CLONE_CHILD_CLEARTID) {
      NewThread->State.ThreadManager.clear_child_tid = child_tid;
    }

    return NewThread;
  }

  uint64_t DoFork(FEXCore::Core::InternalThreadState *Thread, uint32_t flags, void *stack, pid_t *parent_tid, pid_t *child_tid, void *tls) {
    LogMan::Msg::E("FORKING!");
    uint64_t PrevTID = Thread->State.ThreadManager.TID;
    pid_t pid = fork();
    if (pid == 0) {
      // Child
      // uhhh...
      Thread->CTX->HandleForkChildSide(PrevTID);
      Thread->State.ThreadManager.TID = ::gettid();
    }
    else {
      // Parent
      return pid;
    }

    // Handle child setup now
    if (stack == nullptr) {
      // In the case of fork and nullptr stack then the child uses the same stack space as the parent
      // Same virtual address, different addressspace
      LogMan::Msg::D("@@@@@@@ Had to setup custom stack");
      stack = (void*)Thread->State.State.gregs[X86State::REG_RSP];
    }

    LogMan::Msg::D("Time to spin up fork thread with Stack: 0x%lx\n", stack);
    auto NewThread = CreateNewThread(Thread, flags, stack, 0, child_tid, tls);

    NewThread->State.State.gregs[FEXCore::X86State::REG_RAX] = 0;
    // Return the new threads TID
    uint64_t Result = NewThread->State.ThreadManager.GetTID();
    LogMan::Msg::D("Child [%d] starting at: 0x%lx. Parent was at 0x%lx", Result, NewThread->State.State.rip, Thread->State.State.rip);

    // Actually start the thread
    Thread->CTX->RunThread(NewThread);

    // This thread needs to die now
    // New thread becomes the new thread
    Thread->State.ThreadManager.parent_tid = ~0ULL;
    Thread->State.RunningEvents.ShouldStop = true;

    return 0;
  }


  void RegisterThread() {
    REGISTER_SYSCALL_IMPL(getpid, [](FEXCore::Core::InternalThreadState *Thread) -> uint64_t {
  #if 1
      uint64_t Result = Thread->CTX->GetPIDHack();
      if (Result == ~0ULL) {
        Result = ::getpid();
        SYSCALL_ERRNO();
      }
      return Result;

  #else
      uint64_t Result = ::getpid();
      SYSCALL_ERRNO();
  #endif
    });

    REGISTER_SYSCALL_IMPL(clone, [](FEXCore::Core::InternalThreadState *Thread, uint32_t flags, void *stack, pid_t *parent_tid, pid_t *child_tid, void *tls) -> uint64_t {
    #define FLAGPRINT(x, y) if (flags & (y)) LogMan::Msg::I("\tFlag: " #x)
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

      if (!(flags & CLONE_VM)) {
        LogMan::Msg::D("Parent [%d] Leaving syscall at rip: 0x%lx", Thread->State.ThreadManager.GetTID(), Thread->State.State.rip);
        return DoFork(Thread, flags, stack, parent_tid, child_tid, tls);
      }

      auto NewThread = CreateNewThread(Thread, flags, stack, parent_tid, child_tid, tls);

      // Return the new threads TID
      uint64_t Result = NewThread->State.ThreadManager.GetTID();
      LogMan::Msg::D("Child [%d] starting at: 0x%lx. Parent was at 0x%lx", Result, NewThread->State.State.rip, Thread->State.State.rip);

      // Actually start the thread
      Thread->CTX->RunThread(NewThread);

      if (flags & CLONE_VFORK) {
        // If VFORK is set then the calling process is suspended until the thread exits with execve or exit
        NewThread->ExecutionThread.join();
      }

      if (!(flags & CLONE_VM)) {
        // This thread needs to die now
        // New thread becomes the new thread
        NewThread->ExecutionThread.join();
        Thread->State.ThreadManager.parent_tid = ~0ULL;
        Thread->State.RunningEvents.ShouldStop = true;
      }

      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(execve, [](FEXCore::Core::InternalThreadState *Thread, const char *pathname, char *const argv[], char *const envp[]) -> uint64_t {
      // XXX: Disallow execve for now
      return -ENOEXEC;
    });

    REGISTER_SYSCALL_IMPL(exit, [](FEXCore::Core::InternalThreadState *Thread, int status) -> uint64_t {
      Thread->State.RunningEvents.ShouldStop = true;
      if (Thread->State.ThreadManager.clear_child_tid) {
        std::atomic<uint32_t> *Addr = reinterpret_cast<std::atomic<uint32_t>*>(Thread->State.ThreadManager.clear_child_tid);
        Addr->store(0);
        syscall(SYS_futex,
          Thread->State.ThreadManager.clear_child_tid,
          FUTEX_WAKE,
          ~0ULL,
          0,
          0,
          0);
      }

      Thread->StatusCode = status;

      return 0;
    });

    REGISTER_SYSCALL_IMPL(wait4, [](FEXCore::Core::InternalThreadState *Thread, pid_t pid, int *wstatus, int options, struct rusage *rusage) -> uint64_t {
      uint64_t Result = ::wait4(pid, wstatus, options, rusage);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(kill, [](FEXCore::Core::InternalThreadState *Thread, pid_t pid, int sig) -> uint64_t {
      uint64_t Result = ::kill(pid, sig);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(tkill, [](FEXCore::Core::InternalThreadState *Thread, int tid, int sig) -> uint64_t {
      uint64_t Result = ::tgkill(-1, tid, sig);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(tgkill, [](FEXCore::Core::InternalThreadState *Thread, int tgid, int tid, int sig) -> uint64_t {
      uint64_t Result = ::tgkill(tgid, tid, sig);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(getuid, [](FEXCore::Core::InternalThreadState *Thread) -> uint64_t {
      uint64_t Result = ::getuid();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(getgid, [](FEXCore::Core::InternalThreadState *Thread) -> uint64_t {
      uint64_t Result = ::getgid();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(setuid, [](FEXCore::Core::InternalThreadState *Thread, uid_t uid) -> uint64_t {
      uint64_t Result = ::setuid(uid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(setgid, [](FEXCore::Core::InternalThreadState *Thread, gid_t gid) -> uint64_t {
      uint64_t Result = ::setgid(gid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(geteuid, [](FEXCore::Core::InternalThreadState *Thread) -> uint64_t {
      uint64_t Result = ::geteuid();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(getegid, [](FEXCore::Core::InternalThreadState *Thread) -> uint64_t {
      uint64_t Result = ::getegid();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(getppid, [](FEXCore::Core::InternalThreadState *Thread) -> uint64_t {
      uint64_t Result = ::getppid();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(getpgrp, [](FEXCore::Core::InternalThreadState *Thread) -> uint64_t {
      uint64_t Result = ::getpgrp();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(setsid, [](FEXCore::Core::InternalThreadState *Thread) -> uint64_t {
      uint64_t Result = ::setsid();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(setreuid, [](FEXCore::Core::InternalThreadState *Thread, uid_t ruid, uid_t euid) -> uint64_t {
      uint64_t Result = ::setreuid(ruid, euid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(setregid, [](FEXCore::Core::InternalThreadState *Thread, gid_t rgid, gid_t egid) -> uint64_t {
      uint64_t Result = ::setregid(rgid, egid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(setresuid, [](FEXCore::Core::InternalThreadState *Thread, uid_t ruid, uid_t euid, uid_t suid) -> uint64_t {
      uint64_t Result = ::setresuid(ruid, euid, suid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(getresuid, [](FEXCore::Core::InternalThreadState *Thread, uid_t *ruid, uid_t *euid, uid_t *suid) -> uint64_t {
      uint64_t Result = ::getresuid(ruid, euid, suid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(setresgid, [](FEXCore::Core::InternalThreadState *Thread, gid_t rgid, gid_t egid, gid_t sgid) -> uint64_t {
      uint64_t Result = ::setresgid(rgid, egid, sgid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(getresgid, [](FEXCore::Core::InternalThreadState *Thread, gid_t *rgid, gid_t *egid, gid_t *sgid) -> uint64_t {
      uint64_t Result = ::getresgid(rgid, egid, sgid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(personality, [](FEXCore::Core::InternalThreadState *Thread, uint64_t persona) -> uint64_t {
      uint64_t Result = ::personality(persona);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(prctl, [](FEXCore::Core::InternalThreadState *Thread, int option, unsigned long arg2, unsigned long arg3, unsigned long arg4, unsigned long arg5) -> uint64_t {
      uint64_t Result = ::prctl(option, arg2, arg3, arg4, arg5);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(arch_prctl, [](FEXCore::Core::InternalThreadState *Thread, int code, unsigned long addr) -> uint64_t {
      uint64_t Result{};
      switch (code) {
        case 0x1001: // ARCH_SET_GS
          Thread->State.State.gs = addr;
          Result = 0;
        break;
        case 0x1002: // ARCH_SET_FS
          Thread->State.State.fs = addr;
          Result = 0;
        break;
        case 0x1003: // ARCH_GET_FS
          *reinterpret_cast<uint64_t*>(addr) = Thread->State.State.fs;
          Result = 0;
        break;
        case 0x1004: // ARCH_GET_GS
          *reinterpret_cast<uint64_t*>(addr) = Thread->State.State.gs;
          Result = 0;
        break;
        case 0x3001: // ARCH_CET_STATUS
          Result = -EINVAL; // We don't support CET, return EINVAL
        break;
        default:
          LogMan::Msg::E("Unknown prctl: 0x%x", code);
          Result = -EINVAL;
          Thread->CTX->ShouldStop = true;
        break;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(gettid, [](FEXCore::Core::InternalThreadState *Thread) -> uint64_t {
      uint64_t Result = ::gettid();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(futex, [](FEXCore::Core::InternalThreadState *Thread, int *uaddr, int futex_op, int val, const struct timespec *timeout, int *uaddr2, uint32_t val3) -> uint64_t {
      uint64_t Result = syscall(SYS_futex,
        uaddr,
        futex_op,
        val,
        timeout,
        uaddr2,
        val3);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(set_tid_address, [](FEXCore::Core::InternalThreadState *Thread, int *tidptr) -> uint64_t {
      Thread->State.ThreadManager.clear_child_tid = tidptr;
      return Thread->State.ThreadManager.GetTID();
    });

    REGISTER_SYSCALL_IMPL(exit_group, [](FEXCore::Core::InternalThreadState *Thread, int status) -> uint64_t {
      return 0;
    });

    REGISTER_SYSCALL_IMPL(set_robust_list, [](FEXCore::Core::InternalThreadState *Thread, struct robust_list_head *head, size_t len) -> uint64_t {
      Thread->State.ThreadManager.robust_list_head = reinterpret_cast<uint64_t>(head);
      uint64_t Result = ::syscall(SYS_set_robust_list, head, len);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(get_robust_list, [](FEXCore::Core::InternalThreadState *Thread, int pid, struct robust_list_head **head, size_t *len_ptr) -> uint64_t {
      uint64_t Result = ::syscall(SYS_get_robust_list, pid, head, len_ptr);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(prlimit64, [](FEXCore::Core::InternalThreadState *Thread, pid_t pid, int resource, const struct rlimit *new_limit, struct rlimit *old_limit) -> uint64_t {
      uint64_t Result = ::prlimit(pid, (enum __rlimit_resource)(resource), new_limit, old_limit);
      SYSCALL_ERRNO();
    });
  }
}
