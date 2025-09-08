// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/
#pragma once

#include <FEXCore/Config/Config.h>
#include <FEXCore/fextl/list.h>
#include <FEXCore/Utils/SignalScopeGuards.h>

#include <atomic>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <optional>

struct sock_fprog;
struct seccomp_data;
struct seccomp_notif_sizes;

namespace FEXCore {

namespace Core {
  struct CpuStateFrame;
}

namespace HLE {
  struct SyscallArguments;
}

} // namespace FEXCore

namespace FEX::HLE {

class SignalDelegator;
class SyscallHandler;
struct ThreadStateObject;

using SeccompFilterFunc = uint64_t (*)(uint32_t Acc, uint32_t Index, uint32_t Tmp, uint32_t Tmp2, void* Data);
struct SeccompFilterInfo final {
  SeccompFilterFunc Func;
  uint64_t RefCount;
  size_t MappedSize;
  uint32_t FilterInstructions;
  bool ShouldLog;
};

class SeccompEmulator final {
public:
  SeccompEmulator(FEX::HLE::SyscallHandler* SyscallHandler, FEX::HLE::SignalDelegator* SignalDelegation)
    : SyscallHandler {SyscallHandler}
    , SignalDelegation {SignalDelegation} {}

  uint64_t Handle(FEXCore::Core::CpuStateFrame* Frame, uint32_t Op, uint32_t flags, void* arg);

  // Equivalent to prctl(PR_GET_SECCOMP)
  uint64_t GetSeccomp(FEXCore::Core::CpuStateFrame* Frame);

  void InheritSeccompFilters(FEX::HLE::ThreadStateObject* Parent, FEX::HLE::ThreadStateObject* Child);
  void FreeSeccompFilters(FEX::HLE::ThreadStateObject* Thread);

  struct ExecuteFilterResult {
    bool EarlyReturn {};
    uint64_t Result;
  };
  ExecuteFilterResult ExecuteFilter(FEXCore::Core::CpuStateFrame* Frame, uint64_t JITPC, FEXCore::HLE::SyscallArguments* Args);
  int GetKillSignal() const {
    return CurrentKillSignal;
  }

  std::optional<int> SerializeFilters(FEXCore::Core::CpuStateFrame* Frame);
  void DeserializeFilters(FEXCore::Core::CpuStateFrame* Frame, int FD);

private:
  FEX_CONFIG_OPT(Is64BitMode, IS64BIT_MODE);
  FEX_CONFIG_OPT(NeedsSeccomp, NEEDSSECCOMP);
  FEX_CONFIG_OPT(Filename, APP_FILENAME);
  FEX::HLE::SyscallHandler* SyscallHandler;
  FEX::HLE::SignalDelegator* SignalDelegation;

  int CurrentKillSignal {SIGSYS};

  // Equivalent to seccomp(SECCOMP_SET_MODE_STRICT, ...);
  uint64_t SetModeStrict(FEXCore::Core::CpuStateFrame* Frame, uint32_t flags, const void* arg);
  // Equivalent to seccomp(SECCOMP_SET_MODE_FILTER, ...);
  uint64_t SetModeFilter(FEXCore::Core::CpuStateFrame* Frame, uint32_t flags, const sock_fprog* prog);
  // Equivalent to seccomp(SECCOMP_GET_ACTION_AVAIL, ...);
  uint64_t GetActionAvail(uint32_t flags, const uint32_t* action);
  // Equivalent to seccomp(SECCOMP_GET_NOTIF_SIZES, ...);
  uint64_t GetNotifSizes(uint32_t flags, struct seccomp_notif_sizes* sizes);

  // 0 on TSync possible
  /// TID for the first thread that breaks tsync.
  uint64_t CanDoTSync(FEXCore::Core::CpuStateFrame* Frame);
  void TSyncFilters(FEXCore::Core::CpuStateFrame* Frame);

  static void DumpProgram(const sock_fprog* prog);

  // Multiple filter instruction count penalty.
  // When multiple filters are installed there is a penalty per filter counted towards the maximum number of instructions.
  constexpr static size_t BPF_MULTIFILTERPENALTY = 4;
  // Maximum number of BPF instructions.
  constexpr static size_t BPF_MAX_INSNS_PER_PATH = 32768;
  uint64_t TotalFilterInstructions {};

  FEXCore::ForkableUniqueMutex FilterMutex;
  fextl::list<SeccompFilterInfo> Filters {};

  uint64_t AuditSerialIncrement() {
    return AuditSerial.fetch_add(1);
  }
  std::atomic<uint64_t> AuditSerial {};
};
} // namespace FEX::HLE
