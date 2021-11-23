/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#include "Tests/LinuxSyscalls/SignalDelegator.h"
#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Thread.h"
#include "Tests/LinuxSyscalls/x32/Types.h"

#include "Tests/LinuxSyscalls/x64/Syscalls.h"

#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/HLE/Linux/ThreadManagement.h>

#include <bits/types/siginfo_t.h>
#include <bits/types/stack_t.h>
#include <bits/types/struct_rusage.h>
#include <errno.h>
#include <grp.h>
#include <linux/futex.h>
#include <sched.h>
#include <sys/fsuid.h>
#include <sys/wait.h>
#include <syscall.h>
#include <time.h>
#include <unistd.h>
#include <vector>

ARG_TO_STR(FEX::HLE::x32::compat_ptr<FEX::HLE::x32::stack_t32>, "%x")

namespace FEX::HLE::x32 {
  uint64_t SetThreadArea(FEXCore::Core::CpuStateFrame *Frame, void *tls) {
    struct x32::user_desc* u_info = reinterpret_cast<struct x32::user_desc*>(tls);

    // The kernel only gives 32-bit userspace 3 TLS segments
    // 6 = glibc
    // 7 = wine fs
    // 8 = etc
    constexpr uint32_t NextEntry = 6;
    constexpr uint32_t MaxEntry = NextEntry+3;
    if (u_info->entry_number == -1) {
      for (uint32_t i = NextEntry; i < MaxEntry; ++i) {
        auto GDT = &Frame->State.gdt[i];
        if (GDT->base == 0) {
          // If the base is zero then it isn't present with our setup
          u_info->entry_number = i;
          break;
        }
      }

      if (u_info->entry_number == -1) {
        // Couldn't find a slot. Return empty handed
        return -ESRCH;
      }
    }

    // Now we need to update the thread's GDT to handle this change
    auto GDT = &Frame->State.gdt[u_info->entry_number];
    GDT->base = u_info->base_addr;
    return 0;
  }

  void AdjustRipForNewThread(FEXCore::Core::CpuStateFrame *Frame) {
    Frame->State.rip += 2;
  }

