/*
$info$
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Thread.h"
#include "Tests/LinuxSyscalls/Syscalls/Thread.h"

#include <FEXCore/Core/Context.h>

#include <FEXCore/Core/CodeLoader.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/Utils/LogManager.h>

#include <stdint.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>
#include <filesystem>

namespace FEX::HLE::x64 {
  uint64_t SetThreadArea(FEXCore::Core::CpuStateFrame *Frame, void *tls) {
    Frame->State.fs = reinterpret_cast<uint64_t>(tls);
    return 0;
  }

  void AdjustRipForNewThread(FEXCore::Core::CpuStateFrame *Frame) {
    Frame->State.rip += 2;
  }

  void RegisterThread() {
    REGISTER_SYSCALL_IMPL_X64(clone, ([](FEXCore::Core::CpuStateFrame *Frame, uint32_t flags, void *stack, pid_t *parent_tid, pid_t *child_tid, void *tls) -> uint64_t {
      FEX::HLE::clone3_args args {
        .flags = flags & ~CSIGNAL, // This no longer contains CSIGNAL
        .pidfd = reinterpret_cast<uint64_t>(parent_tid), // For clone, pidfd is duplicated here
        .child_tid = reinterpret_cast<uint64_t>(child_tid),
        .parent_tid = reinterpret_cast<uint64_t>(parent_tid),
        .exit_signal = flags & CSIGNAL,
        .stack = reinterpret_cast<uint64_t>(stack),
        .stack_size = ~0ULL, // This syscall isn't able to see the stack size
        .tls = reinterpret_cast<uint64_t>(tls),
        .set_tid = 0, // This syscall isn't able to select TIDs
        .set_tid_size = 0,
        .cgroup = 0, // This syscall can't select cgroups
      };
      return CloneHandler(Frame, &args);
    }));

    REGISTER_SYSCALL_IMPL_X64(futex, [](FEXCore::Core::CpuStateFrame *Frame, int *uaddr, int futex_op, int val, const struct timespec *timeout, int *uaddr2, uint32_t val3) -> uint64_t {
      uint64_t Result = syscall(SYS_futex,
        uaddr,
        futex_op,
        val,
        timeout,
        uaddr2,
        val3);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(set_robust_list, [](FEXCore::Core::CpuStateFrame *Frame, struct robust_list_head *head, size_t len) -> uint64_t {
      auto Thread = Frame->Thread;
      Thread->ThreadManager.robust_list_head = reinterpret_cast<uint64_t>(head);
      uint64_t Result = ::syscall(SYS_set_robust_list, head, len);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(get_robust_list, [](FEXCore::Core::CpuStateFrame *Frame, int pid, struct robust_list_head **head, size_t *len_ptr) -> uint64_t {
      uint64_t Result = ::syscall(SYS_get_robust_list, pid, head, len_ptr);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X64(sigaltstack, [](FEXCore::Core::CpuStateFrame *Frame, const stack_t *ss, stack_t *old_ss) -> uint64_t {
      return FEX::HLE::_SyscallHandler->GetSignalDelegator()->RegisterGuestSigAltStack(ss, old_ss);
    });

    // launch a new process under fex
    // currently does not propagate argv[0] correctly
    REGISTER_SYSCALL_IMPL_X64(execve, [](FEXCore::Core::CpuStateFrame *Frame, const char *pathname, char *const argv[], char *const envp[]) -> uint64_t {
      std::vector<const char*> Args;
      std::vector<const char*> Envp;

      for (int i = 0; argv[i]; i++) {
        Args.push_back(argv[i]);
      }

      Args.push_back(nullptr);

      for (int i = 0; envp[i]; i++) {
        Envp.push_back(envp[i]);
      }

      Envp.push_back(nullptr);

      return FEX::HLE::ExecveHandler(pathname, Args, Envp, nullptr);
    });

    REGISTER_SYSCALL_IMPL_X64(execveat, ([](FEXCore::Core::CpuStateFrame *Frame, int dirfd, const char *pathname, char *const argv[], char *const envp[], int flags) -> uint64_t {
      std::vector<const char*> Args;
      std::vector<const char*> Envp;

      for (int i = 0; argv[i]; i++) {
        Args.push_back(argv[i]);
      }

      Args.push_back(nullptr);

      for (int i = 0; envp[i]; i++) {
        Envp.push_back(envp[i]);
      }

      Envp.push_back(nullptr);

      FEX::HLE::ExecveAtArgs AtArgs {
        .dirfd = dirfd,
        .flags = flags,
      };

      return FEX::HLE::ExecveHandler(pathname, Args, Envp, &AtArgs);
    }));

    REGISTER_SYSCALL_IMPL_X64(wait4, [](FEXCore::Core::CpuStateFrame *Frame, pid_t pid, int *wstatus, int options, struct rusage *rusage) -> uint64_t {
      uint64_t Result = ::wait4(pid, wstatus, options, rusage);
      SYSCALL_ERRNO();
    });
  }
}
