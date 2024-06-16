// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/
#pragma once

#include <CodeEmitter/Emitter.h>

#include <FEXCore/Config/Config.h>
#include <FEXCore/fextl/vector.h>
#include <FEXCore/Utils/SignalScopeGuards.h>

#include <atomic>
#include <signal.h>

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

class SyscallHandler;
struct ThreadStateObject;

class SeccompEmulator final {
public:
  SeccompEmulator(FEX::HLE::SyscallHandler* SyscallHandler)
    : SyscallHandler {SyscallHandler} {}

  uint64_t Handle(FEXCore::Core::CpuStateFrame* Frame, uint32_t Op, uint32_t flags, void* arg);

  ///< Equivalent to prctl(PR_GET_SECCOMP)
  uint64_t GetSeccomp(FEXCore::Core::CpuStateFrame* Frame);

  void InheritSeccompFilters(FEX::HLE::ThreadStateObject* Parent, FEX::HLE::ThreadStateObject* Child);
  void FreeSeccompFilters(FEX::HLE::ThreadStateObject* Thread);

  struct ExecuteFilterResult {
    uint32_t SeccompResult;
    bool ShouldLog {};
  };
  ExecuteFilterResult ExecuteFilter(FEXCore::Core::CpuStateFrame* Frame, uint64_t RIP, FEXCore::HLE::SyscallArguments* Args);
  int GetKillSignal() const {
    return CurrentKillSignal;
  }

  std::optional<int> SerializeFilters(FEXCore::Core::CpuStateFrame* Frame);
  void DeserializeFilters(FEXCore::Core::CpuStateFrame* Frame, int FD);

  using FilterFunc = uint64_t (*)(uint32_t Acc, uint32_t Index, uint32_t Tmp, uint32_t Tmp2, void* Data);
  struct FilterInformation final {
    FilterFunc Func;
    uint64_t RefCount;
    size_t MappedSize;
    uint32_t FilterInstructions;
    bool ShouldLog;
  };

private:
  FEX_CONFIG_OPT(Is64BitMode, IS64BIT_MODE);
  FEX::HLE::SyscallHandler* SyscallHandler;

  int CurrentKillSignal {SIGSYS};

  ///< Equivalent to seccomp(SECCOMP_SET_MODE_STRICT, ...);
  uint64_t SetModeStrict(FEXCore::Core::CpuStateFrame* Frame, uint32_t flags, const void* arg);
  ///< Equivalent to seccomp(SECCOMP_SET_MODE_FILTER, ...);
  uint64_t SetModeFilter(FEXCore::Core::CpuStateFrame* Frame, uint32_t flags, const sock_fprog* prog);
  ///< Equivalent to seccomp(SECCOMP_GET_ACTION_AVAIL, ...);
  uint64_t GetActionAvail(uint32_t flags, const uint32_t* action);
  ///< Equivalent to seccomp(SECCOMP_GET_NOTIF_SIZES, ...);
  uint64_t GetNotifSizes(uint32_t flags, struct seccomp_notif_sizes* sizes);

  ///< TODO: These two TSYNC functions have race conditions to be fixed.
  ///< 0 on TSync possible
  /// TID for the first thread that breaks tsync.
  uint64_t CanDoTSync(FEXCore::Core::CpuStateFrame* Frame);
  void TSyncFilters(FEXCore::Core::CpuStateFrame* Frame);

  void DumpProgram(const sock_fprog* prog);

  // Multiple filter instruction count penalty.
  // When multiple filters are installed there is a penalty per filter counted towards the maximum number of instructions.
  constexpr static size_t BPF_MULTIFILTERPENALTY = 4;
  // Maximum number of BPF instructions.
  constexpr static size_t BPF_MAX_INSNS_PER_PATH = 32768;
  uint64_t TotalFilterInstructions {};

  FEXCore::ForkableUniqueMutex FilterMutex;
  fextl::list<FilterInformation> Filters {};
};
} // namespace FEX::HLE
