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
#include <FEXCore/Core/UContext.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/HLE/Linux/ThreadManagement.h>

#include <errno.h>
#include <grp.h>
#include <linux/futex.h>
#include <sched.h>
#include <signal.h>
#include <sys/fsuid.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <syscall.h>
#include <time.h>
#include <unistd.h>
#include <vector>

ARG_TO_STR(FEX::HLE::x32::compat_ptr<FEX::HLE::x32::stack_t32>, "%x")
ARG_TO_STR(FEX::HLE::x32::compat_ptr<FEXCore::x86::siginfo_t>, "%x")

namespace FEX::HLE::x32 {
  // The kernel only gives 32-bit userspace 3 TLS segments
  // Depending on if the host kernel is 32-bit or 64-bit then the TLS index assigned is different
  //
  // Host kernel x86_64, valid TLS enries: 12,13,14
  // Host kernel x86, valid TLS enries: 6,7,8
  // Since we are claiming to be a 64-bit kernel, use the 64-bit range
  //
  // 6/12 = glibc
  // 7/13 = wine fs
  // 8/14 = etc
  constexpr uint32_t TLS_NextEntry = 12;
  constexpr uint32_t TLS_MaxEntry = TLS_NextEntry+3;

  uint64_t SetThreadArea(FEXCore::Core::CpuStateFrame *Frame, void *tls) {
    struct x32::user_desc* u_info = reinterpret_cast<struct x32::user_desc*>(tls);
    if (u_info->entry_number == -1) {
      for (uint32_t i = TLS_NextEntry; i < TLS_MaxEntry; ++i) {
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

    // With the segment register optimization we need to check all of the segment registers and update.
    const auto GetEntry = [](auto value) {
      return value >> 3;
    };
    if (GetEntry(Frame->State.cs_idx) == u_info->entry_number) {
      Frame->State.cs_cached = GDT->base;
    }
    if (GetEntry(Frame->State.ds_idx) == u_info->entry_number) {
      Frame->State.ds_cached = GDT->base;
    }
    if (GetEntry(Frame->State.es_idx) == u_info->entry_number) {
      Frame->State.es_cached = GDT->base;
    }
    if (GetEntry(Frame->State.fs_idx) == u_info->entry_number) {
      Frame->State.fs_cached = GDT->base;
    }
    if (GetEntry(Frame->State.gs_idx) == u_info->entry_number) {
      Frame->State.gs_cached = GDT->base;
    }
    if (GetEntry(Frame->State.ss_idx) == u_info->entry_number) {
      Frame->State.ss_cached = GDT->base;
    }
    return 0;
  }

  void AdjustRipForNewThread(FEXCore::Core::CpuStateFrame *Frame) {
    Frame->State.rip += 2;
  }

  void RegisterThread(FEX::HLE::SyscallHandler *Handler) {
    REGISTER_SYSCALL_IMPL_X32(sigreturn, [](FEXCore::Core::CpuStateFrame *Frame) -> uint64_t {
      FEXCore::Context::HandleSignalHandlerReturn(Frame->Thread->CTX, false);
      FEX_UNREACHABLE;
    });

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
          .stack_size = 0, // This syscall isn't able to see the stack size
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

    REGISTER_SYSCALL_IMPL_X32(get_thread_area, [](FEXCore::Core::CpuStateFrame *Frame, struct user_desc *u_info) -> uint64_t {
      // Index to fetch comes from the user_desc
      uint32_t Entry = u_info->entry_number;
      if (Entry < TLS_NextEntry || Entry > TLS_MaxEntry) {
        return -EINVAL;
      }

      const auto &GDT = &Frame->State.gdt[Entry];

      memset(u_info, 0, sizeof(*u_info));

      // FEX only stores base instead of the full GDT
      u_info->base_addr = GDT->base;

      // Fill the rest of the structure with expected data (even if wrong at the moment)
      if (u_info->base_addr) {
        u_info->limit = 0xF'FFFF;
        u_info->seg_32bit = 1;
        u_info->limit_in_pages = 1;
        u_info->useable = 1;
      }
      else {
        u_info->read_exec_only = 1;
        u_info->seg_not_present = 1;
      }
      return 0;
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

      auto* const* ArgsPtr = argv ? const_cast<char* const*>(Args.data()) : nullptr;
      auto* const* EnvpPtr = envp ? const_cast<char* const*>(Envp.data()) : nullptr;

      FEX::HLE::ExecveAtArgs AtArgs = FEX::HLE::ExecveAtArgs::Empty();

      return FEX::HLE::ExecveHandler(pathname, ArgsPtr, EnvpPtr, AtArgs);
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

      auto* const* ArgsPtr = argv ? const_cast<char* const*>(Args.data()) : nullptr;
      auto* const* EnvpPtr = envp ? const_cast<char* const*>(Envp.data()) : nullptr;
      return FEX::HLE::ExecveHandler(pathname, ArgsPtr, EnvpPtr, AtArgs);
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

    REGISTER_SYSCALL_IMPL_X32(waitid, [](FEXCore::Core::CpuStateFrame *Frame, int which, pid_t upid, compat_ptr<FEXCore::x86::siginfo_t> info, int options, struct rusage_32 *rusage) -> uint64_t {
      struct rusage usage64{};
      struct rusage *usage64_p{};

      siginfo_t info64{};
      siginfo_t *info64_p{};

      if (rusage) {
        usage64 = *rusage;
        usage64_p = &usage64;
      }

      if (info) {
        info64_p = &info64;
      }

      uint64_t Result = ::syscall(SYSCALL_DEF(waitid), which, upid, info64_p, options, usage64_p);

      if (Result != -1) {
        if (rusage) {
          *rusage = usage64;
        }

        if (info) {
          *info = info64;
        }
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
