#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Thread.h"
#include "Tests/LinuxSyscalls/Syscalls/Thread.h"

#include <FEXCore/Core/Context.h>

#include <FEXCore/Core/CodeLoader.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/Utils/ELFLoader.h>
#include <FEXCore/Utils/LogManager.h>

#include <stdint.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>
#include <filesystem>

namespace FEX::HLE::x64 {
  uint64_t SetThreadArea(FEXCore::Core::InternalThreadState *Thread, void *tls) {
    Thread->State.State.fs = reinterpret_cast<uint64_t>(tls);
    return 0;
  }

  void AdjustRipForNewThread(FEXCore::Core::CPUState *Thread) {
    Thread->rip += 2;
  }

  static bool AnyFlagsSet(uint64_t Flags, uint64_t Mask) {
    return (Flags & Mask) != 0;
  }

  static bool AllFlagsSet(uint64_t Flags, uint64_t Mask) {
    return (Flags & Mask) == Mask;
  }

  void RegisterThread() {
    REGISTER_SYSCALL_IMPL_X64(futex, [](FEXCore::Core::InternalThreadState *Thread, int *uaddr, int futex_op, int val, const struct timespec *timeout, int *uaddr2, uint32_t val3) -> uint64_t {
      uint64_t Result = syscall(SYS_futex,
        uaddr,
        futex_op,
        val,
        timeout,
        uaddr2,
        val3);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(clone, [](FEXCore::Core::InternalThreadState *Thread, uint32_t flags, void *stack, pid_t *parent_tid, pid_t *child_tid, void *tls) -> uint64_t {
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

        return FEX::HLE::ForkGuest(Thread, flags, stack, parent_tid, child_tid, tls);
      } else {

        if (!AllFlagsSet(flags, CLONE_SYSVSEM | CLONE_FS |  CLONE_FILES | CLONE_SIGHAND)) {
          ERROR_AND_DIE("clone: CLONE_THREAD: Unsuported flags w/ CLONE_THREAD (Shared Resources), %X", flags);
        }

        auto NewThread = FEX::HLE::CreateNewThread(Thread, flags, stack, parent_tid, child_tid, tls);

        // Return the new threads TID
        uint64_t Result = NewThread->State.ThreadManager.GetTID();
        LogMan::Msg::D("Child [%d] starting at: 0x%lx. Parent was at 0x%lx", Result, NewThread->State.State.rip, Thread->State.State.rip);

        // Actually start the thread
        FEXCore::Context::RunThread(Thread->CTX, NewThread);

        if (flags & CLONE_VFORK) {
          // If VFORK is set then the calling process is suspended until the thread exits with execve or exit
          NewThread->ExecutionThread.join();
        }
        SYSCALL_ERRNO();
      }
    });

    REGISTER_SYSCALL_IMPL_X64(set_robust_list, [](FEXCore::Core::InternalThreadState *Thread, struct robust_list_head *head, size_t len) -> uint64_t {
      Thread->State.ThreadManager.robust_list_head = reinterpret_cast<uint64_t>(head);
      uint64_t Result = ::syscall(SYS_set_robust_list, head, len);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(get_robust_list, [](FEXCore::Core::InternalThreadState *Thread, int pid, struct robust_list_head **head, size_t *len_ptr) -> uint64_t {
      uint64_t Result = ::syscall(SYS_get_robust_list, pid, head, len_ptr);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(sigaltstack, [](FEXCore::Core::InternalThreadState *Thread, const stack_t *ss, stack_t *old_ss) -> uint64_t {
      return FEX::HLE::_SyscallHandler->GetSignalDelegator()->RegisterGuestSigAltStack(ss, old_ss);
    });

    // launch a new process under fex
    // currently does not propagate argv[0] correctly
    REGISTER_SYSCALL_IMPL_X64(execve, [](FEXCore::Core::InternalThreadState *Thread, const char *pathname, char *const argv[], char *const envp[]) -> uint64_t {
      std::vector<const char*> Args;
      std::string Filename{};

      std::error_code ec;
      // Check the rootfs if it is available first
      if (pathname[0] == '/') {
        Filename = FEX::HLE::_SyscallHandler->RootFSPath() + pathname;

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
      if (FEX::HLE::_SyscallHandler->IsInterpreter()) {
        if (FEX::HLE::_SyscallHandler->IsInterpreterInstalled() && ELFLoader::ELFContainer::IsSupportedELF(Filename.c_str())) {
          Result = execve(Filename.c_str(), argv, envp);
          SYSCALL_ERRNO();
        }
        else {
          // Otherwise we need to fall down the interpreter unsupported code path
          FEX::HLE::_SyscallHandler->GetCodeLoader()->GetExecveArguments(&Args);
        }
      }
      else {
        FEX::HLE::_SyscallHandler->GetCodeLoader()->GetExecveArguments(&Args);
        Args.push_back("--");
      }

      Args.push_back(Filename.c_str());

      for (int i = 0; argv[i]; i++) {
        if (i == 0)
          continue;

        Args.push_back(argv[i]);
      }

      Args.push_back(nullptr);
      Result = execve("/proc/self/exe", const_cast<char *const *>(&Args[0]), envp);

      SYSCALL_ERRNO();
    });
  }
}
