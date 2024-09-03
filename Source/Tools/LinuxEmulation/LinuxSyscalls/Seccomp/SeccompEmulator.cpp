// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "LinuxSyscalls/Seccomp/BPFEmitter.h"
#include "LinuxSyscalls/Seccomp/SeccompEmulator.h"

#include "LinuxSyscalls/x32/Syscalls.h"
#include "LinuxSyscalls/x64/Syscalls.h"
#include "LinuxSyscalls/SignalDelegator.h"

#include <CodeEmitter/Emitter.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/Utils/LogManager.h>

#include <fcntl.h>
#include <linux/audit.h>
#include <linux/bpf_common.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>

// seccomp
//
// global
// - kcmp                              - pass
// - mode_strict_support               - pass
// - mode_strict_cannot_call_prctl     - pass
// - no_new_privs_support              - pass
// - mode_filter_support               - pass
// - mode_filter_without_nnp           - pass
// - filter_size_limits                - pass
// - filter_chain_limits               - pass
// - mode_filter_cannot_move_to_strict - pass
// - mode_filter_get_seccomp           - pass
// - ALLOW_all                         - pass
// - empty_prog                        - pass
// - log_all                           - pass
// - unknown_ret_is_kill_inside        - pass
// - unknown_ret_is_kill_above_allow   - pass
// - KILL_all                          - pass
// - KILL_one                          - pass
// - KILL_one_arg_one                  - pass
// - KILL_one_arg_six                  - pass
// - KILL_thread                       - FAIL (unrelated to bpf)
// - KILL_process                      - FAIL (unrelated to bpf)
// - KILL_unknown                      - FAIL (unrelated to bpf)
// - arg_out_of_range                  - pass
// - ERRNO_valid                       - pass
// - ERRNO_zero                        - pass
// - ERRNO_capped                      - pass
// - ERRNO_order                       - pass
// - seccomp_syscall                   - pass
// - seccomp_syscall_mode_lock         - pass
// - detect_seccomp_filter_flags       - pass
// - TSYNC_first                       - pass
// - syscall_restart                   - FAIL (PTRACE)
// - filter_flag_log                   - pass
// - get_action_avail                  - FAIL (ptrace and user-notif)
// TSYNC
// - siblings_fail_prctl               - pass
// - two_siblings_with_ancestor        - FAIL (kill-thread not working quite right)
// - two_sibling_want_nnp              - pass
// - two_siblings_with_one_divergence  - pass
// - two_siblings_with_one_divergence_no_tid_in_err - pass
// - two_siblings_not_under_filter     - FAIL (kill-thread not working quite right)
// - two_siblings_with_no_filter       - FAIL (kill-thread not working quite right)
//
// user-notif stuff
// - get_metadata                      - SKIP (Needs root)
// - user_notification_basic           - FAIL (user-notif)
// - user_notification_with_tsync      - FAIL (user-notif)
// - user_notification_kill_in_middle  - FAIL (user-notif)
// - user_notification_signal          - FAIL (user-notif)
// - user_notification_closed_listener - FAIL (user-notif)
// - user_notification_child_pid_ns    - FAIL (user-notif)
// - user_notification_sibling_pid_ns  - FAIL (user-notif)
// - user_notification_fault_recv      - FAIL (user-notif)
// - seccomp_get_notif_sizes           - pass
// - user_notification_continue        - FAIL (user-notif)
// - user_notification_filter_empty    - FAIL (user-notif)
// - user_notification_filter_empty_threaded - FAIL (user-notif)
// - user_notification_addfd           - FAIL (user-notif)
// - user_notification_addfd_rlimit    - FAIL (user-notif)
// - user_notification_sync            - FAIL (user-notif)
// - user_notification_fifo            - FAIL (user-notif)
// - user_notification_wait_killable_pre_notification - FAIL (user-notif)
// - user_notification_wait_killable   - FAIL (user-notif)
// - user_notification_wait_killable_fatal - FAIL (user-notif)
//
// O_SUSPEND_SECCOMP
// - setoptions - FAIL (ptrace)
// - seize      - FAIL (ptrace)
// TRAP
// - dfl     - pass
// - ign     - pass
// - handler - pass
//
// precedence
// - allow_ok                     - pass
// - kill_is_highest              - pass
// - kill_is_highest_in_any_order - pass
// - trap_is_second               - pass
// - trap_is_second_in_any_order  - pass
// - errno_is_third               - pass
// - errno_is_third_in_any_order  - pass
// - trace_is_fourth              - pass
// - trace_is_fourth_in_any_order - pass
// - log_is_fifth                 - pass
// - log_is_fifth_in_any_order    - pass
//
// TRACE_poke
// - ptrace unsupported
// TRACE_syscall
// - ptrace unsupported

