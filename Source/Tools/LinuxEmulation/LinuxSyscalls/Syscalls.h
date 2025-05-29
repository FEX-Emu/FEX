// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|common
desc: Glue logic, STRACE magic
$end_info$
*/

#pragma once

#include "LinuxSyscalls/FileManagement.h"
#include "LinuxSyscalls/LinuxAllocator.h"
#include "LinuxSyscalls/ThreadManager.h"
#include "LinuxSyscalls/Seccomp/SeccompEmulator.h"
#include "LinuxSyscalls/SyscallsVMATracking.h"
#include "ArchHelpers/MContext.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/Thunks.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/HLE/SourcecodeResolver.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/SignalScopeGuards.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/functional.h>
#include <FEXCore/fextl/map.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>

#include <mutex>
#include <shared_mutex>

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <type_traits>
#include <list>
#ifdef _M_X86_64
#define SYSCALL_ARCH_NAME x64
#elif _M_ARM_64
#include "LinuxSyscalls/Arm64/SyscallsEnum.h"
#define SYSCALL_ARCH_NAME Arm64
#endif

#include "LinuxSyscalls/x64/SyscallsEnum.h"
#include "LinuxSyscalls/x32/SyscallsEnum.h"

#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)
#define SYSCALL_DEF(name) (HLE::SYSCALL_ARCH_NAME::CONCAT(CONCAT(SYSCALL_, SYSCALL_ARCH_NAME), _##name))

// #define DEBUG_STRACE

namespace FEX {
class CodeLoader;
}

namespace FEXCore {
namespace Context {
  class Context;
}
namespace Core {
  struct CpuStateFrame;
}
} // namespace FEXCore

namespace FEX::HLE {

class SyscallHandler;
class SignalDelegator;
class ThunkHandler;

void RegisterEpoll(FEX::HLE::SyscallHandler* Handler);
void RegisterFD(FEX::HLE::SyscallHandler* Handler);
void RegisterFS(FEX::HLE::SyscallHandler* Handler);
void RegisterInfo(FEX::HLE::SyscallHandler* Handler);
void RegisterIO(FEX::HLE::SyscallHandler* Handler);
void RegisterMemory(FEX::HLE::SyscallHandler* Handler);
void RegisterNuma(FEX::HLE::SyscallHandler* Handler);
void RegisterSignals(FEX::HLE::SyscallHandler* Handler);
void RegisterThread(FEX::HLE::SyscallHandler* Handler);
void RegisterTimer(FEX::HLE::SyscallHandler* Handler);
void RegisterNotImplemented(FEX::HLE::SyscallHandler* Handler);
void RegisterStubs(FEX::HLE::SyscallHandler* Handler);

uint64_t UnimplementedSyscall(FEXCore::Core::CpuStateFrame* Frame, uint64_t SyscallNumber);
uint64_t UnimplementedSyscallSafe(FEXCore::Core::CpuStateFrame* Frame, uint64_t SyscallNumber);

struct ExecveAtArgs {
  int dirfd;
  int flags;
  static ExecveAtArgs Empty() {
    return ExecveAtArgs {
      .dirfd = AT_FDCWD,
      .flags = 0,
    };
  }
};

uint64_t ExecveHandler(FEXCore::Core::CpuStateFrame* Frame, const char* pathname, char* const* argv, char* const* envp, ExecveAtArgs Args);

class SyscallHandler : public FEXCore::HLE::SyscallHandler, FEXCore::HLE::SourcecodeResolver, public FEXCore::Allocator::FEXAllocOperators {
public:
  ThreadManager TM;
  FEX::HLE::SeccompEmulator SeccompEmulator;

  virtual ~SyscallHandler();

  // In the case that the syscall doesn't hit the optimized path then we still need to go here
  uint64_t HandleSyscall(FEXCore::Core::CpuStateFrame* Frame, FEXCore::HLE::SyscallArguments* Args) final override;

  void DefaultProgramBreak(uint64_t Base, uint64_t Size);
  void DeserializeSeccompFD(FEX::HLE::ThreadStateObject* Thread, int FD) {
    if (FD == -1) {
      return;
    }
    SeccompEmulator.DeserializeFilters(Thread->Thread->CurrentFrame, FD);
  }

