// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#include "ArchHelpers/UContext.h"
#include "LinuxSyscalls/SignalDelegator.h"
#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x32/Syscalls.h"
#include "LinuxSyscalls/x32/Thread.h"
#include "LinuxSyscalls/x32/Types.h"

#include "LinuxSyscalls/x64/Syscalls.h"

#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/fextl/vector.h>

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
constexpr uint32_t TLS_MaxEntry = TLS_NextEntry + 3;

uint64_t SetThreadArea(FEXCore::Core::CpuStateFrame* Frame, void* tls) {
  struct x32::user_desc* u_info = reinterpret_cast<struct x32::user_desc*>(tls);
  FaultSafeUserMemAccess::VerifyIsReadable(u_info, sizeof(*u_info));

  if (u_info->entry_number == -1) {
    for (uint32_t i = TLS_NextEntry; i < TLS_MaxEntry; ++i) {
      auto GDT = &Frame->State.gdt[i];
      if (Frame->State.CalculateGDTLimit(*GDT) == 0) {
        // If the limit is zero then it isn't present with our setup
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
  Frame->State.SetGDTBase(GDT, u_info->base_addr);
  Frame->State.SetGDTLimit(GDT, 0xF'FFFFU);

  // With the segment register optimization we need to check all of the segment registers and update.
  const auto GetEntry = [](auto value) {
    return value >> 3;
  };
  if (GetEntry(Frame->State.cs_idx) == u_info->entry_number) {
    Frame->State.cs_cached = Frame->State.CalculateGDTBase(*GDT);
  }
  if (GetEntry(Frame->State.ds_idx) == u_info->entry_number) {
    Frame->State.ds_cached = Frame->State.CalculateGDTBase(*GDT);
  }
  if (GetEntry(Frame->State.es_idx) == u_info->entry_number) {
    Frame->State.es_cached = Frame->State.CalculateGDTBase(*GDT);
  }
  if (GetEntry(Frame->State.fs_idx) == u_info->entry_number) {
    Frame->State.fs_cached = Frame->State.CalculateGDTBase(*GDT);
  }
  if (GetEntry(Frame->State.gs_idx) == u_info->entry_number) {
    Frame->State.gs_cached = Frame->State.CalculateGDTBase(*GDT);
  }
  if (GetEntry(Frame->State.ss_idx) == u_info->entry_number) {
    Frame->State.ss_cached = Frame->State.CalculateGDTBase(*GDT);
  }
  return 0;
}

void AdjustRipForNewThread(FEXCore::Core::CpuStateFrame* Frame) {
  Frame->State.rip += 2;
}

void RegisterThread(FEX::HLE::SyscallHandler* Handler) {
  REGISTER_SYSCALL_IMPL_X32(sigreturn, [](FEXCore::Core::CpuStateFrame* Frame) -> uint64_t {
    FEX::HLE::_SyscallHandler->GetSignalDelegator()->HandleSignalHandlerReturn(false);
    FEX_UNREACHABLE;
  });

  REGISTER_SYSCALL_IMPL_X32(
    clone, ([](FEXCore::Core::CpuStateFrame* Frame, uint32_t flags, void* stack, pid_t* parent_tid, void* tls, pid_t* child_tid) -> uint64_t {
      // This is slightly different EFAULT behaviour, if child_tid or parent_tid is invalid then the kernel just doesn't write to the
      // pointer. Still need to be EFAULT safe although.
      if ((flags & (CLONE_CHILD_SETTID | CLONE_CHILD_CLEARTID)) && child_tid) {
        FaultSafeUserMemAccess::VerifyIsWritable(child_tid, sizeof(*child_tid));
      }

      if ((flags & CLONE_PARENT_SETTID) && parent_tid) {
        FaultSafeUserMemAccess::VerifyIsWritable(parent_tid, sizeof(*parent_tid));
      }


      FEX::HLE::clone3_args args {.Type = TypeOfClone::TYPE_CLONE2,
                                  .args = {
                                    .flags = flags & ~CSIGNAL,                       // This no longer contains CSIGNAL
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
                                  }};
      return CloneHandler(Frame, &args);
    }));

  REGISTER_SYSCALL_IMPL_X32(waitpid, [](FEXCore::Core::CpuStateFrame* Frame, pid_t pid, int32_t* status, int32_t options) -> uint64_t {
    uint64_t Result = ::waitpid(pid, status, options);
    FaultSafeUserMemAccess::VerifyIsWritableOrNull(status, sizeof(*status));
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(nice, [](FEXCore::Core::CpuStateFrame* Frame, int inc) -> uint64_t {
    uint64_t Result = ::nice(inc);
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(
    set_thread_area, [](FEXCore::Core::CpuStateFrame* Frame, struct user_desc* u_info) -> uint64_t { return SetThreadArea(Frame, u_info); });

  REGISTER_SYSCALL_IMPL_X32(get_thread_area, [](FEXCore::Core::CpuStateFrame* Frame, struct user_desc* u_info) -> uint64_t {
    // Index to fetch comes from the user_desc
    uint32_t Entry = u_info->entry_number;
    if (Entry < TLS_NextEntry || Entry > TLS_MaxEntry) {
      return -EINVAL;
    }

    FaultSafeUserMemAccess::VerifyIsWritable(u_info, sizeof(*u_info));

    const auto& GDT = &Frame->State.gdt[Entry];

    memset(u_info, 0, sizeof(*u_info));

    // FEX only stores base instead of the full GDT
    u_info->base_addr = Frame->State.CalculateGDTBase(*GDT);

    // Fill the rest of the structure with expected data (even if wrong at the moment)
    if (u_info->base_addr) {
      u_info->limit = 0xF'FFFF;
      u_info->seg_32bit = 1;
      u_info->limit_in_pages = 1;
      u_info->useable = 1;
    } else {
      u_info->read_exec_only = 1;
      u_info->seg_not_present = 1;
    }
    return 0;
  });

  REGISTER_SYSCALL_IMPL_X32(set_robust_list, [](FEXCore::Core::CpuStateFrame* Frame, struct robust_list_head* head, size_t len) -> uint64_t {
    if (len != 12) {
      // Return invalid if the passed in length doesn't match what's expected.
      return -EINVAL;
    }

    auto ThreadObject = FEX::HLE::ThreadManager::GetStateObjectFromCPUState(Frame);
    // Retain the robust list head but don't give it to the kernel
    // The kernel would break if it tried parsing a 32bit robust list from a 64bit process
    ThreadObject->ThreadInfo.robust_list_head = reinterpret_cast<uint64_t>(head);
    return 0;
  });

  REGISTER_SYSCALL_IMPL_X32(
    get_robust_list, [](FEXCore::Core::CpuStateFrame* Frame, int pid, struct robust_list_head** head, uint32_t* len_ptr) -> uint64_t {
      FaultSafeUserMemAccess::VerifyIsWritable(head, sizeof(uint32_t));
      FaultSafeUserMemAccess::VerifyIsWritable(len_ptr, sizeof(*len_ptr));

      auto ThreadObject = FEX::HLE::ThreadManager::GetStateObjectFromCPUState(Frame);
      // Give the robust list back to the application
      // Steam specifically checks to make sure the robust list is set
      *(uint32_t*)head = (uint32_t)ThreadObject->ThreadInfo.robust_list_head;
      *len_ptr = 12;
      return 0;
    });

  REGISTER_SYSCALL_IMPL_X32(
    futex, [](FEXCore::Core::CpuStateFrame* Frame, int* uaddr, int futex_op, int val, const timespec32* timeout, int* uaddr2, uint32_t val3) -> uint64_t {
      void* timeout_ptr = (void*)timeout;
      struct timespec tp64 {};
      int cmd = futex_op & FUTEX_CMD_MASK;
      if (timeout && (cmd == FUTEX_WAIT || cmd == FUTEX_LOCK_PI || cmd == FUTEX_WAIT_BITSET || cmd == FUTEX_WAIT_REQUEUE_PI)) {
        FaultSafeUserMemAccess::VerifyIsReadable(timeout, sizeof(*timeout));
        // timeout argument is only handled as timespec in these cases
        // Otherwise just an integer
        tp64 = *timeout;
        timeout_ptr = &tp64;
      }

      uint64_t Result = syscall(SYSCALL_DEF(futex), uaddr, futex_op, val, timeout_ptr, uaddr2, val3);
      SYSCALL_ERRNO();
    });

  REGISTER_SYSCALL_IMPL_X32(
    sigaltstack, [](FEXCore::Core::CpuStateFrame* Frame, const compat_ptr<stack_t32> ss, compat_ptr<stack_t32> old_ss) -> uint64_t {
      stack_t ss64 {};
      stack_t old64 {};

      stack_t* ss64_ptr {};
      stack_t* old64_ptr {};

      if (ss) {
        FaultSafeUserMemAccess::VerifyIsReadable(ss, sizeof(*ss));
        ss64 = *ss;
        ss64_ptr = &ss64;
      }

      if (old_ss) {
        FaultSafeUserMemAccess::VerifyIsReadable(old_ss, sizeof(*old_ss));
        old64 = *old_ss;
        old64_ptr = &old64;
      }
      uint64_t Result = FEX::HLE::_SyscallHandler->GetSignalDelegator()->RegisterGuestSigAltStack(
        FEX::HLE::ThreadManager::GetStateObjectFromCPUState(Frame), ss64_ptr, old64_ptr);

      if (Result == 0 && old_ss) {
        FaultSafeUserMemAccess::VerifyIsWritable(old_ss, sizeof(*old_ss));
        *old_ss = old64;
      }
      return Result;
    });

  // launch a new process under fex
  // currently does not propagate argv[0] correctly
  REGISTER_SYSCALL_IMPL_X32(execve, [](FEXCore::Core::CpuStateFrame* Frame, const char* pathname, uint32_t* argv, uint32_t* envp) -> uint64_t {
    fextl::vector<const char*> Args;
    fextl::vector<const char*> Envp;

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

    return FEX::HLE::ExecveHandler(Frame, pathname, ArgsPtr, EnvpPtr, AtArgs);
  });

  REGISTER_SYSCALL_IMPL_X32(
    execveat, ([](FEXCore::Core::CpuStateFrame* Frame, int dirfd, const char* pathname, uint32_t* argv, uint32_t* envp, int flags) -> uint64_t {
      fextl::vector<const char*> Args;
      fextl::vector<const char*> Envp;

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
      return FEX::HLE::ExecveHandler(Frame, pathname, ArgsPtr, EnvpPtr, AtArgs);
    }));

  REGISTER_SYSCALL_IMPL_X32(wait4, [](FEXCore::Core::CpuStateFrame* Frame, pid_t pid, int* wstatus, int options, struct rusage_32* rusage) -> uint64_t {
    struct rusage usage64 {};
    struct rusage* usage64_p {};

    if (rusage) {
      FaultSafeUserMemAccess::VerifyIsReadable(rusage, sizeof(*rusage));
      usage64 = *rusage;
      usage64_p = &usage64;
    }
    uint64_t Result = ::wait4(pid, wstatus, options, usage64_p);
    if (rusage) {
      FaultSafeUserMemAccess::VerifyIsWritable(rusage, sizeof(*rusage));
      *rusage = usage64;
    }
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_X32(waitid,
                            [](FEXCore::Core::CpuStateFrame* Frame, int which, pid_t upid, compat_ptr<FEXCore::x86::siginfo_t> info,
                               int options, struct rusage_32* rusage) -> uint64_t {
                              struct rusage usage64 {};
                              struct rusage* usage64_p {};

                              siginfo_t info64 {};
                              siginfo_t* info64_p {};

                              if (rusage) {
                                FaultSafeUserMemAccess::VerifyIsReadable(rusage, sizeof(*rusage));
                                usage64 = *rusage;
                                usage64_p = &usage64;
                              }

                              if (info) {
                                info64_p = &info64;
                              }

                              uint64_t Result = ::syscall(SYSCALL_DEF(waitid), which, upid, info64_p, options, usage64_p);

                              if (Result != -1) {
                                if (rusage) {
                                  FaultSafeUserMemAccess::VerifyIsWritable(rusage, sizeof(*rusage));
                                  *rusage = usage64;
                                }

                                if (info) {
                                  FaultSafeUserMemAccess::VerifyIsWritable(info, sizeof(*info));
                                  *info = info64;
                                }
                              }

                              SYSCALL_ERRNO();
                            });
}
} // namespace FEX::HLE::x32