namespace FEX::HLE {
uint64_t SeccompEmulator::Handle(FEXCore::Core::CpuStateFrame* Frame, uint32_t Op, uint32_t flags, void* arg) {
  // If seccomp isn't enabled then say so.
  if (!NeedsSeccomp) {
    return -EINVAL;
  }

  switch (Op) {
  case SECCOMP_SET_MODE_STRICT: return SetModeStrict(Frame, flags, arg);
  case SECCOMP_SET_MODE_FILTER: return SetModeFilter(Frame, flags, static_cast<const sock_fprog*>(arg));
  case SECCOMP_GET_ACTION_AVAIL: return GetActionAvail(flags, static_cast<const uint32_t*>(arg));
  case SECCOMP_GET_NOTIF_SIZES: return GetNotifSizes(flags, static_cast<struct seccomp_notif_sizes*>(arg));
  default:
    // operation is unknown or is not supported by this kernel version or configuration.
    return -EINVAL;
  }
}

// Equivalent to prctl(PR_GET_SECCOMP)
uint64_t SeccompEmulator::GetSeccomp(FEXCore::Core::CpuStateFrame* Frame) {
  // If seccomp isn't enabled then say so.
  if (!NeedsSeccomp) {
    return -EINVAL;
  }

  auto Thread = FEX::HLE::ThreadManager::GetStateObjectFromCPUState(Frame);
  return Thread->SeccompMode;
}

void SeccompEmulator::InheritSeccompFilters(FEX::HLE::ThreadStateObject* Parent, FEX::HLE::ThreadStateObject* Child) {
  // Don't interrupt me while I'm copying.
  auto lk = FEXCore::MaskSignalsAndLockMutex(FilterMutex);

  Child->Filters.resize(Parent->Filters.size());

  for (size_t i = 0; i < Child->Filters.size(); ++i) {
    auto& ParentFilter = Parent->Filters[i];
    auto& ChildFilter = Child->Filters[i];
    ChildFilter = ParentFilter;
    std::atomic_ref<uint64_t>(ParentFilter->RefCount)++;
  }

  // Copy the operating mode.
  Child->SeccompMode = Parent->SeccompMode;
}

void SeccompEmulator::FreeSeccompFilters(FEX::HLE::ThreadStateObject* Thread) {
  // Don't talk to me when I'm busy deleting myself.
  auto lk = FEXCore::MaskSignalsAndLockMutex(FilterMutex);

  bool HasFiltersToDelete {};
  for (auto& Filter : Thread->Filters) {
    auto RefCount = std::atomic_ref<uint64_t>(Filter->RefCount).fetch_sub(1);

    if (RefCount == 1) {
      HasFiltersToDelete = true;
    }
  }
  Thread->Filters.clear();

  if (HasFiltersToDelete) {
    // Garbage collect filters
    std::erase_if(Filters, [](auto& Filter) {
      if (std::atomic_ref<uint64_t>(Filter.RefCount).load(std::memory_order_relaxed) != 0) {
        return false;
      }

      FEXCore::Allocator::munmap(reinterpret_cast<void*>(Filter.Func), Filter.MappedSize);
      return true;
    });
  }
}

struct SerializedFilter {
  size_t CodeSize;
  uint32_t FilterInstructions;
  bool ShouldLog;
  char Code[];
};

struct SerializationHeader {
  size_t NumberOfFilters;
  uint32_t SeccompMode;
  SerializedFilter Filters[];
};

std::optional<int> SeccompEmulator::SerializeFilters(FEXCore::Core::CpuStateFrame* Frame) {
  auto Thread = FEX::HLE::ThreadManager::GetStateObjectFromCPUState(Frame);
  if (Thread->SeccompMode == SECCOMP_MODE_DISABLED) {
    // Didn't have seccomp enabled.
    return std::nullopt;
  }

  int FD = memfd_create("seccomp_filters", MFD_ALLOW_SEALING);
  if (FD == -1) {
    // Couldn't create memfd
    LogMan::Msg::EFmt("Couldn't create seccomp filter FD!");
    return -1;
  }

  SerializationHeader Header {
    .NumberOfFilters = Thread->Filters.size(),
    .SeccompMode = Thread->SeccompMode,
  };

  int Res = write(FD, &Header, sizeof(Header));
  if (Res == -1) {
    LogMan::Msg::EFmt("Couldn't write header!");
    close(FD);
    return -1;
  }

  for (auto& Filter : Thread->Filters) {
    SerializedFilter SFilter {
      .CodeSize = Filter->MappedSize,
      .FilterInstructions = Filter->FilterInstructions,
      .ShouldLog = Filter->ShouldLog,
    };

    Res = write(FD, &SFilter, sizeof(SFilter));
    if (Res == -1) {
      LogMan::Msg::EFmt("Couldn't write filter header!");
      close(FD);
      return -1;
    }

    Res = write(FD, (const void*)Filter->Func, Filter->MappedSize);
    if (Res == -1) {
      LogMan::Msg::EFmt("Couldn't write filter!");
      close(FD);
      return -1;
    }
  }

  // Reset FD to start.
  lseek(FD, 0, SEEK_SET);

  // Seal everything about this FD.
  fcntl(FD, F_ADD_SEALS, F_SEAL_SEAL | F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE | F_SEAL_FUTURE_WRITE);

  return FD;
}

void SeccompEmulator::DeserializeFilters(FEXCore::Core::CpuStateFrame* Frame, int FD) {
  auto Thread = FEX::HLE::ThreadManager::GetStateObjectFromCPUState(Frame);

  SerializationHeader Header;
  int Res = read(FD, &Header, sizeof(Header));
  if (Res == -1 || Res != sizeof(Header)) {
    LogMan::Msg::EFmt("Couldn't read Seccomp header!");
    close(FD);
    return;
  }

  for (size_t i = 0; i < Header.NumberOfFilters; ++i) {
    SerializedFilter SFilter;

    Res = read(FD, &SFilter, sizeof(SFilter));
    if (Res == -1 || Res != sizeof(SFilter)) {
      LogMan::Msg::EFmt("Couldn't read Seccomp Filter header!");
      close(FD);
      return;
    }
    auto Ptr = FEXCore::Allocator::mmap(nullptr, SFilter.CodeSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (Ptr == (void*)~0ULL) {
      LogMan::Msg::EFmt("Couldn't allocate ptr for filter!");
      close(FD);
      return;
    }

    Res = read(FD, Ptr, SFilter.CodeSize);
    if (Res == -1 || Res != SFilter.CodeSize) {
      LogMan::Msg::EFmt("Couldn't read Seccomp Filter code!");
      close(FD);
      return;
    }

    ::mprotect(Ptr, SFilter.CodeSize, PROT_READ | PROT_EXEC);

    auto& it = Filters.emplace_back(FilterInformation {(FilterFunc)Ptr, 1, SFilter.CodeSize, SFilter.FilterInstructions, SFilter.ShouldLog});
    TotalFilterInstructions += SFilter.FilterInstructions;

    // Append the filter to the thread.
    Thread->Filters.emplace_back(&it);
  }

  Thread->SeccompMode = Header.SeccompMode;
  close(FD);
}

SeccompEmulator::ExecuteFilterResult
SeccompEmulator::ExecuteFilter(FEXCore::Core::CpuStateFrame* Frame, uint64_t JITPC, FEXCore::HLE::SyscallArguments* Args) {
  auto Thread = FEX::HLE::ThreadManager::GetStateObjectFromCPUState(Frame);

  if (Thread->Filters.empty()) {
    // Seccomp not installed. Allow it.
    return {false, 0};
  }

  // Reconstruct the RIP from the JITPC.
  const uint64_t RIP = Thread->Thread->CTX->RestoreRIPFromHostPC(Frame->Thread, JITPC);

  const auto Arch = Is64BitMode() ? AUDIT_ARCH_X86_64 : AUDIT_ARCH_I386;
  bool ShouldLog {};
  uint32_t SeccompResult {};

  {
    BPFEmitter::WorkingBuffer Data {
      .Data =
        {
          .nr = static_cast<int32_t>(Args->Argument[0]),
          .arch = Arch,
          .instruction_pointer = RIP,
          .args =
            {
              Args->Argument[1],
              Args->Argument[2],
              Args->Argument[3],
              Args->Argument[4],
              Args->Argument[5],
              Args->Argument[6],
            },
        },
    };

    bool HasResult {};
    // seccomp filters are executed from latest added to oldest.
    for (auto it = Thread->Filters.rbegin(); it != Thread->Filters.rend(); ++it) {
      // Explicitly zero scratch memory.
      memset(&Data.ScratchMemory, 0, sizeof(Data.ScratchMemory));

      uint32_t CurrentResult = (*it)->Func(0, 0, 0, 0, &Data);

      if (!HasResult) {
        SeccompResult = CurrentResult;
        ShouldLog = (*it)->ShouldLog;
        HasResult = true;
        continue;
      }

      const int16_t CurrentAction = (CurrentResult & SECCOMP_RET_ACTION_FULL) >> 16;
      const int16_t Action = (SeccompResult & SECCOMP_RET_ACTION_FULL) >> 16;

      // All actions are executed but the first highest precendent result is returned.
      // Precedent order from highest priority to lowest:
      //   - SECCOMP_RET_KILL_PROCESS (0x8000, -32768)
      //   - SECCOMP_RET_KILL_THREAD  (0x0000,      0)
      //   - SECCOMP_RET_TRAP         (0x0003,      3)
      //   - SECCOMP_RET_ERRNO        (0x0005,      5)
      //   - SECCOMP_RET_USER_NOTIF   (0x7fc0,  32704)
      //   - SECCOMP_RET_TRACE        (0x7ff0,  32752)
      //   - SECCOMP_RET_LOG          (0x7ffc,  32764)
      //   - SECCOMP_RET_ALLOW        (0x7fff,  32767)
      if (CurrentAction < Action) {
        SeccompResult = CurrentResult;
        ShouldLog = (*it)->ShouldLog;
      }
    }
  }

  const auto ActionMasked = SeccompResult & SECCOMP_RET_ACTION_FULL;
  const auto DataMasked = SeccompResult & SECCOMP_RET_DATA;

  // Logging rules
  // - Log if explicitly returning SECCOMP_RET_LOG
  // - Log if the filter enabled the logging flag and the action is something other than SECCOMP_RET_ALLOW.
  if ((ShouldLog && ActionMasked != SECCOMP_RET_ALLOW) || ActionMasked == SECCOMP_RET_LOG) {
    int Signal = 0;
    switch (ActionMasked) {
    case SECCOMP_RET_KILL_PROCESS:
    case SECCOMP_RET_KILL_THREAD: Signal = GetKillSignal(); break;
    case SECCOMP_RET_TRAP: Signal = SIGSYS; break;
    default: break;
    }

    // With real secommp the logs go to dmesg. log through FEX since we can't use dmesg.
    // ex: `[13572.669277] audit: type=1326 audit(1715469332.533:62): auid=1000 uid=1000 gid=1000 ses=2 subj=unconfined pid=52546 comm="seccomp_bpf"
    // exe="/mnt/Work/Projects/work/linux/tools/testing/selftests/seccomp/seccomp_bpf" sig=0 arch=c000003e syscall=39 compat=0 ip=0x7d789352725d code=0x7ffc0000`
    timespec tp {};
    clock_gettime(CLOCK_MONOTONIC, &tp);
    LogMan::Msg::IFmt("audit: type={} audit({}.{:03}:{}): uid={} gid={} pid={} comm={} sig={} arch={:x} syscall={} ip=0x{:x} code=0x{:x}",
                      AUDIT_SECCOMP, tp.tv_sec, tp.tv_nsec / 1'000'000, AuditSerialIncrement(), ::getuid(), ::getgid(), ::getpid(),
                      Filename(), Signal, Arch, Args->Argument[0], RIP, SeccompResult);
  }

  switch (ActionMasked) {
  // Unknown actions behave like RET_KILL_PROCESS.
  default:
  case SECCOMP_RET_KILL_PROCESS: {
    const int KillSignal = GetKillSignal();
    // Ignores signal handler and sigmask
    uint64_t Mask = 1 << (KillSignal - 1);
    SignalDelegation->GuestSigProcMask(Thread, SIG_UNBLOCK, &Mask, nullptr);
    SignalDelegation->UninstallHostHandler(KillSignal);
    kill(0, KillSignal);
    break;
  }
  case SECCOMP_RET_KILL_THREAD: {
    // Ignores signal handler and sigmask
    uint64_t Mask = 1 << (SIGSYS - 1);
    SignalDelegation->GuestSigProcMask(Thread, SIG_UNBLOCK, &Mask, nullptr);
    SignalDelegation->UninstallHostHandler(SIGSYS);
    tgkill(::getpid(), ::gettid(), SIGSYS);
    break;
  }
  case SECCOMP_RET_TRAP: {
    siginfo_t Info {
      .si_signo = SIGSYS,
      .si_errno = static_cast<int32_t>(DataMasked),
      .si_code = 1, // SYS_SECCOMP
    };

    Info.si_call_addr = reinterpret_cast<void*>(RIP);
    Info.si_syscall = Args->Argument[0];
    Info.si_arch = Arch;

    SignalDelegation->QueueSignal(::getpid(), ::gettid(), SIGSYS, &Info, true);
    break;
  }
  case SECCOMP_RET_ERRNO: {
    // errno return is clamped.
    return {true, -(std::min<uint64_t>(DataMasked, 4095))};
  }
  case SECCOMP_RET_TRACE: {
    // When no tracer attached, behave like RET_ERRNO returning ENOSYS.
    // TODO: Implement once FEX supports tracing.
    return {true, static_cast<uint64_t>(-ENOSYS)};
  }
  case SECCOMP_RET_USER_NOTIF:
  case SECCOMP_RET_LOG:
  case SECCOMP_RET_ALLOW: break;
  }

  return {false, 0};
}

// Equivalent to seccomp(SECCOMP_SET_MODE_STRICT, ...);
uint64_t SeccompEmulator::SetModeStrict(FEXCore::Core::CpuStateFrame* Frame, uint32_t flags, const void* arg) {
  const auto Thread = FEX::HLE::ThreadManager::GetStateObjectFromCPUState(Frame);

  if (::prctl(PR_GET_NO_NEW_PRIVS, 0, 0, 0, 0) == 0) {
    // The caller did not have the CAP_SYS_ADMIN capability in its user namespace, or had not set no_new_privs before using SECCOMP_SET_MODE_FILTER.
    return -EACCES;
  }

  if (flags != 0) {
    // The specified flags are invalid for the given operation.
    return -EINVAL;
  }

  if (arg != nullptr) {
    // The specified arg are invalid for the given operation.
    return -EINVAL;
  }

  if (Thread->SeccompMode == SECCOMP_MODE_FILTER) {
    // Filter mode cannot move to strict
    return -EINVAL;
  }

#define syscall_nr (offsetof(struct seccomp_data, nr))
#define ALLOW_SYSCALL(name) \
  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, FEX::HLE::x64::SYSCALL_x64_##name, 0, 1), BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW)
#define ALLOW_SYSCALL_x32(name) \
  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, FEX::HLE::x32::SYSCALL_x86_##name, 0, 1), BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW)

  constexpr static struct sock_filter strict_filter_x64[] = {
    // Load syscall number
    BPF_STMT(BPF_LD + BPF_W + BPF_ABS, syscall_nr),

    // Allow read, write, exit, exit_group, and sigreturn
    ALLOW_SYSCALL(read),
    ALLOW_SYSCALL(write),
    ALLOW_SYSCALL(exit),
    ALLOW_SYSCALL(exit_group),
    ALLOW_SYSCALL(rt_sigreturn),
    BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_KILL_PROCESS),
  };

  constexpr static struct sock_filter strict_filter_x32[] = {
    // Load syscall number
    BPF_STMT(BPF_LD + BPF_W + BPF_ABS, syscall_nr),

    // Allow read, write, exit, exit_group, and sigreturn
    ALLOW_SYSCALL_x32(read),
    ALLOW_SYSCALL_x32(write),
    ALLOW_SYSCALL_x32(exit),
    ALLOW_SYSCALL_x32(exit_group),
    ALLOW_SYSCALL_x32(rt_sigreturn),
    ALLOW_SYSCALL_x32(sigreturn),
    BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_KILL_PROCESS),
  };

