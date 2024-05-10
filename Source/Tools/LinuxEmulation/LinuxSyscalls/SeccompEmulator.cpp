// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "LinuxSyscalls/SeccompEmulator.h"
#include "LinuxSyscalls/x32/Syscalls.h"
#include "LinuxSyscalls/x64/Syscalls.h"

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
struct WorkingBuffer {
  struct seccomp_data Data;
  uint32_t ScratchMemory[BPF_MEMWORDS]; ///< Defined as 16 words.
};

class BPFEmitter final : public ARMEmitter::Emitter {
public:
  BPFEmitter() = default;

  uint64_t JITFilter(uint32_t flags, const sock_fprog* prog);
  void* GetFunc() const {
    return Func;
  }

  size_t AllocationSize() const {
    return FuncSize;
  }

private:
  template<bool CalculateSize>
  uint64_t HandleLoad(uint32_t BPFIP, const sock_filter* Inst);
  template<bool CalculateSize>
  uint64_t HandleStore(uint32_t BPFIP, const sock_filter* Inst);
  template<bool CalculateSize>
  uint64_t HandleALU(uint32_t BPFIP, const sock_filter* Inst);
  template<bool CalculateSize>
  uint64_t HandleJmp(uint32_t BPFIP, uint32_t NumInst, const sock_filter* Inst);
  template<bool CalculateSize>
  uint64_t HandleRet(uint32_t BPFIP, const sock_filter* Inst);
  template<bool CalculateSize>
  uint64_t HandleMisc(uint32_t BPFIP, const sock_filter* Inst);

#define EMIT_INST(x)               \
  do {                             \
    if constexpr (CalculateSize) { \
      OpSize += 4;                 \
    } else {                       \
      x                            \
    }                              \
  } while (0)

#define RETURN_ERROR(x)                                                                \
  if constexpr (CalculateSize) {                                                       \
    return ~0ULL;                                                                      \
  } else {                                                                             \
    static_assert(x == -EINVAL, "Early return error evaluation only supports EINVAL"); \
    return x;                                                                          \
  }

#define RETURN_SUCCESS()           \
  do {                             \
    if constexpr (CalculateSize) { \
      return OpSize;               \
    } else {                       \
      return 0;                    \
    }                              \
  } while (0)

  using SizeErrorCheck = decltype([](uint64_t Result) -> bool { return Result == ~0ULL; });
  using EmissionErrorCheck = decltype([](uint64_t Result) { return Result != 0; });

  template<bool CalculateSize, class Pred>
  uint64_t HandleEmission(uint32_t flags, const sock_fprog* prog);

  ///< Register selection comes from function signature.
  constexpr static auto REG_A = ARMEmitter::WReg::w0;
  constexpr static auto REG_X = ARMEmitter::WReg::w1;
  constexpr static auto REG_TMP = ARMEmitter::WReg::w2;
  constexpr static auto REG_TMP2 = ARMEmitter::WReg::w3;
  constexpr static auto REG_SECCOMP_DATA = ARMEmitter::XReg::x4;
  fextl::unordered_map<uint32_t, ARMEmitter::ForwardLabel> JumpLabels;
  fextl::unordered_map<uint32_t, ARMEmitter::ForwardLabel> ConstPool;

  void* Func;
  size_t FuncSize;
};

uint64_t SeccompEmulator::Handle(FEXCore::Core::CpuStateFrame* Frame, uint32_t Op, uint32_t flags, void* arg) {
  switch (Op) {
  case SECCOMP_SET_MODE_STRICT: return SetModeStrict(Frame, flags, arg);
  case SECCOMP_SET_MODE_FILTER: return SetModeFilter(Frame, flags, static_cast<const sock_fprog*>(arg));
  case SECCOMP_GET_ACTION_AVAIL: return GetActionAvail(flags, static_cast<const uint32_t*>(arg));
  case SECCOMP_GET_NOTIF_SIZES: return GetNotifSizes(flags, static_cast<struct seccomp_notif_sizes*>(arg));
  default:
    ///< operation is unknown or is not supported by this kernel version or configuration.
    return -EINVAL;
  }
}

