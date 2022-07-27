/*
$info$
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#include "Tests/LinuxSyscalls/SignalDelegator.h"
#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Thread.h"

#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/HLE/Linux/ThreadManagement.h>
#include <FEXCore/IR/IR.h>

#include <sched.h>
#include <signal.h>
#include <stddef.h>
#include <syscall.h>
#include <stdint.h>
#include <unistd.h>
#include <vector>

namespace FEX::HLE::x64 {
  uint64_t SetThreadArea(FEXCore::Core::CpuStateFrame *Frame, void *tls) {
    Frame->State.fs = reinterpret_cast<uint64_t>(tls);
    return 0;
  }

  void AdjustRipForNewThread(FEXCore::Core::CpuStateFrame *Frame) {
    Frame->State.rip += 2;
  }

  void RegisterThread(FEX::HLE::SyscallHandler *Handler) {
    using namespace FEXCore::IR;

    REGISTER_SYSCALL_IMPL_X64_FLAGS(clone, SyscallFlags::DEFAULT,
      ([](FEXCore::Core::CpuStateFrame *Frame, uint32_t flags, void *stack, pid_t *parent_tid, pid_t *child_tid, void *tls) -> uint64_t {
      FEX::HLE::clone3_args args {
        .Type = TypeOfClone::TYPE_CLONE2,
        .args = {
          .flags = flags, // CSIGNAL is contained in here
          .pidfd = 0, // For clone, pidfd is duplicated here
          .child_tid = reinterpret_cast<uint64_t>(child_tid),
          .parent_tid = reinterpret_cast<uint64_t>(parent_tid),
          .exit_signal = flags & CSIGNAL,
          .stack = reinterpret_cast<uint64_t>(stack),
          .stack_size = 0, // This syscall isn't able to see the stack size
          .tls = reinterpret_cast<uint64_t>(tls),
          .set_tid = 0, // This syscall isn't able to select TIDs
          .set_tid_size = 0,
          .cgroup = 0, // This syscall can't select cgroups
        },
      };
      return CloneHandler(Frame, &args);
    }));

    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(futex, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int *uaddr, int futex_op, int val, const struct timespec *timeout, int *uaddr2, uint32_t val3) -> uint64_t {
      uint64_t Result = syscall(SYSCALL_DEF(futex),
        uaddr,
        futex_op,
        val,
        timeout,
        uaddr2,
        val3);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_FLAGS(set_robust_list, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, struct robust_list_head *head, size_t len) -> uint64_t {
      auto Thread = Frame->Thread;
      Thread->ThreadManager.robust_list_head = reinterpret_cast<uint64_t>(head);
#ifdef TERMUX_BUILD
      // Termux/Android doesn't support `set_robust_list` syscall.
      // The seccomp filter that the OS installs explicitly blocks this syscall from working
      // glibc uses this syscall for tls and thread data so almost every application uses it
      // Return success since we have stored the pointer ourselves.
      return 0;
#else
      uint64_t Result = ::syscall(SYSCALL_DEF(set_robust_list), head, len);
      SYSCALL_ERRNO();
#endif
    });

    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(get_robust_list, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int pid, struct robust_list_head **head, size_t *len_ptr) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(get_robust_list), pid, head, len_ptr);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(sigaltstack, [](FEXCore::Core::CpuStateFrame *Frame, const stack_t *ss, stack_t *old_ss) -> uint64_t {
      return FEX::HLE::_SyscallHandler->GetSignalDelegator()->RegisterGuestSigAltStack(ss, old_ss);
    });

    // launch a new process under fex
    // currently does not propagate argv[0] correctly
    REGISTER_SYSCALL_IMPL_X64_FLAGS(execve, SyscallFlags::DEFAULT,
      [](FEXCore::Core::CpuStateFrame *Frame, const char *pathname, char *const argv[], char *const envp[]) -> uint64_t {
      std::vector<const char*> Args;
      std::vector<const char*> Envp;

      if (argv) {
        for (int i = 0; argv[i]; i++) {
          Args.push_back(argv[i]);
        }

        Args.push_back(nullptr);
      }

      if (envp) {
        for (int i = 0; envp[i]; i++) {
          Envp.push_back(envp[i]);
        }

        Envp.push_back(nullptr);
      }

      auto* const* ArgsPtr = argv ? const_cast<char* const*>(Args.data()) : nullptr;
      auto* const* EnvpPtr = envp ? const_cast<char* const*>(Envp.data()) : nullptr;
      return FEX::HLE::ExecveHandler(pathname, ArgsPtr, EnvpPtr, nullptr);
    });

    REGISTER_SYSCALL_IMPL_X64_FLAGS(execveat, SyscallFlags::DEFAULT,
      ([](FEXCore::Core::CpuStateFrame *Frame, int dirfd, const char *pathname, char *const argv[], char *const envp[], int flags) -> uint64_t {
      std::vector<const char*> Args;
      std::vector<const char*> Envp;

      if (argv) {
        for (int i = 0; argv[i]; i++) {
          Args.push_back(argv[i]);
        }

        Args.push_back(nullptr);
      }

      if (envp) {
        for (int i = 0; envp[i]; i++) {
          Envp.push_back(envp[i]);
        }

        Envp.push_back(nullptr);
      }

      FEX::HLE::ExecveAtArgs AtArgs {
        .dirfd = dirfd,
        .flags = flags,
      };

      auto* const* ArgsPtr = argv ? const_cast<char* const*>(Args.data()) : nullptr;
      auto* const* EnvpPtr = envp ? const_cast<char* const*>(Envp.data()) : nullptr;
      return FEX::HLE::ExecveHandler(pathname, ArgsPtr, EnvpPtr, &AtArgs);
    }));

    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(wait4, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, pid_t pid, int *wstatus, int options, struct rusage *rusage) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(wait4), pid, wstatus, options, rusage);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(waitid, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
      [](FEXCore::Core::CpuStateFrame *Frame, int which, pid_t upid, siginfo_t *infop, int options, struct rusage *rusage) -> uint64_t {
      uint64_t Result = ::syscall(SYSCALL_DEF(waitid), which, upid, infop, options, rusage);
      SYSCALL_ERRNO();
    });
  }
}