  const sock_fprog prog_x64 {
    .len = (unsigned short)(sizeof(strict_filter_x64) / sizeof(strict_filter_x64[0])),
    .filter = const_cast<struct sock_filter*>(strict_filter_x64),
  };

  const sock_fprog prog_x32 {
    .len = (unsigned short)(sizeof(strict_filter_x32) / sizeof(strict_filter_x32[0])),
    .filter = const_cast<struct sock_filter*>(strict_filter_x32),
  };
  CurrentKillSignal = SIGKILL;
  const sock_fprog* prog = Is64BitMode() ? &prog_x64 : &prog_x32;
  SetModeFilter(Frame, 0, prog);
  Thread->SeccompMode = SECCOMP_MODE_STRICT;

  return 0;
}

uint64_t SeccompEmulator::CanDoTSync(FEXCore::Core::CpuStateFrame* Frame) {
  auto ParentThread = FEX::HLE::ThreadManager::GetStateObjectFromCPUState(Frame);
  auto Threads = SyscallHandler->TM.GetThreads();

  for (auto& Thread : *Threads) {
    if (Thread == ParentThread) {
      // Skip same thread.
      continue;
    }

    if (Thread->SeccompMode == SECCOMP_MODE_DISABLED) {
      // Threads which have seccomp disabled are safe to TSync
      continue;
    }

    if (Thread->SeccompMode != ParentThread->SeccompMode) {
      /// If the seccomp mode differs between threads then it can't tsync.
      /// Strict versus filter mode aren't tsync compatible.
      return Thread->ThreadInfo.TID;
    }

    if (Thread->Filters.size() != ParentThread->Filters.size()) {
      // If the filter count doesn't even match then it can't tsync.
      return Thread->ThreadInfo.TID;
    }

    // Walk each filter and ensure the entry points are the same and in the same order.
    for (size_t i = 0; i < ParentThread->Filters.size(); ++i) {
      if (Thread->Filters[i]->Func != ParentThread->Filters[i]->Func) {
        /// Entry point mismatch, not the same filter.
        /// Not tsync compatible.
        return Thread->ThreadInfo.TID;
      }
    }
  }

  // Everything matched. tsync compatible!
  return 0;
}

