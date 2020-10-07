#include "Interface/Context/Context.h"
#include "Interface/Core/InternalThreadState.h"
#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"
#include "Interface/HLE/x32/Syscalls.h"

#include <FEXCore/Core/CodeLoader.h>
#include <FEXCore/Core/X86Enums.h>

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
#include <filesystem>

ARG_TO_STR(idtype_t, "%u")

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

  uint64_t ForkGuest(FEXCore::Core::InternalThreadState *Thread, uint32_t flags, void *stack, pid_t *parent_tid, pid_t *child_tid, void *tls) {
    pid_t Result = fork();

    if (Result == 0) {
      // Child
      // update the internal TID
      Thread->State.ThreadManager.TID = ::gettid();
      Thread->State.ThreadManager.PID = ::getpid();
      Thread->State.ThreadManager.clear_child_tid = nullptr;

      // Clear all the other threads that are being tracked
      for (auto &DeadThread : Thread->CTX->Threads) {
        if (DeadThread == Thread) {
          continue;
        }

        // Setting running to false ensures that when they shutdown we won't send signals to kill them
        DeadThread->State.RunningEvents.Running = false;
      }

      // only a  single thread running so no need to remove anything from the thread array

      // Handle child setup now
      if (stack != nullptr) {
        // use specified stack
        LogMan::Msg::D("@@@@@@@ Fork uses custom stack");
        Thread->State.State.gregs[X86State::REG_RSP] = reinterpret_cast<uint64_t>(stack);
      } else {
        // In the case of fork and nullptr stack then the child uses the same stack space as the parent
        // Same virtual address, different addressspace
        LogMan::Msg::D("@@@@@@@ Fork uses parent stack");
      }

      if (flags & CLONE_SETTLS) {
        Thread->State.State.fs = reinterpret_cast<uint64_t>(tls);
      }

      // Sets the child TID to the pointer in ChildTID
      if (flags & CLONE_CHILD_SETTID) {
        Thread->State.ThreadManager.set_child_tid = child_tid;
        *child_tid = Thread->State.ThreadManager.TID;
      }

      // When the thread exits, clear the child thread ID at ChildTID
      // Additionally wakeup a futex at that address
      // Address /may/ be changed with SET_TID_ADDRESS syscall
      if (flags & CLONE_CHILD_CLEARTID) {
        Thread->State.ThreadManager.clear_child_tid = child_tid;
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

  bool AnyFlagsSet(uint64_t Flags, uint64_t Mask) {
    return (Flags & Mask) != 0;
  }

  bool AllFlagsSet(uint64_t Flags, uint64_t Mask) {
    return (Flags & Mask) == Mask;
  }

  void RegisterThread() {
    REGISTER_SYSCALL_IMPL(getpid, [](FEXCore::Core::InternalThreadState *Thread) -> uint64_t {
      uint64_t Result = ::getpid();
      SYSCALL_ERRNO();
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

      if (AnyFlagsSet(flags, CLONE_UNTRACED | CLONE_PTRACE)) {
        LogMan::Msg::D("clone: Ptrace* not supported");
      }

      if (AnyFlagsSet(flags, CLONE_NEWNS | CLONE_NEWCGROUP | CLONE_NEWUTS | CLONE_NEWIPC | CLONE_NEWUSER | CLONE_NEWPID | CLONE_NEWNET)) {
        ERROR_AND_DIE("clone: Namespaces are not supported");
      }

      if (!(flags & CLONE_THREAD)) {

        if (flags & CLONE_VFORK) {
          flags &= ~CLONE_VFORK;
          flags &= ~CLONE_VM;
          LogMan::Msg::D("clone: WARNING: CLONE_VFORK w/o CLONE_THREAD");
        }

        if (AnyFlagsSet(flags, CLONE_SYSVSEM | CLONE_FS |  CLONE_FILES | CLONE_SIGHAND | CLONE_VM)) {
          ERROR_AND_DIE("clone: Unsuported flags w/o CLONE_THREAD (Shared Resources), %X", flags);
        }

        // CLONE_PARENT is ignored (Implied by CLONE_THREAD)

        if (Thread->CTX->GetThreadCount() != 1) {
          LogMan::Msg::E("clone: Fork only supported on single threaded applications. Allowing");
        } else {
          LogMan::Msg::D("clone: Forking process");
        }

        return ForkGuest(Thread, flags, stack, parent_tid, child_tid, tls);
      } else {

        if (!AllFlagsSet(flags, CLONE_SYSVSEM | CLONE_FS |  CLONE_FILES | CLONE_SIGHAND)) {
          ERROR_AND_DIE("clone: CLONE_THREAD: Unsuported flags w/ CLONE_THREAD (Shared Resources), %X", flags);
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
        SYSCALL_ERRNO();
      }
    });


    REGISTER_SYSCALL_IMPL(fork, [](FEXCore::Core::InternalThreadState *Thread) -> uint64_t {
      if (Thread->CTX->GetThreadCount() != 1) {
        LogMan::Msg::E("fork: Fork only supported on single threaded applications. Allowing");
      } else {
        LogMan::Msg::D("fork: Forking process");
      }

      return ForkGuest(Thread, 0, 0, 0, 0, 0);
    });

    REGISTER_SYSCALL_IMPL(vfork, [](FEXCore::Core::InternalThreadState *Thread) -> uint64_t {
      if (Thread->CTX->GetThreadCount() != 1) {
        LogMan::Msg::E("vfork: Fork only supported on single threaded applications. Allowing");
      } else {
        LogMan::Msg::D("vfork: WARNING: Forking process using fork semantics");
      }
      return ForkGuest(Thread, 0, 0, 0, 0, 0);
    });

    // launch a new process under fex
    // currently does not propagate argv[0] correctly
    REGISTER_SYSCALL_IMPL(execve, [](FEXCore::Core::InternalThreadState *Thread, const char *pathname, char *const argv[], char *const envp[]) -> uint64_t {
      std::vector<const char*> Args;

      std::error_code ec;
      bool exists = std::filesystem::exists(pathname, ec);
      if (ec || !exists) {
        return -ENOENT;
      }

      Thread->CTX->GetCodeLoader()->GetExecveArguments(&Args);

      Args.push_back("--");

      Args.push_back(pathname);

      for (int i = 0; argv[i]; i++) {
        if (i == 0)
          continue;

          Args.push_back(argv[i]);
      }

      Args.push_back(nullptr);

      uint64_t Result = execve("/proc/self/exe", const_cast<char *const *>(&Args[0]), envp);

      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(exit, [](FEXCore::Core::InternalThreadState *Thread, int status) -> uint64_t {
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
      Thread->CTX->StopThread(Thread);

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

    REGISTER_SYSCALL_IMPL(getgroups, [](FEXCore::Core::InternalThreadState *Thread, int size, gid_t list[]) -> uint64_t {
      uint64_t Result = ::getgroups(size, list);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(setgroups, [](FEXCore::Core::InternalThreadState *Thread, size_t size, const gid_t *list) -> uint64_t {
      uint64_t Result = ::setgroups(size, list);
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
      Thread->StatusCode = status;
      Thread->CTX->Stop(false /* Ignore current thread */);
      // This will never be reached
      std::unexpected();
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

    /*
    REGISTER_SYSCALL_IMPL(setpgid, [](FEXCore::Core::InternalThreadState *Thread, pid_t pid, pid_t pgid) -> uint64_t {
      SYSCALL_STUB(setpgid);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(setpgid);

    /*REGISTER_SYSCALL_IMPL(getpgid, [](FEXCore::Core::InternalThreadState *Thread, pid_t pid) -> uint64_t {
      SYSCALL_STUB(getpgid);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(getpgid);

    /*REGISTER_SYSCALL_IMPL(setfsuid, [](FEXCore::Core::InternalThreadState *Thread, uid_t fsuid) -> uint64_t {
      SYSCALL_STUB(setfsuid);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(setfsuid);

    /*REGISTER_SYSCALL_IMPL(setfsgid, [](FEXCore::Core::InternalThreadState *Thread, uid_t fsgid) -> uint64_t {
      SYSCALL_STUB(setfsgid);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(setfsgid);

    /*REGISTER_SYSCALL_IMPL(getsid, [](FEXCore::Core::InternalThreadState *Thread, pid_t pid) -> uint64_t {
      SYSCALL_STUB(getsid);
    });*/
    REGISTER_SYSCALL_FORWARD_ERRNO(getsid);

    REGISTER_SYSCALL_IMPL(waitid, [](FEXCore::Core::InternalThreadState *Thread, idtype_t idtype, id_t id, siginfo_t *infop, int options) -> uint64_t {
      uint64_t Result = ::waitid(idtype, id, infop, options);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(unshare, [](FEXCore::Core::InternalThreadState *Thread, int flags) -> uint64_t {
      uint64_t Result = ::unshare(flags);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL(setns, [](FEXCore::Core::InternalThreadState *Thread, int fd, int nstype) -> uint64_t {
      uint64_t Result = ::setns(fd, nstype);
      SYSCALL_ERRNO();
    });
  }
}
