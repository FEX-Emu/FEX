// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <shared_mutex>

#include <FEXCore/IR/IR.h>

namespace FEXCore::IR {
struct AOTIRCacheEntry;
}

namespace FEXCore::Context {
class Context;
}

namespace FEXCore::Core {
struct InternalThreadState;
struct CpuStateFrame;
} // namespace FEXCore::Core

namespace FEXCore::HLE {
struct SyscallArguments {
  static constexpr std::size_t MAX_ARGS = 7;
  uint64_t Argument[MAX_ARGS];
};

struct SyscallABI {
  // Expectation is that the backend will be aware of how to modify the arguments based on numbering
  // Only GPRs expected
  uint8_t NumArgs;
  // If the syscall has a return then it should be stored in the ABI specific syscall register
  // Linux = RAX
  bool HasReturn;

  int32_t HostSyscallNumber;
};

enum class SyscallOSABI {
  OS_UNKNOWN,
  OS_LINUX64,
  OS_LINUX32,
  OS_GENERIC, // No JIT-side argument handling, spill/fill all regs.
};

class SyscallHandler;
class SourcecodeResolver;

struct AOTIRCacheEntryLookupResult {
  AOTIRCacheEntryLookupResult(FEXCore::IR::AOTIRCacheEntry* Entry, uintptr_t VAFileStart)
    : Entry(Entry)
    , VAFileStart(VAFileStart) {}

  AOTIRCacheEntryLookupResult(AOTIRCacheEntryLookupResult&&) = default;

  FEXCore::IR::AOTIRCacheEntry* Entry;
  uintptr_t VAFileStart;

  friend class SyscallHandler;
};

class SyscallHandler {
public:
  virtual ~SyscallHandler() = default;

  virtual uint64_t HandleSyscall(FEXCore::Core::CpuStateFrame* Frame, FEXCore::HLE::SyscallArguments* Args) = 0;
  virtual SyscallABI GetSyscallABI(uint64_t Syscall) = 0;
  virtual FEXCore::IR::SyscallFlags GetSyscallFlags(uint64_t Syscall) const {
    return FEXCore::IR::SyscallFlags::DEFAULT;
  }

  SyscallOSABI GetOSABI() const {
    return OSABI;
  }
  virtual void MarkGuestExecutableRange(FEXCore::Core::InternalThreadState* Thread, uint64_t Start, uint64_t Length) {}
  virtual void MarkOvercommitRange(uint64_t Start, uint64_t Length) {}
  virtual void UnmarkOvercommitRange(uint64_t Start, uint64_t Length) {}
  virtual AOTIRCacheEntryLookupResult LookupAOTIRCacheEntry(FEXCore::Core::InternalThreadState* Thread, uint64_t GuestAddr) = 0;

  virtual SourcecodeResolver* GetSourcecodeResolver() {
    return nullptr;
  }

  virtual void SleepThread(FEXCore::Context::Context* CTX, FEXCore::Core::CpuStateFrame* Frame) {}

protected:
  SyscallOSABI OSABI;
};
} // namespace FEXCore::HLE