void SeccompEmulator::TSyncFilters(FEXCore::Core::CpuStateFrame* Frame) {
  auto ParentThread = FEX::HLE::ThreadManager::GetStateObjectFromCPUState(Frame);
  auto Threads = SyscallHandler->TM.GetThreads();

  for (auto& Thread : *Threads) {
    if (Thread == ParentThread) {
      // Skip same thread.
      continue;
    }

    Thread->Filters.clear();
    Thread->Filters = ParentThread->Filters;
    for (auto& Filter : ParentThread->Filters) {
      // Need to increment all the refcounters
      std::atomic_ref<uint64_t>(Filter->RefCount)++;
    }
    Thread->SeccompMode = ParentThread->SeccompMode;
  }
}

// Equivalent to seccomp(SECCOMP_SET_MODE_FILTER, ...);
uint64_t SeccompEmulator::SetModeFilter(FEXCore::Core::CpuStateFrame* Frame, uint32_t flags, const sock_fprog* prog) {
  auto Thread = FEX::HLE::ThreadManager::GetStateObjectFromCPUState(Frame);

  // Order of checks in this function matter
  // 1) Check flags
  // 2) Check if program is invalid
  uint32_t SUPPORTED_FLAGS = SECCOMP_FILTER_FLAG_TSYNC |      // 1U << 0
                             SECCOMP_FILTER_FLAG_LOG |        // 1U << 1
                             SECCOMP_FILTER_FLAG_SPEC_ALLOW | // 1U << 2
                             // SECCOMP_FILTER_FLAG_NEW_LISTENER |      // 1U << 3
                             SECCOMP_FILTER_FLAG_TSYNC_ESRCH | // 1U << 4
                             0;

  const bool DoingTsync = flags & SECCOMP_FILTER_FLAG_TSYNC;

  if (flags & ~SUPPORTED_FLAGS) {
    // Unknown flags passed in.
    return -EINVAL;
  }

  if ((flags & SECCOMP_FILTER_FLAG_TSYNC) && (flags & SECCOMP_FILTER_FLAG_NEW_LISTENER) && !(flags & SECCOMP_FILTER_FLAG_TSYNC_ESRCH)) {
    /// If NEW_LISTENER and TSYNC are both used then TSYNC_ESRCH must also be set.
    /// Otherwise on error there would be no way to tell the difference between success and failure.
    return -EINVAL;
  }

  if (!prog) {
    return -EFAULT;
  }

  if (::prctl(PR_GET_NO_NEW_PRIVS, 0, 0, 0, 0) == 0) {
    // The caller did not have the CAP_SYS_ADMIN capability in its user namespace, or had not set no_new_privs before using SECCOMP_SET_MODE_FILTER.
    return -EACCES;
  }

  if (prog->len > BPF_MAXINSNS || prog->len == 0) {
    // operation specified SECCOMP_SET_MODE_FILTER, but the filter program pointed to by args was not valid or the length of the filter
    // program was zero or exceeded BPF_MAXINSNS (4096) instructions.
    return -EINVAL;
  }

  // Don't interrupt me while I'm jitting.
  auto lk = FEXCore::MaskSignalsAndLockMutex(FilterMutex);

  const size_t TotalFinalInstructions = TotalFilterInstructions + prog->len + Thread->Filters.size() * BPF_MULTIFILTERPENALTY;
  if (TotalFinalInstructions > BPF_MAX_INSNS_PER_PATH) {
    return -ENOMEM;
  }

  if constexpr (false) {
    // Useful for debugging seccomp problems.
    DumpProgram(prog);
  }

  if (DoingTsync) {
    auto TSyncThread = CanDoTSync(Frame);
    if (TSyncThread != 0) {
      if (flags & SECCOMP_FILTER_FLAG_TSYNC_ESRCH) {
        // This flag explicitly ensures that if TSYNC can't sync then it won't return a TID.
        return -ESRCH;
      } else {
        // Return the TID that caused a tsync problem.
        return TSyncThread;
      }
    }
  }

  BPFEmitter emit {};
  const bool LoggingEnabled = flags & SECCOMP_FILTER_FLAG_LOG;
  auto Result = emit.JITFilter(flags, prog);
  if (Result == 0) {

    auto& it = Filters.emplace_back(FilterInformation {(FilterFunc)emit.GetFunc(), 1, emit.AllocationSize(), prog->len, LoggingEnabled});
    TotalFilterInstructions += prog->len;

    // Append the filter to the thread.
    Thread->Filters.emplace_back(&it);
    Thread->SeccompMode = SECCOMP_MODE_FILTER;
    if (flags & SECCOMP_FILTER_FLAG_TSYNC) {
      TSyncFilters(Frame);
    }
  }

  return Result;
}

