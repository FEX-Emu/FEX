#pragma once
#include <cstdint>
#include <string>
#include <shared_mutex>

#include <FEXCore/IR/IR.h>
#include <FEXHeaderUtils/ScopedSignalMask.h>

namespace FEXCore {
  class CodeLoader;
}

namespace FEXCore::IR {
  struct AOTIRCacheEntry;
}

namespace FEXCore::Context {
  struct Context;
}

namespace FEXCore::Core {
  struct InternalThreadState;
  struct CpuStateFrame;
}

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
    OS_WIN64,
    OS_WIN32,
    OS_HANGOVER,
  };

  class SyscallHandler;
  struct AOTIRCacheEntryLookupResult {
    AOTIRCacheEntryLookupResult(FEXCore::IR::AOTIRCacheEntry *Entry, uintptr_t Offset, FHU::ScopedSignalMaskWithSharedLock &&lk)
      : Entry(Entry), Offset(Offset), lk(std::move(lk))
    {

    }

    AOTIRCacheEntryLookupResult(AOTIRCacheEntryLookupResult&&) = default;

    FEXCore::IR::AOTIRCacheEntry *Entry;
    uintptr_t Offset;

    friend class SyscallHandler;
    protected:
    FHU::ScopedSignalMaskWithSharedLock lk;
  };

  class SyscallHandler {
  public:
    virtual ~SyscallHandler() = default;

    virtual uint64_t HandleSyscall(FEXCore::Core::CpuStateFrame *Frame, FEXCore::HLE::SyscallArguments *Args) = 0;
    virtual SyscallABI GetSyscallABI(uint64_t Syscall) = 0;
    virtual FEXCore::IR::SyscallFlags GetSyscallFlags(uint64_t Syscall) const { return FEXCore::IR::SyscallFlags::DEFAULT; }

    SyscallOSABI GetOSABI() const { return OSABI; }
    virtual FEXCore::CodeLoader *GetCodeLoader() const { return nullptr; }
    virtual void MarkGuestExecutableRange(uint64_t Start, uint64_t Length) { }
    virtual AOTIRCacheEntryLookupResult LookupAOTIRCacheEntry(uint64_t GuestAddr) = 0;

  protected:
    SyscallOSABI OSABI;
  };
}
