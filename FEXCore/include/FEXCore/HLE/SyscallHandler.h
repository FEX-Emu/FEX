// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <optional>

#include <FEXCore/Core/CodeCache.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/fextl/string.h>

namespace FEXCore::Context {
class Context;
}

namespace FEXCore::Core {
struct InternalThreadState;
struct CpuStateFrame;
} // namespace FEXCore::Core

namespace FEXCore::HLE {
struct ExecutableRangeInfo {
  uint64_t Base;
  uint64_t Size;
  bool Writable;
};

class SyscallHandler;
class SourcecodeResolver;

class SyscallHandler {
public:
  virtual ~SyscallHandler() = default;

  virtual void HandleSyscall(FEXCore::Core::CpuStateFrame* Frame) = 0;

  virtual void MarkGuestExecutableRange(FEXCore::Core::InternalThreadState* Thread, uint64_t Start, uint64_t Length) {}
  virtual void InvalidateGuestCodeRange(FEXCore::Core::InternalThreadState* Thread, uint64_t Start, uint64_t Length) {}
  virtual void MarkOvercommitRange(uint64_t Start, uint64_t Length) {}
  virtual void UnmarkOvercommitRange(uint64_t Start, uint64_t Length) {}
  virtual ExecutableRangeInfo QueryGuestExecutableRange(FEXCore::Core::InternalThreadState* Thread, uint64_t Address) = 0;
  virtual std::optional<ExecutableFileSectionInfo> LookupExecutableFileSection(Core::InternalThreadState* Thread, uint64_t GuestAddr) = 0;

  virtual void PreCompile() {}

  virtual SourcecodeResolver* GetSourcecodeResolver() {
    return nullptr;
  }

  virtual void SleepThread(FEXCore::Context::Context* CTX, FEXCore::Core::CpuStateFrame* Frame) {}
};
} // namespace FEXCore::HLE