// Equivalent to seccomp(SECCOMP_GET_ACTION_AVAIL, ...);
uint64_t SeccompEmulator::GetActionAvail(uint32_t flags, const uint32_t* action) {
  if (flags != 0) {
    // Unknown flags passed in
    return -EINVAL;
  }

  if (!action) {
    // Invalid action
    return -EFAULT;
  }
  switch (*action) {
  case SECCOMP_RET_KILL_PROCESS:
  case SECCOMP_RET_KILL_THREAD:
  case SECCOMP_RET_TRAP:
  case SECCOMP_RET_ERRNO:
  case SECCOMP_RET_LOG:
  case SECCOMP_RET_ALLOW: return 0;
  case SECCOMP_RET_USER_NOTIF:
  case SECCOMP_RET_TRACE:
  default: break;
  }

  return -EOPNOTSUPP;
}

// Equivalent to seccomp(SECCOMP_GET_NOTIF_SIZES, ...);
uint64_t SeccompEmulator::GetNotifSizes(uint32_t flags, struct seccomp_notif_sizes* sizes) {
  if (flags != 0) {
    // Unknown flags passed in
    return -EINVAL;
  }
  sizes->seccomp_notif = sizeof(struct seccomp_notif);
  sizes->seccomp_notif_resp = sizeof(struct seccomp_notif_resp);
  sizes->seccomp_data = sizeof(struct seccomp_data);

  return 0;
}

} // namespace FEX::HLE