  using SyscallPtrArg0 = uint64_t (*)(FEXCore::Core::CpuStateFrame* Frame);
  using SyscallPtrArg1 = uint64_t (*)(FEXCore::Core::CpuStateFrame* Frame, uint64_t);
  using SyscallPtrArg2 = uint64_t (*)(FEXCore::Core::CpuStateFrame* Frame, uint64_t, uint64_t);
  using SyscallPtrArg3 = uint64_t (*)(FEXCore::Core::CpuStateFrame* Frame, uint64_t, uint64_t, uint64_t);
  using SyscallPtrArg4 = uint64_t (*)(FEXCore::Core::CpuStateFrame* Frame, uint64_t, uint64_t, uint64_t, uint64_t);
  using SyscallPtrArg5 = uint64_t (*)(FEXCore::Core::CpuStateFrame* Frame, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
  using SyscallPtrArg6 = uint64_t (*)(FEXCore::Core::CpuStateFrame* Frame, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

  struct SyscallFunctionDefinition {
    uint8_t NumArgs;
    FEXCore::IR::SyscallFlags Flags;
    union {
      void* Ptr;
      SyscallPtrArg0 Ptr0;
      SyscallPtrArg1 Ptr1;
      SyscallPtrArg2 Ptr2;
      SyscallPtrArg3 Ptr3;
      SyscallPtrArg4 Ptr4;
      SyscallPtrArg5 Ptr5;
      SyscallPtrArg6 Ptr6;
    };
    int32_t HostSyscallNumber;
#ifdef DEBUG_STRACE
    fextl::string StraceFmt;
#endif
  };

  const SyscallFunctionDefinition* GetDefinition(uint64_t Syscall) {
    return &Definitions.at(Syscall);
  }

  FEXCore::HLE::SyscallABI GetSyscallABI(uint64_t Syscall) override {
    if (NeedsSeccomp) {
      // Override ABI if seccomp is enabled.
      return {FEXCore::HLE::SyscallArguments::MAX_ARGS, true, -1};
    }
    auto& Def = Definitions.at(Syscall);
    return {Def.NumArgs, true, Def.HostSyscallNumber};
  }

  FEXCore::IR::SyscallFlags GetSyscallFlags(uint64_t Syscall) const override {
    if (NeedsSeccomp) {
      // Override flags if seccomp is enabled.
      return FEXCore::IR::SyscallFlags::DEFAULT;
    }
    auto& Def = Definitions.at(Syscall);
    return Def.Flags;
  }

  virtual void RegisterSyscall_32(int SyscallNumber, int32_t HostSyscallNumber, FEXCore::IR::SyscallFlags Flags,
#ifdef DEBUG_STRACE
                                  const fextl::string& TraceFormatString,
#endif
                                  void* SyscallHandler, int ArgumentCount) {
  }

  virtual void RegisterSyscall_64(int SyscallNumber, int32_t HostSyscallNumber, FEXCore::IR::SyscallFlags Flags,
#ifdef DEBUG_STRACE
                                  const fextl::string& TraceFormatString,
#endif
                                  void* SyscallHandler, int ArgumentCount) {
  }

  uint64_t HandleBRK(FEXCore::Core::CpuStateFrame* Frame, void* Addr);

  FEX::HLE::FileManager FM;
  FEX::CodeLoader* GetCodeLoader() const {
    return LocalLoader;
  }
  void SetCodeLoader(FEX::CodeLoader* Loader) {
    LocalLoader = Loader;
  }
  FEX::HLE::SignalDelegator* GetSignalDelegator() {
    return SignalDelegation;
  }

  FEX::HLE::ThunkHandler* GetThunkHandler() {
    return ThunkHandler;
  }

  FEX_CONFIG_OPT(IsInterpreter, IS_INTERPRETER);
  FEX_CONFIG_OPT(IsInterpreterInstalled, INTERPRETER_INSTALLED);
  FEX_CONFIG_OPT(Filename, APP_FILENAME);
  FEX_CONFIG_OPT(RootFSPath, ROOTFS);
  FEX_CONFIG_OPT(Is64BitMode, IS64BIT_MODE);
  FEX_CONFIG_OPT(SMCChecks, SMCCHECKS);
  FEX_CONFIG_OPT(NeedsSeccomp, NEEDSSECCOMP);

  uint32_t GetHostKernelVersion() const {
    return HostKernelVersion;
  }
  uint32_t GetGuestKernelVersion() const {
    return GuestKernelVersion;
  }

  bool IsHostKernelVersionAtLeast(uint32_t Major, uint32_t Minor = 0, uint32_t Patch = 0) const {
    return GetHostKernelVersion() >= KernelVersion(Major, Minor, Patch);
  }

  static uint32_t CalculateHostKernelVersion();
  uint32_t CalculateGuestKernelVersion();

  static uint32_t KernelVersion(uint32_t Major, uint32_t Minor = 0, uint32_t Patch = 0) {
    return (Major << 24) | (Minor << 16) | Patch;
  }

  static uint32_t KernelMajor(uint32_t Version) {
    return Version >> 24;
  }
  static uint32_t KernelMinor(uint32_t Version) {
    return (Version >> 16) & 0xFF;
  }
  static uint32_t KernelPatch(uint32_t Version) {
    return Version & 0xFFFF;
  }

  virtual FEX::HLE::MemAllocator* Get32BitAllocator() {
    return Alloc32Handler.get();
  }

  // does a mmap as if done via a guest syscall
  virtual void* GuestMmap(FEXCore::Core::InternalThreadState* Thread, void* addr, size_t length, int prot, int flags, int fd, off_t offset) = 0;
  void* GuestMmap(bool Is64Bit, FEXCore::Core::InternalThreadState* Thread, void* addr, size_t length, int prot, int flags, int fd, off_t offset);

  // does a guest munmap as if done via a guest syscall
  virtual uint64_t GuestMunmap(FEXCore::Core::InternalThreadState* Thread, void* addr, uint64_t length) = 0;
  uint64_t GuestMunmap(bool Is64Bit, FEXCore::Core::InternalThreadState* Thread, void* addr, uint64_t length);

  uint64_t GuestMremap(bool Is64Bit, FEXCore::Core::InternalThreadState*, void* old_address, size_t old_size, size_t new_size, int flags,
                       void* new_address);
  uint64_t GuestMprotect(FEXCore::Core::InternalThreadState*, void* addr, size_t len, int prot);
  uint64_t GuestShmat(bool Is64Bit, FEXCore::Core::InternalThreadState*, int shmid, const void* shmaddr, int shmflg);
  uint64_t GuestShmdt(bool Is64Bit, FEXCore::Core::InternalThreadState*, const void* shmaddr);

  ///// Memory Manager tracking /////
  void TrackMmap(FEXCore::Core::InternalThreadState* Thread, uint64_t addr, size_t length, int prot, int flags, int fd, off_t offset);
  void TrackMunmap(FEXCore::Core::InternalThreadState* Thread, void* addr, size_t length);
  void TrackMremap(FEXCore::Core::InternalThreadState* Thread, uint64_t OldAddress, size_t OldSize, size_t NewSize, int flags, uint64_t NewAddress);
  void TrackShmat(FEXCore::Core::InternalThreadState* Thread, int shmid, uint64_t shmaddr, int shmflg, uint64_t Length);
  uint64_t TrackShmdt(FEXCore::Core::InternalThreadState* Thread, uint64_t shmaddr);
  void TrackMprotect(FEXCore::Core::InternalThreadState* Thread, void* addr, size_t len, int prot);
  void TrackMadvise(FEXCore::Core::InternalThreadState* Thread, uintptr_t Base, uintptr_t Size, int advice);

  void InvalidateCodeRangeIfNecessary(FEXCore::Core::InternalThreadState* Thread, uint64_t Base, uint64_t Length) {
    if (SMCChecks != FEXCore::Config::CONFIG_SMC_NONE) {
      TM.InvalidateGuestCodeRange(Thread, Base, Length);
    }
  }

  void InvalidateCodeRangeIfNecessaryOnRemap(FEXCore::Core::InternalThreadState* Thread, uint64_t OldAddress, uint64_t NewAddress,
                                             size_t OldSize, size_t NewSize) {
    if (SMCChecks != FEXCore::Config::CONFIG_SMC_NONE) {
      if (OldAddress != NewAddress) {
        if (OldSize != 0) {
          // This also handles the MREMAP_DONTUNMAP case
          TM.InvalidateGuestCodeRange(Thread, OldAddress, OldSize);
        }
      } else {
        // If mapping shrunk, flush the unmapped region
        if (OldSize > NewSize) {
          TM.InvalidateGuestCodeRange(Thread, OldAddress + NewSize, OldSize - NewSize);
        }
      }
    }
  }


  ///// VMA (Virtual Memory Area) tracking /////
  static bool HandleSegfault(FEXCore::Core::InternalThreadState* Thread, int Signal, void* info, void* ucontext);
  void MarkGuestExecutableRange(FEXCore::Core::InternalThreadState* Thread, uint64_t Start, uint64_t Length) override;
  // AOTIRCacheEntryLookupResult also includes a shared lock guard, so the pointed AOTIRCacheEntry return can be safely used
  FEXCore::HLE::AOTIRCacheEntryLookupResult LookupAOTIRCacheEntry(FEXCore::Core::InternalThreadState* Thread, uint64_t GuestAddr) final override;

  ///// FORK tracking /////
  void LockBeforeFork(FEXCore::Core::InternalThreadState* Thread);
  void UnlockAfterFork(FEXCore::Core::InternalThreadState* LiveThread, bool Child);

  void RegisterTLSState(FEX::HLE::ThreadStateObject* Thread);
  void UninstallTLSState(FEX::HLE::ThreadStateObject* Thread);

  SourcecodeResolver* GetSourcecodeResolver() override {
    return this;
  }

  void SleepThread(FEXCore::Context::Context* CTX, FEXCore::Core::CpuStateFrame* Frame) override {
    TM.SleepThread(CTX, Frame);
  }

  bool NeedXIDCheck() const {
    return NeedToCheckXID;
  }
  void DisableXIDCheck() {
    NeedToCheckXID = false;
  }

  constexpr static uint64_t TASK_MAX_64BIT = (1ULL << 48);

  VMATracking::VMATracking VMATracking;

protected:
  SyscallHandler(FEXCore::Context::Context* _CTX, FEX::HLE::SignalDelegator* _SignalDelegation, FEX::HLE::ThunkHandler* ThunkHandler);

  fextl::vector<SyscallFunctionDefinition> Definitions {std::max<std::size_t>(FEX::HLE::x64::SYSCALL_x64_MAX, FEX::HLE::x32::SYSCALL_x86_MAX),
                                                        {
                                                          .NumArgs = 255,
                                                          .Ptr = reinterpret_cast<void*>(&UnimplementedSyscall),
                                                        }};
  std::mutex MMapMutex;

  // BRK management
  uint64_t DataSpace {};
  uint64_t DataSpaceSize {};
  uint64_t DataSpaceMaxSize {};
  uint64_t DataSpaceStartingSize {};

  // (Major << 24) | (Minor << 16) | Patch
  uint32_t HostKernelVersion {};
  uint32_t GuestKernelVersion {};

  FEXCore::Context::Context* CTX;
private:

  FEX::HLE::SignalDelegator* SignalDelegation;
  FEX::HLE::ThunkHandler* ThunkHandler;

  std::mutex FutexMutex;
  std::mutex SyscallMutex;
  FEX::CodeLoader* LocalLoader {};
  bool NeedToCheckXID {true};

#ifdef DEBUG_STRACE
  void Strace(FEXCore::HLE::SyscallArguments* Args, uint64_t Ret);
#endif

  fextl::unique_ptr<FEX::HLE::MemAllocator> Alloc32Handler {};

  fextl::unique_ptr<FEXCore::HLE::SourcecodeMap>
  GenerateMap(const std::string_view& GuestBinaryFile, const std::string_view& GuestBinaryFileId) override;

  std::atomic<uint64_t> AnonSharedId {1};
};

uint64_t HandleSyscall(SyscallHandler* Handler, FEXCore::Core::CpuStateFrame* Frame, FEXCore::HLE::SyscallArguments* Args);

#define SYSCALL_ERRNO()              \
  do {                               \
    if (Result == -1) return -errno; \
    return Result;                   \
  } while (0)
#define SYSCALL_ERRNO_NULL()        \
  do {                              \
    if (Result == 0) return -errno; \
    return Result;                  \
  } while (0)

extern FEX::HLE::SyscallHandler* _SyscallHandler;

#ifdef DEBUG_STRACE
//////
/// Templates to map parameters to format string for syscalls
//////

template<typename T>
struct ArgToFmtString;

#define ARG_TO_STR(tpy, str)                      \
  template<>                                      \
  struct FEX::HLE::ArgToFmtString<tpy> {          \
    inline static const char* const Format = str; \
  };

// Base types
ARG_TO_STR(int, "%d")
ARG_TO_STR(unsigned int, "%u")
ARG_TO_STR(long, "%ld")
ARG_TO_STR(unsigned long, "%lu")

// string types
ARG_TO_STR(char*, "%s")
ARG_TO_STR(const char*, "%s")

// Pointers
template<typename T>
struct ArgToFmtString<T*> {
  inline static const char* const Format = "%p";
};

// Use ArgToFmtString and variadic template to create a format string from an args list
template<typename... Args>
fextl::string CollectArgsFmtString() {
  std::array<const char*, sizeof...(Args)> array = {ArgToFmtString<Args>::Format...};
  return fextl::fmt::format("{}", fmt::join(array, ", "));
}
#else
#define ARG_TO_STR(tpy, str)
#endif

struct open_how {
  uint64_t flags;
  uint64_t mode;
  uint64_t resolve;
};

struct kernel_clone3_args {
  uint64_t flags;
  uint64_t pidfd;
  uint64_t child_tid;
  uint64_t parent_tid;
  uint64_t exit_signal;
  uint64_t stack;
  uint64_t stack_size;
  uint64_t tls;
  uint64_t set_tid;
  uint64_t set_tid_size;
  uint64_t cgroup;
};

enum TypeOfClone {
  TYPE_CLONE2,
  TYPE_CLONE3,
};

struct clone3_args {
  TypeOfClone Type;
  uint64_t SignalMask;

  uint64_t StackSize;
  void* NewStack;

  kernel_clone3_args args;
};

uint64_t CloneHandler(FEXCore::Core::CpuStateFrame* Frame, FEX::HLE::clone3_args* args);

inline static int RemapFromX86Flags(int flags) {
#ifdef _M_X86_64
  // Nothing to change here
#elif _M_ARM_64
  constexpr int X86_64_FLAG_O_DIRECT = 040000;
  constexpr int X86_64_FLAG_O_LARGEFILE = 0100000;
  constexpr int X86_64_FLAG_O_DIRECTORY = 0200000;
  constexpr int X86_64_FLAG_O_NOFOLLOW = 0400000;

  constexpr int AARCH64_FLAG_O_DIRECTORY = 040000;
  constexpr int AARCH64_FLAG_O_NOFOLLOW = 0100000;
  constexpr int AARCH64_FLAG_O_DIRECT = 0200000;
  constexpr int AARCH64_FLAG_O_LARGEFILE = 0400000;

  int new_flags {};
  if (flags & X86_64_FLAG_O_DIRECT) {
    flags = (flags & ~X86_64_FLAG_O_DIRECT);
    new_flags |= AARCH64_FLAG_O_DIRECT;
  }
  if (flags & X86_64_FLAG_O_LARGEFILE) {
    flags = (flags & ~X86_64_FLAG_O_LARGEFILE);
    new_flags |= AARCH64_FLAG_O_LARGEFILE;
  }
  if (flags & X86_64_FLAG_O_DIRECTORY) {
    flags = (flags & ~X86_64_FLAG_O_DIRECTORY);
    new_flags |= AARCH64_FLAG_O_DIRECTORY;
  }
  if (flags & X86_64_FLAG_O_NOFOLLOW) {
    flags = (flags & ~X86_64_FLAG_O_NOFOLLOW);
    new_flags |= AARCH64_FLAG_O_NOFOLLOW;
  }
  flags |= new_flags;
#else
#error Unknown flag remappings for this host platform
#endif
  return flags;
}

inline static int RemapToX86Flags(int flags) {
#ifdef _M_X86_64
  // Nothing to change here
#elif _M_ARM_64
  constexpr int X86_64_FLAG_O_DIRECT = 040000;
  constexpr int X86_64_FLAG_O_LARGEFILE = 0100000;
  constexpr int X86_64_FLAG_O_DIRECTORY = 0200000;
  constexpr int X86_64_FLAG_O_NOFOLLOW = 0400000;

  constexpr int AARCH64_FLAG_O_DIRECTORY = 040000;
  constexpr int AARCH64_FLAG_O_NOFOLLOW = 0100000;
  constexpr int AARCH64_FLAG_O_DIRECT = 0200000;
  constexpr int AARCH64_FLAG_O_LARGEFILE = 0400000;

  int new_flags {};
  if (flags & AARCH64_FLAG_O_DIRECT) {
    flags = (flags & ~AARCH64_FLAG_O_DIRECT);
    new_flags |= X86_64_FLAG_O_DIRECT;
  }
  if (flags & AARCH64_FLAG_O_LARGEFILE) {
    flags = (flags & ~AARCH64_FLAG_O_LARGEFILE);
    new_flags |= X86_64_FLAG_O_LARGEFILE;
  }
  if (flags & AARCH64_FLAG_O_DIRECTORY) {
    flags = (flags & ~AARCH64_FLAG_O_DIRECTORY);
    new_flags |= X86_64_FLAG_O_DIRECTORY;
  }
  if (flags & AARCH64_FLAG_O_NOFOLLOW) {
    flags = (flags & ~AARCH64_FLAG_O_NOFOLLOW);
    new_flags |= X86_64_FLAG_O_NOFOLLOW;
  }
  flags |= new_flags;
#else
#error Unknown flag remappings for this host platform
#endif
  return flags;
}

/**
 * @brief Checks raw syscall return for error
 *
 * This should only be used with raw syscall usage
 *
 * This should not be used with glibc wrapped syscall functions
 *   - This includes the glibc ::syscall(...) function
 *   - This is due to glibc already wrapping the return and setting errno
 *
 * This function should not be used with UAPI breaking syscall results
 * ioctl specifically will break this convention.
 *
 * @param Result The raw syscall return
 *
 * @return If the result was an error result
 */

[[maybe_unused]]
static bool HasSyscallError(uint64_t Result) {
  // MAX_ERRNO is part of the Linux Syscall ABI
  // Redefined here since it doesn't exist as a visible define in the UAPI headers
  constexpr uint64_t MAX_ERRNO = 0xFFFF'FFFF'FFFF'0001ULL;
  // Raw syscalls are guaranteed to not return a valid result in the range of [-4095, -1]
  // In cases where FEX needs to use raw syscalls, this helper checks for this idiom
  return reinterpret_cast<uint64_t>(Result) >= MAX_ERRNO;
}

[[maybe_unused]]
static bool HasSyscallError(const void* Result) {
  return HasSyscallError(reinterpret_cast<uintptr_t>(Result));
}

template<bool IncrementOffset, typename T>
uint64_t GetDentsEmulation(int fd, T* dirp, uint32_t count);

namespace FaultSafeUserMemAccess {
  // These are little helper functions for cases when FEX needs to copy data to or from the application in a robust fashion.
  // CopyFromUser and CopyToUser are memcpy routines that expect to safely SIGSEGV when reading or writing application memory respectively.
  // Returns zero if the memcpy completed, or crashes with SIGABRT and a log message if it faults.
  [[nodiscard]]
  size_t CopyFromUser(void* Dest, const void* Src, size_t Size);
  [[nodiscard]]
  size_t CopyToUser(void* Dest, const void* Src, size_t Size);
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED && defined(_M_ARM_64)
  // These helpers just check if the user pointer is readable and writable.
  // This is useful in an assert build that can be safely sprinkled through the syscall handler without overhead in release builds.
  void VerifyIsReadable(const void* Src, size_t Size);
  void VerifyIsReadableOrNull(const void* Src, size_t Size);
  void VerifyIsWritable(void* Src, size_t Size);
  void VerifyIsWritableOrNull(void* Src, size_t Size);

  // Iterates a null-terminated string and checks if all bytes are readable
  void VerifyIsStringReadable(const char* Src);

  // Iterates a null-terminated string and checks if all bytes are readable. Up to MaxSize bytes are checked.
  void VerifyIsStringReadableMaxSize(const char* Src, size_t MaxSize);
#else
  inline void VerifyIsReadable(const void* Src, size_t Size) {
    if (Src == nullptr) {
      ERROR_AND_DIE_FMT("Unexpected nullptr syscall argument");
    }
  }
  inline void VerifyIsReadableOrNull(const void* Src, size_t Size) {}
  inline void VerifyIsWritable(void* Src, size_t Size) {
    if (Src == nullptr) {
      ERROR_AND_DIE_FMT("Unexpected nullptr syscall argument");
    }
  }
  inline void VerifyIsWritableOrNull(void* Src, size_t Size) {}
  inline void VerifyIsStringReadable(const char* Src) {
    if (Src == nullptr) {
      ERROR_AND_DIE_FMT("Unexpected nullptr syscall argument");
    }
  }
  inline void VerifyIsStringReadableMaxSize(const char* Src, size_t MaxSize) {
    if (Src == nullptr) {
      ERROR_AND_DIE_FMT("Unexpected nullptr syscall argument");
    }
  }
#endif
  bool IsFaultLocation(uint64_t PC);

  static inline bool TryHandleSafeFault(int Signal, const siginfo_t& SigInfo, void* UContext) {
    if (Signal == SIGSEGV && (SigInfo.si_code == SEGV_MAPERR || SigInfo.si_code == SEGV_ACCERR) &&
        FaultSafeUserMemAccess::IsFaultLocation(ArchHelpers::Context::GetPc(UContext))) {
      // Return from the subroutine, returning EFAULT.
      ArchHelpers::Context::SetArmReg(UContext, 0, EFAULT);
      ArchHelpers::Context::SetPc(UContext, ArchHelpers::Context::GetArmReg(UContext, 30));
      return true;
    }

    return false;
  }
} // namespace FaultSafeUserMemAccess


template<typename T>
inline static uint64_t futimesat_compat(int dirfd, const char* pathname, const T times[2]) {
  FaultSafeUserMemAccess::VerifyIsReadableOrNull(times, sizeof(*times) * 2);

  timespec tvs[2] {};
  timespec* tv_ptr {};
  if (times) {
    constexpr int64_t ONE_SECOND_AS_USEC = 1'000'000LL;

    // Incoming microsecond time must not be negative or be larger than one second.
    if (times[0].tv_usec < 0 || times[1].tv_usec < 0 || times[0].tv_usec >= ONE_SECOND_AS_USEC || times[1].tv_usec >= ONE_SECOND_AS_USEC) {
      return -EINVAL;
    }

    tvs[0].tv_sec = times[0].tv_sec;
    tvs[0].tv_nsec = 1000LL * times[0].tv_usec;
    tvs[1].tv_sec = times[1].tv_sec;
    tvs[1].tv_nsec = 1000LL * times[1].tv_usec;
    tv_ptr = tvs;
  }

  uint64_t Result = ::syscall(SYSCALL_DEF(utimensat), dirfd, pathname, tv_ptr, 0);
  SYSCALL_ERRNO();
}

} // namespace FEX::HLE

// Registers syscall for both 32bit and 64bit
#define REGISTER_SYSCALL_IMPL(name, lambda) REGISTER_SYSCALL_IMPL_INTERNAL(name, ~0, FEXCore::IR::SyscallFlags::DEFAULT, lambda)

#define REGISTER_SYSCALL_IMPL_FLAGS(name, flags, lambda) REGISTER_SYSCALL_IMPL_INTERNAL(name, ~0, flags, lambda)

#define REGISTER_SYSCALL_IMPL_PASS_FLAGS(name, flags, lambda) REGISTER_SYSCALL_IMPL_INTERNAL(name, SYSCALL_DEF(name), flags, lambda)

#define REGISTER_SYSCALL_IMPL_INTERNAL(name, number, flags, lambda)                                                 \
  do {                                                                                                              \
    FEX::HLE::x64::RegisterSyscall(Handler, FEX::HLE::x64::SYSCALL_x64_##name, (number), (flags), #name, (lambda)); \
    FEX::HLE::x32::RegisterSyscall(Handler, FEX::HLE::x32::SYSCALL_x86_##name, (number), (flags), #name, (lambda)); \
  } while (false)