  void RegisterThread() {
    REGISTER_SYSCALL_IMPL_X32(clone, ([](FEXCore::Core::CpuStateFrame *Frame, uint32_t flags, void *stack, pid_t *parent_tid, void *tls, pid_t *child_tid) -> uint64_t {
      FEX::HLE::clone3_args args {
        .Type = TypeOfClone::TYPE_CLONE2,
        .args = {
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
        }
      };
      return CloneHandler(Frame, &args);
    }));

    REGISTER_SYSCALL_IMPL_X32(waitpid, [](FEXCore::Core::CpuStateFrame *Frame, pid_t pid, int32_t *status, int32_t options) -> uint64_t {
      uint64_t Result = ::waitpid(pid, status, options);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(nice, [](FEXCore::Core::CpuStateFrame *Frame, int inc) -> uint64_t {
      uint64_t Result = ::nice(inc);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(set_thread_area, [](FEXCore::Core::CpuStateFrame *Frame, struct user_desc *u_info) -> uint64_t {
      return SetThreadArea(Frame, u_info);
    });

    REGISTER_SYSCALL_IMPL_X32(set_robust_list, [](FEXCore::Core::CpuStateFrame *Frame, struct robust_list_head *head, size_t len) -> uint64_t {
      auto Thread = Frame->Thread;
      // Retain the robust list head but don't give it to the kernel
      // The kernel would break if it tried parsing a 32bit robust list from a 64bit process
      Thread->ThreadManager.robust_list_head = reinterpret_cast<uint64_t>(head);
      return 0;
    });

    REGISTER_SYSCALL_IMPL_X32(get_robust_list, [](FEXCore::Core::CpuStateFrame *Frame, int pid, struct robust_list_head **head, uint32_t *len_ptr) -> uint64_t {
      auto Thread = Frame->Thread;
      // Give the robust list back to the application
      // Steam specifically checks to make sure the robust list is set
      *(uint32_t**)head = (uint32_t*)Thread->ThreadManager.robust_list_head;
      *len_ptr = 12;
      return 0;
    });

    REGISTER_SYSCALL_IMPL_X32(futex, [](FEXCore::Core::CpuStateFrame *Frame, int *uaddr, int futex_op, int val, const timespec32 *timeout, int *uaddr2, uint32_t val3) -> uint64_t {
      void* timeout_ptr = (void*)timeout;
      struct timespec tp64{};
      int cmd = futex_op & FUTEX_CMD_MASK;
      if (timeout &&
          (cmd == FUTEX_WAIT ||
           cmd == FUTEX_LOCK_PI ||
           cmd == FUTEX_WAIT_BITSET ||
           cmd == FUTEX_WAIT_REQUEUE_PI)) {
        // timeout argument is only handled as timespec in these cases
        // Otherwise just an integer
        tp64 = *timeout;
        timeout_ptr = &tp64;
      }

      uint64_t Result = syscall(SYSCALL_DEF(futex),
        uaddr,
        futex_op,
        val,
        timeout_ptr,
        uaddr2,
        val3);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL(getgroups32, getgroups, [](FEXCore::Core::CpuStateFrame *Frame, int size, gid_t list[]) -> uint64_t {
      uint64_t Result = ::getgroups(size, list);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL(setgroups32, setgroups, [](FEXCore::Core::CpuStateFrame *Frame, size_t size, const gid_t *list) -> uint64_t {
      uint64_t Result = ::setgroups(size, list);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL(getuid32, getuid, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      uint64_t Result = ::getuid();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL(getgid32, getgid, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      uint64_t Result = ::getgid();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL(setuid32, setuid, [](FEXCore::Core::CpuStateFrame *Frame, uid_t uid) -> uint64_t {
      uint64_t Result = ::setuid(uid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL(setgid32, setgid, [](FEXCore::Core::CpuStateFrame *Frame, gid_t gid) -> uint64_t {
      uint64_t Result = ::setgid(gid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL(geteuid32, geteuid, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      uint64_t Result = ::geteuid();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL(getegid32, getegid, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      uint64_t Result = ::getegid();
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL(setfsuid32, setfsuid, [](FEXCore::Core::CpuStateFrame *Frame, uid_t fsuid) -> uint64_t {
      uint64_t Result = ::setfsuid(fsuid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL(setfsgid32, setfsgid, [](FEXCore::Core::CpuStateFrame *Frame, uid_t fsgid) -> uint64_t {
      uint64_t Result = ::setfsgid(fsgid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL(setreuid32, setreuid, [](FEXCore::Core::CpuStateFrame *Frame, uid_t ruid, uid_t euid) -> uint64_t {
      uint64_t Result = ::setreuid(ruid, euid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL(setresuid32, setresuid, [](FEXCore::Core::CpuStateFrame *Frame, uid_t ruid, uid_t euid, uid_t suid) -> uint64_t {
      uint64_t Result = ::setresuid(ruid, euid, suid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL(getresuid32, getresuid, [](FEXCore::Core::CpuStateFrame *Frame, uid_t *ruid, uid_t *euid, uid_t *suid) -> uint64_t {
      uint64_t Result = ::getresuid(ruid, euid, suid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL(setresgid32, setresgid, [](FEXCore::Core::CpuStateFrame *Frame, gid_t rgid, gid_t egid, gid_t sgid) -> uint64_t {
      uint64_t Result = ::setresgid(rgid, egid, sgid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL(getresgid32, getresgid, [](FEXCore::Core::CpuStateFrame *Frame, gid_t *rgid, gid_t *egid, gid_t *sgid) -> uint64_t {
      uint64_t Result = ::getresgid(rgid, egid, sgid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL(setregid32, setregid, [](FEXCore::Core::CpuStateFrame *Frame, gid_t rgid, gid_t egid) -> uint64_t {
      uint64_t Result = ::setregid(rgid, egid);
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(sigaltstack, [](FEXCore::Core::CpuStateFrame *Frame, const compat_ptr<stack_t32> ss, compat_ptr<stack_t32> old_ss) -> uint64_t {
      stack_t ss64{};
      stack_t old64{};

      stack_t *ss64_ptr{};
      stack_t *old64_ptr{};

      if (ss) {
        ss64 = *ss;
        ss64_ptr = &ss64;
      }

      if (old_ss) {
        old64 = *old_ss;
        old64_ptr = &old64;
      }
      uint64_t Result = FEX::HLE::_SyscallHandler->GetSignalDelegator()->RegisterGuestSigAltStack(ss64_ptr, old64_ptr);

      if (Result == 0 && old_ss) {
        *old_ss = old64;
      }
      return Result;
    });

    // launch a new process under fex
    // currently does not propagate argv[0] correctly
    REGISTER_SYSCALL_IMPL_X32(execve, [](FEXCore::Core::CpuStateFrame *Frame, const char *pathname, uint32_t *argv, uint32_t *envp) -> uint64_t {
      std::vector<const char*> Args;
      std::vector<const char*> Envp;

      if (argv) {
        for (int i = 0; argv[i]; i++) {
          Args.push_back(reinterpret_cast<const char*>(static_cast<uintptr_t>(argv[i])));
        }

        Args.push_back(nullptr);
      }

      if (envp) {
        for (int i = 0; envp[i]; i++) {
          Envp.push_back(reinterpret_cast<const char*>(static_cast<uintptr_t>(envp[i])));
        }
        Envp.push_back(nullptr);
      }

      return FEX::HLE::ExecveHandler(pathname, argv ? const_cast<char* const*>(&Args.at(0)) : nullptr, envp ? const_cast<char* const*>(&Envp.at(0)) : nullptr, nullptr);
    });

    REGISTER_SYSCALL_IMPL_X32(execveat, ([](FEXCore::Core::CpuStateFrame *Frame, int dirfd, const char *pathname, uint32_t *argv, uint32_t *envp, int flags) -> uint64_t {
      std::vector<const char*> Args;
      std::vector<const char*> Envp;

      if (argv) {
        for (int i = 0; argv[i]; i++) {
          Args.push_back(reinterpret_cast<const char*>(static_cast<uintptr_t>(argv[i])));
        }

        Args.push_back(nullptr);
      }

      if (envp) {
        for (int i = 0; envp[i]; i++) {
          Envp.push_back(reinterpret_cast<const char*>(static_cast<uintptr_t>(envp[i])));
        }
        Envp.push_back(nullptr);
      }

      FEX::HLE::ExecveAtArgs AtArgs {
        .dirfd = dirfd,
        .flags = flags,
      };

      return FEX::HLE::ExecveHandler(pathname, argv ? const_cast<char* const*>(&Args.at(0)) : nullptr, envp ? const_cast<char * const*>(&Envp.at(0)) : nullptr, &AtArgs);
    }));

    REGISTER_SYSCALL_IMPL_X32(wait4, [](FEXCore::Core::CpuStateFrame *Frame, pid_t pid, int *wstatus, int options, struct rusage_32 *rusage) -> uint64_t {
      struct rusage usage64{};
      struct rusage *usage64_p{};

      if (rusage) {
        usage64 = *rusage;
        usage64_p = &usage64;
      }
      uint64_t Result = ::wait4(pid, wstatus, options, usage64_p);
      if (rusage) {
        *rusage = usage64;
      }
      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32(waitid, [](FEXCore::Core::CpuStateFrame *Frame, int which, pid_t upid, siginfo_t *infop, int options, struct rusage_32 *rusage) -> uint64_t {
      struct rusage usage64{};
      struct rusage *usage64_p{};

      if (rusage) {
        usage64 = *rusage;
        usage64_p = &usage64;
      }

      uint64_t Result = ::syscall(SYSCALL_DEF(waitid), which, upid, infop, options, usage64_p);

      if (rusage) {
        *rusage = usage64;
      }

      SYSCALL_ERRNO();
    });

    REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL(futex_time64, futex, [](FEXCore::Core::CpuStateFrame *Frame, int *uaddr, int futex_op, int val, const struct timespec *timeout, int *uaddr2, uint32_t val3) -> uint64_t {
      uint64_t Result = syscall(SYSCALL_DEF(futex),
        uaddr,
        futex_op,
        val,
        timeout,
        uaddr2,
        val3);
      SYSCALL_ERRNO();
    });
  }
}