///< Equivalent to prctl(PR_GET_SECCOMP)
uint64_t SeccompEmulator::GetSeccomp(FEXCore::Core::CpuStateFrame* Frame) {
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
    ///< Didn't have seccomp enabled.
    return std::nullopt;
  }

  int FD = memfd_create("seccomp_filters", MFD_ALLOW_SEALING);
  if (FD == -1) {
    ///< Couldn't create memfd
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

  ///< Reset FD to start.
  lseek(FD, 0, SEEK_SET);

  ///< Seal everything about this FD.
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
SeccompEmulator::ExecuteFilter(FEXCore::Core::CpuStateFrame* Frame, uint64_t RIP, FEXCore::HLE::SyscallArguments* Args) {
  auto Thread = FEX::HLE::ThreadManager::GetStateObjectFromCPUState(Frame);

  if (Thread->Filters.empty()) {
    // Seccomp not installed. Allow it.
    return {SECCOMP_RET_ALLOW, false};
  }

  WorkingBuffer Data {
    .Data =
      {
        .nr = (int)Args->Argument[0],
        .arch = Is64BitMode() ? AUDIT_ARCH_X86_64 : AUDIT_ARCH_I386,
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
  ExecuteFilterResult Result {};
  // seccomp filters are executed from latest added to oldest.
  for (auto it = Thread->Filters.rbegin(); it != Thread->Filters.rend(); ++it) {
    ///< Explicitly zero scratch memory.
    memset(&Data.ScratchMemory, 0, sizeof(Data.ScratchMemory));

    uint32_t CurrentResult = (*it)->Func(0, 0, 0, 0, &Data);

    // LogMan::Msg::IFmt("\tSub-Filter result: 0x{:x}", CurrentResult);
    if (!HasResult) {
      Result.SeccompResult = CurrentResult;
      Result.ShouldLog = (*it)->ShouldLog;
      HasResult = true;
      continue;
    }

    const int16_t CurrentAction = (CurrentResult & SECCOMP_RET_ACTION_FULL) >> 16;
    const int16_t Action = (Result.SeccompResult & SECCOMP_RET_ACTION_FULL) >> 16;

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
      Result.SeccompResult = CurrentResult;
      Result.ShouldLog = (*it)->ShouldLog;
    }
  }
  // LogMan::Msg::DFmt("Seccomp[{}]: 0x{:x}", Args->Argument[0], Result.SeccompResult);
  return Result;
}

///< Equivalent to seccomp(SECCOMP_SET_MODE_STRICT, ...);
uint64_t SeccompEmulator::SetModeStrict(FEXCore::Core::CpuStateFrame* Frame, uint32_t flags, const void* arg) {
  const auto Thread = FEX::HLE::ThreadManager::GetStateObjectFromCPUState(Frame);

  if (::prctl(PR_GET_NO_NEW_PRIVS, 0, 0, 0, 0) == 0) {
    ///< The caller did not have the CAP_SYS_ADMIN capability in its user namespace, or had not set no_new_privs before using SECCOMP_SET_MODE_FILTER.
    return -EACCES;
  }

  if (flags != 0) {
    ///< The specified flags are invalid for the given operation.
    return -EINVAL;
  }

  if (arg != nullptr) {
    ///< The specified arg are invalid for the given operation.
    return -EINVAL;
  }

  if (Thread->SeccompMode == SECCOMP_MODE_FILTER) {
    ///< Filter mode cannot move to strict
    return -EINVAL;
  }

#define syscall_nr (offsetof(struct seccomp_data, nr))
#define ALLOW_SYSCALL(name) \
  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, FEX::HLE::x64::SYSCALL_x64_##name, 0, 1), BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW)
#define ALLOW_SYSCALL_x32(name) \
  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, FEX::HLE::x32::SYSCALL_x86_##name, 0, 1), BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW)

  constexpr static struct sock_filter filter_x64[] = {
    ///< Load syscall number
    BPF_STMT(BPF_LD + BPF_W + BPF_ABS, syscall_nr),

    // Allow read, write, exit, exit_group, and sigreturn
    ALLOW_SYSCALL(read),
    ALLOW_SYSCALL(write),
    ALLOW_SYSCALL(exit),
    ALLOW_SYSCALL(exit_group),
    ALLOW_SYSCALL(rt_sigreturn),
    BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_KILL_PROCESS),
  };

  constexpr static struct sock_filter filter_x32[] = {
    ///< Load syscall number
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
    .len = (unsigned short)(sizeof(filter_x64) / sizeof(filter_x64[0])),
    .filter = const_cast<struct sock_filter*>(filter_x64),
  };

  const sock_fprog prog_x32 {
    .len = (unsigned short)(sizeof(filter_x32) / sizeof(filter_x32[0])),
    .filter = const_cast<struct sock_filter*>(filter_x32),
  };
  CurrentKillSignal = SIGKILL;
  const sock_fprog* prog = Is64BitMode() ? &prog_x64 : &prog_x32;
  SetModeFilter(Frame, 0, prog);
  Thread->SeccompMode = SECCOMP_MODE_STRICT;

  LogMan::Msg::DFmt("Strict mode engaged!");
  return 0;
}

uint64_t SeccompEmulator::CanDoTSync(FEXCore::Core::CpuStateFrame* Frame) {
  auto ParentThread = FEX::HLE::ThreadManager::GetStateObjectFromCPUState(Frame);
  auto Threads = SyscallHandler->TM.GetThreads();

  for (auto& Thread : *Threads) {
    if (Thread == ParentThread) {
      ///< Skip same thread.
      continue;
    }

    if (Thread->SeccompMode == SECCOMP_MODE_DISABLED) {
      ///< Threads which have seccomp disabled are safe to TSync
      continue;
    }

    if (Thread->SeccompMode != ParentThread->SeccompMode) {
      /// If the seccomp mode differs between threads then it can't tsync.
      /// Strict versus filter mode aren't tsync compatible.
      return Thread->Thread->ThreadManager.GetTID();
    }

    if (Thread->Filters.size() != ParentThread->Filters.size()) {
      // If the filter count doesn't even match then it can't tsync.
      return Thread->Thread->ThreadManager.GetTID();
    }

    // Walk each filter and ensure the entry points are the same and in the same order.
    for (size_t i = 0; i < ParentThread->Filters.size(); ++i) {
      if (Thread->Filters[i]->Func != ParentThread->Filters[i]->Func) {
        /// Entry point mismatch, not the same filter.
        /// Not tsync compatible.
        return Thread->Thread->ThreadManager.GetTID();
      }
    }
  }

  ///< Everything matched. tsync compatible!
  return 0;
}

void SeccompEmulator::TSyncFilters(FEXCore::Core::CpuStateFrame* Frame) {
  auto ParentThread = FEX::HLE::ThreadManager::GetStateObjectFromCPUState(Frame);
  auto Threads = SyscallHandler->TM.GetThreads();

  size_t ThreadCount {};
  for (auto& Thread : *Threads) {
    if (Thread == ParentThread) {
      ///< Skip same thread.
      continue;
    }

    Thread->Filters.clear();
    Thread->Filters = ParentThread->Filters;
    for (auto& Filter : ParentThread->Filters) {
      ///< Need to increment all the refcounters
      std::atomic_ref<uint64_t>(Filter->RefCount)++;
    }
    Thread->SeccompMode = ParentThread->SeccompMode;
    ++ThreadCount;
  }

  LogMan::Msg::DFmt("Out of {} threads. Synced {}", Threads->size(), ThreadCount);
}

///< Equivalent to seccomp(SECCOMP_SET_MODE_FILTER, ...);
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
    ///< Unknown flags passed in.
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
    ///< The caller did not have the CAP_SYS_ADMIN capability in its user namespace, or had not set no_new_privs before using SECCOMP_SET_MODE_FILTER.
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

  // DumpProgram(prog);

  if (DoingTsync) {
    auto TSyncThread = CanDoTSync(Frame);
    LogMan::Msg::IFmt("Doing tsync. {} due to {}", TSyncThread ? "Failing" : "Passing", TSyncThread);
    if (TSyncThread != 0) {
      if (flags & SECCOMP_FILTER_FLAG_TSYNC_ESRCH) {
        ///< This flag explicitly ensures that if TSYNC can't sync then it won't return a TID.
        return -ESRCH;
      } else {
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

///< Equivalent to seccomp(SECCOMP_GET_ACTION_AVAIL, ...);
uint64_t SeccompEmulator::GetActionAvail(uint32_t flags, const uint32_t* action) {
  if (flags != 0) {
    ///< Unknown flags passed in
    return -EINVAL;
  }

  if (!action) {
    ///< Invalid action
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

///< Equivalent to seccomp(SECCOMP_GET_NOTIF_SIZES, ...);
uint64_t SeccompEmulator::GetNotifSizes(uint32_t flags, struct seccomp_notif_sizes* sizes) {
  if (flags != 0) {
    ///< Unknown flags passed in
    return -EINVAL;
  }
  sizes->seccomp_notif = sizeof(struct seccomp_notif);
  sizes->seccomp_notif_resp = sizeof(struct seccomp_notif_resp);
  sizes->seccomp_data = sizeof(struct seccomp_data);

  return 0;
}

template<bool CalculateSize>
uint64_t BPFEmitter::HandleLoad(uint32_t BPFIP, const sock_filter* Inst) {
  [[maybe_unused]] size_t OpSize {};
  switch (BPF_SIZE(Inst->code)) {
  case BPF_H:
  case BPF_B:
  case 0x18: /* BPF_DW */ RETURN_ERROR(-EINVAL);
  case BPF_W: break;
  }

  const auto DestReg = BPF_CLASS(Inst->code) == BPF_LD ? REG_A : REG_X;

  switch (BPF_MODE(Inst->code)) {
  case BPF_IMM: {
    auto Const = ConstPool.try_emplace(Inst->k, ARMEmitter::ForwardLabel {});
    EMIT_INST(ldr(DestReg, &Const.first->second););
    break;
  }
  case BPF_ABS: {
    // ABS has some restrictions
    // - Must be 4-byte aligned
    // - Must be less than the size of seccomp_data
    const auto Offset = Inst->k;
    if (Offset & 0b11) {
      ///< Wasn't 4-byte aligned.
      RETURN_ERROR(-EINVAL);
    }

    if (Offset >= sizeof(seccomp_data)) {
      ///< Tried accessing outside of seccomp_data.
      RETURN_ERROR(-EINVAL);
    }

    EMIT_INST(ldr(DestReg, REG_SECCOMP_DATA, Offset););
    break;
  }
  case BPF_MEM:
    if (Inst->k >= 16) {
      ///< Larger than scratch space size.
      RETURN_ERROR(-EINVAL);
    }

    EMIT_INST(ldr(DestReg, REG_SECCOMP_DATA, offsetof(WorkingBuffer, ScratchMemory[Inst->k])););
    break;
  case BPF_IND:
  case BPF_MSH: RETURN_ERROR(-EINVAL); ///< Unsupported
  case BPF_LEN:
    ///< Just returns the length of seccomp_data.
    EMIT_INST(movz(DestReg, sizeof(seccomp_data)););
    break;
  }

  RETURN_SUCCESS();
}

template<bool CalculateSize>
uint64_t BPFEmitter::HandleStore(uint32_t BPFIP, const sock_filter* Inst) {
  [[maybe_unused]] size_t OpSize {};
  switch (BPF_SIZE(Inst->code)) {
  case BPF_H:
  case BPF_B:
  case 0x18: /* BPF_DW */ RETURN_ERROR(-EINVAL);
  case BPF_W: break;
  }

  const auto SrcReg = BPF_CLASS(Inst->code) == BPF_LD ? REG_A : REG_X;
  if (Inst->k >= 16) {
    ///< Larger than scratch space size.
    RETURN_ERROR(-EINVAL);
  }

  EMIT_INST(str(SrcReg, REG_SECCOMP_DATA, offsetof(WorkingBuffer, ScratchMemory[Inst->k])););

  RETURN_SUCCESS();
}

template<bool CalculateSize>
uint64_t BPFEmitter::HandleALU(uint32_t BPFIP, const sock_filter* Inst) {
  [[maybe_unused]] size_t OpSize {};
  const auto SrcType = BPF_SRC(Inst->code);
  const auto Op = BPF_OP(Inst->code);

  switch (Op) {
  case BPF_ADD:
  case BPF_SUB:
  case BPF_MUL:
  case BPF_DIV:
  case BPF_OR:
  case BPF_AND:
  case BPF_LSH:
  case BPF_RSH:
  case BPF_MOD:
  case BPF_XOR: {
    auto SrcReg = REG_X;
    if (SrcType == BPF_K) {
      SrcReg = REG_TMP;
      auto Const = ConstPool.try_emplace(Inst->k, ARMEmitter::ForwardLabel {});
      EMIT_INST(ldr(SrcReg, &Const.first->second););
    }

    switch (Op) {
    case BPF_ADD: EMIT_INST(add(ARMEmitter::Size::i32Bit, REG_A, REG_A, SrcReg);); break;
    case BPF_SUB: EMIT_INST(sub(ARMEmitter::Size::i32Bit, REG_A, REG_A, SrcReg);); break;
    case BPF_MUL: EMIT_INST(mul(ARMEmitter::Size::i32Bit, REG_A, REG_A, SrcReg);); break;
    case BPF_DIV:
      ///< Specifically unsigned.
      EMIT_INST(udiv(ARMEmitter::Size::i32Bit, REG_A, REG_A, SrcReg););
      break;
    case BPF_OR: EMIT_INST(orr(ARMEmitter::Size::i32Bit, REG_A, REG_A, SrcReg);); break;
    case BPF_AND: EMIT_INST(and_(ARMEmitter::Size::i32Bit, REG_A, REG_A, SrcReg);); break;
    case BPF_LSH: EMIT_INST(lslv(ARMEmitter::Size::i32Bit, REG_A, REG_A, SrcReg);); break;
    case BPF_RSH: EMIT_INST(lsrv(ARMEmitter::Size::i32Bit, REG_A, REG_A, SrcReg);); break;
    case BPF_MOD:
      ///< Specifically unsigned.
      EMIT_INST(udiv(ARMEmitter::Size::i32Bit, REG_TMP2, REG_A, SrcReg););
      EMIT_INST(msub(ARMEmitter::Size::i32Bit, REG_A, REG_TMP2, SrcReg, REG_A););
      break;
    case BPF_XOR: EMIT_INST(eor(ARMEmitter::Size::i32Bit, REG_A, REG_A, SrcReg);); break;
    default: RETURN_ERROR(-EINVAL);
    }

    break;
  }
  case BPF_NEG:
    if (SrcType == BPF_X) {
      ///< Only BPF_K supported on NEG.
      RETURN_ERROR(-EINVAL);
    }

    EMIT_INST(neg(ARMEmitter::Size::i32Bit, REG_A, REG_A););
    break;

  default: RETURN_ERROR(-EINVAL);
  }

  RETURN_SUCCESS();
}

template<bool CalculateSize>
uint64_t BPFEmitter::HandleJmp(uint32_t BPFIP, uint32_t NumInst, const sock_filter* Inst) {
  [[maybe_unused]] size_t OpSize {};
  const auto SrcType = BPF_SRC(Inst->code);
  const auto Op = BPF_OP(Inst->code);

  switch (Op) {
  case BPF_JA: {
    if (SrcType == BPF_X) {
      ///< Only BPF_K supported on JA.
      RETURN_ERROR(-EINVAL);
    }

    // BPF IP register is effectively only 32-bit. Treat k constant like a signed integer.
    // This allows it to jump anywhere in the program.
    // But! Loops are EXPLICITLY disallowed inside of BPF programs.
    // This is to prevent DOS style attacks through BPF programs.
    uint64_t Target = BPFIP + Inst->k + 1;
    if (Target >= NumInst) {
      ///< Jumped past the end.
      RETURN_ERROR(-EINVAL);
    }

    fextl::unordered_map<uint32_t, ARMEmitter::ForwardLabel>::iterator TargetLabel {};

    if constexpr (!CalculateSize) {
      TargetLabel = JumpLabels.try_emplace(Target, ARMEmitter::ForwardLabel {}).first;
    }

    EMIT_INST(b(&TargetLabel->second););
    break;
  }
  case BPF_JEQ:
  case BPF_JGT:
  case BPF_JGE:
  case BPF_JSET: {
    auto CompareSrcReg = REG_X;
    if (SrcType == BPF_K) {
      CompareSrcReg = REG_TMP;
      auto Const = ConstPool.try_emplace(Inst->k, ARMEmitter::ForwardLabel {});
      EMIT_INST(ldr(CompareSrcReg, &Const.first->second););
    }
    uint32_t TargetTrue = BPFIP + Inst->jt + 1;
    uint32_t TargetFalse = BPFIP + Inst->jf + 1;

    if (TargetTrue >= NumInst || TargetFalse >= NumInst) {
      ///< Trying to jump past the end.
      RETURN_ERROR(-EINVAL);
    }

    auto CompareResultOp = ARMEmitter::Condition::CC_EQ;
    if (Op == BPF_JEQ) {
      CompareResultOp = ARMEmitter::Condition::CC_EQ;
      EMIT_INST(cmp(ARMEmitter::Size::i32Bit, REG_A, CompareSrcReg););
    } else if (Op == BPF_JGT) {
      CompareResultOp = ARMEmitter::Condition::CC_HI;
      EMIT_INST(cmp(ARMEmitter::Size::i32Bit, REG_A, CompareSrcReg););
    } else if (Op == BPF_JGE) {
      CompareResultOp = ARMEmitter::Condition::CC_HS;
      EMIT_INST(cmp(ARMEmitter::Size::i32Bit, REG_A, CompareSrcReg););
    } else if (Op == BPF_JSET) {
      CompareResultOp = ARMEmitter::Condition::CC_NE;
      EMIT_INST(tst(ARMEmitter::Size::i32Bit, REG_A, CompareSrcReg););
    }

    fextl::unordered_map<uint32_t, ARMEmitter::ForwardLabel>::iterator TargetTrueLabel {};
    fextl::unordered_map<uint32_t, ARMEmitter::ForwardLabel>::iterator TargetFalseLabel {};

    if constexpr (!CalculateSize) {
      TargetTrueLabel = JumpLabels.try_emplace(TargetTrue, ARMEmitter::ForwardLabel {}).first;
      TargetFalseLabel = JumpLabels.try_emplace(TargetFalse, ARMEmitter::ForwardLabel {}).first;
    }

    EMIT_INST(b(CompareResultOp, &TargetTrueLabel->second););
    EMIT_INST(b(&TargetFalseLabel->second););
    break;
  }
  default: RETURN_ERROR(-EINVAL); ///< Unknown jump type
  }

  RETURN_SUCCESS();
}

template<bool CalculateSize>
uint64_t BPFEmitter::HandleRet(uint32_t BPFIP, const sock_filter* Inst) {
  [[maybe_unused]] size_t OpSize {};
  const auto RValSrc = BPF_RVAL(Inst->code);
  switch (RValSrc) {
  case BPF_K: {
    auto Const = ConstPool.try_emplace(Inst->k, ARMEmitter::ForwardLabel {});
    EMIT_INST(ldr(ARMEmitter::WReg::w0, &Const.first->second););
    break;
  }
  case BPF_X: EMIT_INST(mov(ARMEmitter::WReg::w0, REG_X);); break;
  case BPF_A:
    // w0 is already REG_A
    static_assert(REG_A == ARMEmitter::WReg::w0, "This is expected to be the same");
    break;
  }

  EMIT_INST(ret(););

  RETURN_SUCCESS();
}

template<bool CalculateSize>
uint64_t BPFEmitter::HandleMisc(uint32_t BPFIP, const sock_filter* Inst) {
  [[maybe_unused]] size_t OpSize {};
  const auto MiscOp = BPF_MISCOP(Inst->code);
  switch (MiscOp) {
  case BPF_TAX: EMIT_INST(mov(REG_X, REG_A);); break;
  case BPF_TXA: EMIT_INST(mov(REG_A, REG_X);); break;
  default: RETURN_ERROR(-EINVAL) ///< Unsupported misc operation.
  }

  RETURN_SUCCESS();
}

template<bool CalculateSize, class Pred>
uint64_t BPFEmitter::HandleEmission(uint32_t flags, const sock_fprog* prog) {
  constexpr Pred PredFunc;
  uint64_t CalculatedSize {};

  for (uint32_t i = 0; i < prog->len; ++i) {
    if constexpr (!CalculateSize) {
      auto jump_label = JumpLabels.find(i);
      if (jump_label != JumpLabels.end()) {
        Bind(&jump_label->second);
      }
    }

    bool HadError {};
    uint64_t Result {};

    const sock_filter* Inst = &prog->filter[i];
    const uint16_t Code = Inst->code;
    const uint16_t Class = BPF_CLASS(Code);
    switch (Class) {
    case BPF_LD:
    case BPF_LDX: {
      Result = HandleLoad<CalculateSize>(i, Inst);
      HadError = PredFunc(Result);
      break;
    }
    case BPF_ST:
    case BPF_STX: {
      Result = HandleStore<CalculateSize>(i, Inst);
      HadError = PredFunc(Result);
      break;
    }
    case BPF_ALU: {
      Result = HandleALU<CalculateSize>(i, Inst);
      HadError = PredFunc(Result);
      break;
    }
    case BPF_JMP: {
      Result = HandleJmp<CalculateSize>(i, prog->len, Inst);
      HadError = PredFunc(Result);
      break;
    }
    case BPF_RET: {
      Result = HandleRet<CalculateSize>(i, Inst);
      HadError = PredFunc(Result);
      break;
    }
    case BPF_MISC: {
      Result = HandleMisc<CalculateSize>(i, Inst);
      HadError = PredFunc(Result);
      break;
    }
    }

    if constexpr (CalculateSize) {
      CalculatedSize += Result;
    }

    if (HadError) {
      if constexpr (!CalculateSize) {
        // Had error, early return and free the memory.
        FEXCore::Allocator::munmap(GetBufferBase(), FuncSize);
      }
      return Result;
    }
  }

  if constexpr (CalculateSize) {
    // Add the constant pool size.
    CalculatedSize += ConstPool.size() * 4;

    // Size calculation could have added constants and jump labels. Erase them now.
    ConstPool.clear();
    JumpLabels.clear();

    return CalculatedSize;
  }

  return 0;
}

uint64_t BPFEmitter::JITFilter(uint32_t flags, const sock_fprog* prog) {
  FuncSize = HandleEmission<true, SizeErrorCheck>(flags, prog);

  if (FuncSize == ~0ULL) {
    // Buffer size calculation found invalid code.
    return -EINVAL;
  }

  LogMan::Msg::IFmt("Seccomp filter calculated size: {}", FuncSize);
  SetBuffer((uint8_t*)FEXCore::Allocator::mmap(nullptr, FuncSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0), FuncSize);

  const auto CodeBegin = GetCursorAddress<uint8_t*>();

  uint64_t Result = HandleEmission<false, EmissionErrorCheck>(flags, prog);

  if (Result != 0) {
    // Had error, early return and free the memory.
    FEXCore::Allocator::munmap(GetBufferBase(), FuncSize);
    return Result;
  }

  const uint64_t CodeOnlySize = GetCursorAddress<uint8_t*>() - CodeBegin;

  // Emit the constant pool.
  Align();
  for (auto& Const : ConstPool) {
    Bind(&Const.second);
    dc32(Const.first);
  }

  ClearICache(CodeBegin, CodeOnlySize);
  ::mprotect(CodeBegin, AllocationSize(), PROT_READ | PROT_EXEC);
  Func = CodeBegin;

  LogMan::Msg::DFmt("JITFilter: disas 0x{:x},+{}", (uint64_t)CodeBegin, CodeOnlySize);

  ConstPool.clear();
  JumpLabels.clear();
  return 0;
}

void SeccompEmulator::DumpProgram(const sock_fprog* prog) {
  auto Parse_Class_LD = [](uint32_t BPFIP, const sock_filter* Inst) {
    auto DestName = [](sock_filter const* Inst) {
      if (BPF_CLASS(Inst->code) == BPF_LD) {
        return "A";
      } else {
        return "X";
      }
    };

    auto AccessSize = [](sock_filter const* Inst) {
      switch (BPF_SIZE(Inst->code)) {
      case BPF_W: return 32;
      case BPF_H: return 16;
      case BPF_B: return 8;
      case 0x18: /* BPF_DW */ return 64;
      }
      return 0;
    };

    auto ModeType = [](sock_filter const* Inst) {
      switch (BPF_MODE(Inst->code)) {
      case BPF_IMM: return "IMM";
      case BPF_ABS: return "ABS";
      case BPF_IND: return "IND";
      case BPF_MEM: return "MEM";
      case BPF_LEN: return "LEN";
      case BPF_MSH: return "MSH";
      }
      return "Unknown";
    };

    auto LoadName = [](sock_filter const* Inst) {
      using namespace std::string_view_literals;
      switch (BPF_MODE(Inst->code)) {
      case BPF_IMM: return fextl::fmt::format("#{}", Inst->k);
      case BPF_ABS: return fextl::fmt::format("seccomp_data + #{}", Inst->k);
      case BPF_IND: return fextl::fmt::format("Ind[X+#{}]", Inst->k);
      case BPF_MEM: return fextl::fmt::format("Mem[#{}]", Inst->k);
      case BPF_LEN: return fextl::fmt::format("len");
      case BPF_MSH: return fextl::fmt::format("msh");
      }
      return fextl::fmt::format("Unknown");
    };

    LogMan::Msg::IFmt("0x{:04x}: {} <- LD.{} {} {}", BPFIP, DestName(Inst), AccessSize(Inst), ModeType(Inst), LoadName(Inst));
  };

  auto Parse_Class_ST = [](uint32_t BPFIP, const sock_filter* Inst) {
    auto DestName = [](sock_filter const* Inst) {
      if (BPF_CLASS(Inst->code) == BPF_ST) {
        return "A";
      } else {
        return "X";
      }
    };


    LogMan::Msg::IFmt("0x{:04x}: Mem[{}] <- ST.{}", BPFIP, Inst->k, DestName(Inst));
  };

  auto Parse_Class_JMP = [](uint32_t BPFIP, const sock_filter* Inst) {
    auto GetOp = [](sock_filter const* Inst) {
      switch (BPF_OP(Inst->code)) {
      case BPF_JA: return "a";
      case BPF_JEQ: return "eq";
      case BPF_JGT: return "gt";
      case BPF_JGE: return "ge";
      case BPF_JSET: return "set";
      }
      return "Unknown";
    };

    auto GetSrc = [](sock_filter const* Inst) {
      switch (BPF_SRC(Inst->code)) {
      case BPF_K: return fextl::fmt::format("0x{:x}", Inst->k);
      case BPF_X: return fextl::fmt::format("<X>");
      }
      return fextl::fmt::format("Unknown");
    };

    LogMan::Msg::IFmt("0x{:04x}: JMP.{} {}, +{} (#0x{:x}), +{} (#0x{:x})", BPFIP, GetOp(Inst), GetSrc(Inst), Inst->jt, BPFIP + Inst->jt + 1,
                      Inst->jf, BPFIP + Inst->jf + 1);
  };


  auto Parse_Class_RET = [](uint32_t BPFIP, const sock_filter* Inst) {
    auto GetRetValue = [](sock_filter const* Inst) {
      switch (BPF_RVAL(Inst->code)) {
      case BPF_K: {
        uint32_t RetData = Inst->k & SECCOMP_RET_DATA;
        switch (Inst->k & SECCOMP_RET_ACTION_FULL) {
        case SECCOMP_RET_KILL_PROCESS: return fextl::fmt::format("KILL_PROCESS.{}", RetData);
        case SECCOMP_RET_KILL_THREAD: return fextl::fmt::format("KILL_THREAD.{}", RetData);
        case SECCOMP_RET_TRAP: return fextl::fmt::format("TRAP.{}", RetData);
        case SECCOMP_RET_ERRNO: return fextl::fmt::format("ERRNO.{}", RetData);
        case SECCOMP_RET_USER_NOTIF: return fextl::fmt::format("USER_NOTIF.{}", RetData);
        case SECCOMP_RET_TRACE: return fextl::fmt::format("TRACE.{}", RetData);
        case SECCOMP_RET_LOG: return fextl::fmt::format("LOG.{}", RetData);
        case SECCOMP_RET_ALLOW: return fextl::fmt::format("ALLOW.{}", RetData);
        default: break;
        }
        return fextl::fmt::format("<Unknown>.{}", RetData);
      }
      case BPF_X: return fextl::fmt::format("<X>");
      case BPF_A: return fextl::fmt::format("<A>");
      }

      return fextl::fmt::format("Unknown");
    };

    LogMan::Msg::IFmt("0x{:04x}: RET {}", BPFIP, GetRetValue(Inst));
  };

  LogMan::Msg::IFmt("BPF program: 0x{:x} instructions", prog->len);

  for (size_t i = 0; i < prog->len; ++i) {
    const sock_filter* Inst = &prog->filter[i];
    const uint16_t Code = Inst->code;
    const uint16_t Class = BPF_CLASS(Code);
    switch (Class) {
    case BPF_LD:
    case BPF_LDX: Parse_Class_LD(i, Inst); break;
    case BPF_ST:
    case BPF_STX: Parse_Class_ST(i, Inst); break;
    case BPF_ALU: break;
    case BPF_JMP: Parse_Class_JMP(i, Inst); break;
    case BPF_RET: Parse_Class_RET(i, Inst); break;
    case BPF_MISC: break;
    }
  }
}

} // namespace FEX::HLE
