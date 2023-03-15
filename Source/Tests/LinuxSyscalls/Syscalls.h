/*
$info$
tags: LinuxSyscalls|common
desc: Glue logic, STRACE magic
$end_info$
*/

#pragma once

#include "Tests/LinuxSyscalls/FileManagement.h"
#include "Tests/LinuxSyscalls/LinuxAllocator.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/HLE/SourcecodeResolver.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/fextl/vector.h>

#include <mutex>
#include <shared_mutex>

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <type_traits>
#include <list>
#include <map>
#ifdef _M_X86_64
#define SYSCALL_ARCH_NAME x64
#elif _M_ARM_64
#include "Tests/LinuxSyscalls/Arm64/SyscallsEnum.h"
#define SYSCALL_ARCH_NAME Arm64
#endif

#define CONCAT_(a, b) a ## b
#define CONCAT(a, b) CONCAT_(a, b)
#define SYSCALL_DEF(name) ( SYSCALL_ARCH_NAME::CONCAT(CONCAT(SYSCALL_, SYSCALL_ARCH_NAME), _##name))

// #define DEBUG_STRACE

namespace FEXCore {
  class CodeLoader;
  namespace Context {
    class Context;
  }
  namespace Core {
    struct CpuStateFrame;
  }
}

namespace FEX::HLE {

class SyscallHandler;
class SignalDelegator;
  void RegisterEpoll(FEX::HLE::SyscallHandler *Handler);
  void RegisterFD(FEX::HLE::SyscallHandler *Handler);
  void RegisterFS(FEX::HLE::SyscallHandler *Handler);
  void RegisterInfo(FEX::HLE::SyscallHandler *Handler);
  void RegisterIO(FEX::HLE::SyscallHandler *Handler);
  void RegisterIOUring(FEX::HLE::SyscallHandler *Handler);
  void RegisterKey(FEX::HLE::SyscallHandler *Handler);
  void RegisterMemory(FEX::HLE::SyscallHandler *Handler);
  void RegisterMsg(FEX::HLE::SyscallHandler *Handler);
  void RegisterNamespace(FEX::HLE::SyscallHandler *Handler);
  void RegisterNuma(FEX::HLE::SyscallHandler *Handler);
  void RegisterSched(FEX::HLE::SyscallHandler *Handler);
  void RegisterSemaphore(FEX::HLE::SyscallHandler *Handler);
  void RegisterSHM(FEX::HLE::SyscallHandler *Handler);
  void RegisterSignals(FEX::HLE::SyscallHandler *Handler);
  void RegisterSocket(FEX::HLE::SyscallHandler *Handler);
  void RegisterThread(FEX::HLE::SyscallHandler *Handler);
  void RegisterTime(FEX::HLE::SyscallHandler *Handler);
  void RegisterTimer(FEX::HLE::SyscallHandler *Handler);
  void RegisterNotImplemented(FEX::HLE::SyscallHandler *Handler);
  void RegisterStubs(FEX::HLE::SyscallHandler *Handler);

uint64_t UnimplementedSyscall(FEXCore::Core::CpuStateFrame *Frame, uint64_t SyscallNumber);
uint64_t UnimplementedSyscallSafe(FEXCore::Core::CpuStateFrame *Frame, uint64_t SyscallNumber);

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

uint64_t ExecveHandler(const char *pathname, char* const* argv, char* const* envp, ExecveAtArgs Args);

class SyscallHandler : public FEXCore::HLE::SyscallHandler, FEXCore::HLE::SourcecodeResolver {
public:
  virtual ~SyscallHandler();

  // In the case that the syscall doesn't hit the optimized path then we still need to go here
  uint64_t HandleSyscall(FEXCore::Core::CpuStateFrame *Frame, FEXCore::HLE::SyscallArguments *Args) final override;

  void DefaultProgramBreak(uint64_t Base, uint64_t Size);

  using SyscallPtrArg0 = uint64_t(*)(FEXCore::Core::CpuStateFrame *Frame);
  using SyscallPtrArg1 = uint64_t(*)(FEXCore::Core::CpuStateFrame *Frame, uint64_t);
  using SyscallPtrArg2 = uint64_t(*)(FEXCore::Core::CpuStateFrame *Frame, uint64_t, uint64_t);
  using SyscallPtrArg3 = uint64_t(*)(FEXCore::Core::CpuStateFrame *Frame, uint64_t, uint64_t, uint64_t);
  using SyscallPtrArg4 = uint64_t(*)(FEXCore::Core::CpuStateFrame *Frame, uint64_t, uint64_t, uint64_t, uint64_t);
  using SyscallPtrArg5 = uint64_t(*)(FEXCore::Core::CpuStateFrame *Frame, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
  using SyscallPtrArg6 = uint64_t(*)(FEXCore::Core::CpuStateFrame *Frame, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

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
    std::string StraceFmt;
#endif
  };

  SyscallFunctionDefinition const *GetDefinition(uint64_t Syscall) {
    return &Definitions.at(Syscall);
  }

  FEXCore::HLE::SyscallABI GetSyscallABI(uint64_t Syscall) override {
    auto &Def = Definitions.at(Syscall);
    return {Def.NumArgs, true, Def.HostSyscallNumber};
  }

  FEXCore::IR::SyscallFlags  GetSyscallFlags(uint64_t Syscall) const override {
    auto &Def = Definitions.at(Syscall);
    return Def.Flags;
  }

  virtual void RegisterSyscall_32(int SyscallNumber,
    int32_t HostSyscallNumber,
    FEXCore::IR::SyscallFlags Flags,
#ifdef DEBUG_STRACE
    const std::string& TraceFormatString,
#endif
    void* SyscallHandler, int ArgumentCount) {
  }

  virtual void RegisterSyscall_64(int SyscallNumber,
    int32_t HostSyscallNumber,
    FEXCore::IR::SyscallFlags Flags,
#ifdef DEBUG_STRACE
    const std::string& TraceFormatString,
#endif
    void* SyscallHandler, int ArgumentCount) {
  }

  uint64_t HandleBRK(FEXCore::Core::CpuStateFrame *Frame, void *Addr);

  FEX::HLE::FileManager FM;
  FEXCore::CodeLoader *GetCodeLoader() const override { return LocalLoader; }
  void SetCodeLoader(FEXCore::CodeLoader *Loader) { LocalLoader = Loader; }
  FEX::HLE::SignalDelegator *GetSignalDelegator() { return SignalDelegation; }

  FEX_CONFIG_OPT(IsInterpreter, IS_INTERPRETER);
  FEX_CONFIG_OPT(IsInterpreterInstalled, INTERPRETER_INSTALLED);
  FEX_CONFIG_OPT(Filename, APP_FILENAME);
  FEX_CONFIG_OPT(RootFSPath, ROOTFS);
  FEX_CONFIG_OPT(ThreadsConfig, THREADS);
  FEX_CONFIG_OPT(Is64BitMode, IS64BIT_MODE);
  FEX_CONFIG_OPT(SMCChecks, SMCCHECKS);

  uint32_t GetHostKernelVersion() const { return HostKernelVersion; }
  uint32_t GetGuestKernelVersion() const { return GuestKernelVersion; }

  bool IsHostKernelVersionAtLeast(uint32_t Major, uint32_t Minor = 0, uint32_t Patch = 0) const {
    return GetHostKernelVersion() >= KernelVersion(Major, Minor, Patch);
  }

  static uint32_t CalculateHostKernelVersion();
  uint32_t CalculateGuestKernelVersion();

  static uint32_t KernelVersion(uint32_t Major, uint32_t Minor = 0, uint32_t Patch = 0) {
    return (Major << 24) | (Minor << 16) | Patch;
  }

  static uint32_t KernelMajor(uint32_t Version) { return Version >> 24; }
  static uint32_t KernelMinor(uint32_t Version) { return (Version >> 16) & 0xFF; }
  static uint32_t KernelPatch(uint32_t Version) { return Version & 0xFFFF; }

  FEX::HLE::MemAllocator *Get32BitAllocator() { return Alloc32Handler.get(); }

  // does a mmap as if done via a guest syscall
  virtual void *GuestMmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) = 0;

  // does a guest munmap as if done via a guest syscall
  virtual int GuestMunmap(void *addr, uint64_t length) = 0;

  ///// Memory Manager tracking /////
  void TrackMmap(uintptr_t Base, uintptr_t Size, int Prot, int Flags, int fd, off_t Offset);
  void TrackMunmap(uintptr_t Base, uintptr_t Size);
  void TrackMprotect(uintptr_t Base, uintptr_t Size, int Prot);
  void TrackMremap(uintptr_t OldAddress, size_t OldSize, size_t NewSize, int flags, uintptr_t NewAddress);
  void TrackShmat(int shmid, uintptr_t Base, int shmflg);
  void TrackShmdt(uintptr_t Base);
  void TrackMadvise(uintptr_t Base, uintptr_t Size, int advice);
  
  ///// VMA (Virtual Memory Area) tracking /////
  static bool HandleSegfault(FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext);
  void MarkGuestExecutableRange(uint64_t Start, uint64_t Length) override;
  // AOTIRCacheEntryLookupResult also includes a shared lock guard, so the pointed AOTIRCacheEntry return can be safely used
  FEXCore::HLE::AOTIRCacheEntryLookupResult LookupAOTIRCacheEntry(uint64_t GuestAddr) final override;

  ///// FORK tracking /////
  void LockBeforeFork();
  void UnlockAfterFork();

  SourcecodeResolver *GetSourcecodeResolver() override { return this; }
  
protected:
  SyscallHandler(FEXCore::Context::Context *_CTX, FEX::HLE::SignalDelegator *_SignalDelegation);

  fextl::vector<SyscallFunctionDefinition> Definitions{};
  std::mutex MMapMutex;

  // BRK management
  uint64_t DataSpace {};
  uint64_t DataSpaceSize {};
  uint64_t DataSpaceMaxSize {};
  uint64_t DataSpaceStartingSize{};

  // (Major << 24) | (Minor << 16) | Patch
  uint32_t HostKernelVersion{};
  uint32_t GuestKernelVersion{};

  FEXCore::Context::Context *CTX;

private:

  FEX::HLE::SignalDelegator *SignalDelegation;

  std::mutex FutexMutex;
  std::mutex SyscallMutex;
  FEXCore::CodeLoader *LocalLoader{};

  #ifdef DEBUG_STRACE
    void Strace(FEXCore::HLE::SyscallArguments *Args, uint64_t Ret);
  #endif

  std::unique_ptr<FEX::HLE::MemAllocator> Alloc32Handler{};

  std::unique_ptr<FEXCore::HLE::SourcecodeMap> GenerateMap(const std::string_view& GuestBinaryFile, const std::string_view& GuestBinaryFileId) override;
  
  ///// VMA (Virtual Memory Area) tracking /////


  struct SpecialDev {
    static constexpr uint64_t Anon = 0x1'0000'0000; // Anonymous shared mapping, id is incrementing allocation number
    static constexpr uint64_t SHM = 0x2'0000'0000;  // sys-v shm, id is shmid
  };

  // Memory Resource ID
  // An id that can be used to identify when shared mappings actually have the same backing storage
  // when dev != SpecialDev::Anon, this is unique system wide
  struct MRID {
    uint64_t dev; // kernel dev_t is actually 32-bits, we use the extra bits to track SpecialDevs
    uint64_t id;

    bool operator<(const MRID& other) const {
      return std::tie(dev, id) < std::tie(other.dev, other.id);
    }
  };

  struct VMAEntry;

  // Used to all MAP_SHARED VMAs of a system resource.
  struct MappedResource {
    using ContainerType = std::map<MRID, MappedResource>;

    FEXCore::IR::AOTIRCacheEntry *AOTIRCacheEntry;
    VMAEntry *FirstVMA;
    uint64_t Length; // 0 if not fixed size
    ContainerType::iterator Iterator;
  };

  union VMAProt {
    struct {
      bool Readable: 1;
      bool Writable: 1;
      bool Executable: 1;
    };
    uint8_t All: 3;
    

    static VMAProt fromProt(int Prot);
    static VMAProt fromSHM(int SHMFlg);
  };

  struct VMAFlags {
    bool Shared: 1;

    static VMAFlags fromFlags(int Flags);
  };

  struct VMAEntry {
    MappedResource *Resource;

    // these are for Intrusive linked list tracking, starting from Resource->FirstVMA
    VMAEntry *ResourcePrevVMA;
    VMAEntry *ResourceNextVMA;

    uint64_t Base;
    uint64_t Offset;
    uint64_t Length;

    VMAFlags Flags;
    VMAProt Prot;
  };

  struct VMATracking {
    using VMAEntry = SyscallHandler::VMAEntry;
    // Held while reading/writing this struct
    std::shared_mutex Mutex;
    
    // Memory ranges indexed by page aligned starting address
    std::map<uint64_t, VMAEntry> VMAs;

    using VMACIterator = decltype(VMAs)::const_iterator;

    MappedResource::ContainerType MappedResources;

    // Mutex must be at least shared_locked before calling
    VMACIterator LookupVMAUnsafe(uint64_t GuestAddr) const;

    // Mutex must be unique_locked before calling
    void SetUnsafe(FEXCore::Context::Context *Ctx, MappedResource *MappedResource, uintptr_t Base, uintptr_t Offset, uintptr_t Length, VMAFlags Flags, VMAProt Prot);
    
    // Mutex must be unique_locked before calling
    void ClearUnsafe(FEXCore::Context::Context *Ctx, uintptr_t Base, uintptr_t Length, MappedResource *PreservedMappedResource = nullptr);

    // Mutex must be unique_locked before calling
    void ChangeUnsafe(uintptr_t Base, uintptr_t Length, VMAProt Prot);

    // Mutex must be unique_locked before calling
    // Returns the Size fo the Shm or 0 if not found
    uintptr_t ClearShmUnsafe(FEXCore::Context::Context *Ctx, uintptr_t Base);
  private:
    bool ListRemove(VMAEntry *Mapping);
    void ListReplace(VMAEntry *Mapping, VMAEntry *NewMapping);
    void ListInsertAfter(VMAEntry *Mapping, VMAEntry *NewMapping);
    void ListPrepend(MappedResource *Resource, VMAEntry *NewVMA);
    static void ListCheckVMALinks(VMAEntry *VMA);
  } VMATracking;
};

uint64_t HandleSyscall(SyscallHandler *Handler, FEXCore::Core::CpuStateFrame *Frame, FEXCore::HLE::SyscallArguments *Args);

#define SYSCALL_ERRNO() do { if (Result == -1) return -errno; return Result; } while(0)
#define SYSCALL_ERRNO_NULL() do { if (Result == 0) return -errno; return Result; } while(0)

extern FEX::HLE::SyscallHandler *_SyscallHandler;

#ifdef DEBUG_STRACE
//////
/// Templates to map parameters to format string for syscalls
//////

template<typename T>
struct ArgToFmtString;

#define ARG_TO_STR(tpy, str) template<> struct FEX::HLE::ArgToFmtString<tpy> { inline static const char* const Format = str; };

// Base types
ARG_TO_STR(int, "%d")
ARG_TO_STR(unsigned int, "%u")
ARG_TO_STR(long, "%ld")
ARG_TO_STR(unsigned long, "%lu")

//string types
ARG_TO_STR(char*, "%s")
ARG_TO_STR(const char*, "%s")

// Pointers
template<typename T>
struct ArgToFmtString<T*> {
  inline static const char* const Format = "%p";
};

// Use ArgToFmtString and variadic template to create a format string from an args list
template<typename ...Args>
std::string CollectArgsFmtString() {
  std::array<const char*, sizeof...(Args)> array = { ArgToFmtString<Args>::Format... };
  return fmt::format("{}", fmt::join(array, ", "));
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
  kernel_clone3_args args;
};

uint64_t CloneHandler(FEXCore::Core::CpuStateFrame *Frame, FEX::HLE::clone3_args *args);

  inline static int RemapFromX86Flags(int flags) {
#ifdef _M_X86_64
    // Nothing to change here
#elif _M_ARM_64
    constexpr int X86_64_FLAG_O_DIRECT    =  040000;
    constexpr int X86_64_FLAG_O_LARGEFILE = 0100000;
    constexpr int X86_64_FLAG_O_DIRECTORY = 0200000;
    constexpr int X86_64_FLAG_O_NOFOLLOW  = 0400000;

    constexpr int AARCH64_FLAG_O_DIRECTORY = 040000;
    constexpr int AARCH64_FLAG_O_NOFOLLOW  = 0100000;
    constexpr int AARCH64_FLAG_O_DIRECT    = 0200000;
    constexpr int AARCH64_FLAG_O_LARGEFILE = 0400000;

    int new_flags{};
    if (flags & X86_64_FLAG_O_DIRECT) {    flags = (flags & ~X86_64_FLAG_O_DIRECT);    new_flags |= AARCH64_FLAG_O_DIRECT; }
    if (flags & X86_64_FLAG_O_LARGEFILE) { flags = (flags & ~X86_64_FLAG_O_LARGEFILE); new_flags |= AARCH64_FLAG_O_LARGEFILE; }
    if (flags & X86_64_FLAG_O_DIRECTORY) { flags = (flags & ~X86_64_FLAG_O_DIRECTORY); new_flags |= AARCH64_FLAG_O_DIRECTORY; }
    if (flags & X86_64_FLAG_O_NOFOLLOW) {  flags = (flags & ~X86_64_FLAG_O_NOFOLLOW);  new_flags |= AARCH64_FLAG_O_NOFOLLOW; }
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
    constexpr int X86_64_FLAG_O_DIRECT    =  040000;
    constexpr int X86_64_FLAG_O_LARGEFILE = 0100000;
    constexpr int X86_64_FLAG_O_DIRECTORY = 0200000;
    constexpr int X86_64_FLAG_O_NOFOLLOW  = 0400000;

    constexpr int AARCH64_FLAG_O_DIRECTORY =  040000;
    constexpr int AARCH64_FLAG_O_NOFOLLOW  = 0100000;
    constexpr int AARCH64_FLAG_O_DIRECT    = 0200000;
    constexpr int AARCH64_FLAG_O_LARGEFILE = 0400000;

    int new_flags{};
    if (flags & AARCH64_FLAG_O_DIRECT) {    flags = (flags & ~AARCH64_FLAG_O_DIRECT);    new_flags |= X86_64_FLAG_O_DIRECT; }
    if (flags & AARCH64_FLAG_O_LARGEFILE) { flags = (flags & ~AARCH64_FLAG_O_LARGEFILE); new_flags |= X86_64_FLAG_O_LARGEFILE; }
    if (flags & AARCH64_FLAG_O_DIRECTORY) { flags = (flags & ~AARCH64_FLAG_O_DIRECTORY); new_flags |= X86_64_FLAG_O_DIRECTORY; }
    if (flags & AARCH64_FLAG_O_NOFOLLOW) {  flags = (flags & ~AARCH64_FLAG_O_NOFOLLOW);  new_flags |= X86_64_FLAG_O_NOFOLLOW; }
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
uint64_t GetDentsEmulation(int fd, T *dirp, uint32_t count);
}

// Registers syscall for both 32bit and 64bit
#define REGISTER_SYSCALL_IMPL(name, lambda) \
  REGISTER_SYSCALL_IMPL_INTERNAL(name, ~0, FEXCore::IR::SyscallFlags::DEFAULT, lambda)

#define REGISTER_SYSCALL_IMPL_PASS(name, lambda) \
  REGISTER_SYSCALL_IMPL_INTERNAL(name, SYSCALL_DEF(name), FEXCore::IR::SyscallFlags::DEFAULT, lambda)

#define REGISTER_SYSCALL_IMPL_FLAGS(name, flags, lambda) \
  REGISTER_SYSCALL_IMPL_INTERNAL(name, ~0, flags, lambda)

#define REGISTER_SYSCALL_IMPL_PASS_FLAGS(name, flags, lambda) \
  REGISTER_SYSCALL_IMPL_INTERNAL(name, SYSCALL_DEF(name), flags, lambda)

#define REGISTER_SYSCALL_IMPL_INTERNAL(name, number, flags, lambda) \
  do { \
    FEX::HLE::x64::RegisterSyscall(Handler, FEX::HLE::x64::SYSCALL_x64_##name, (number), (flags), #name, (lambda)); \
    FEX::HLE::x32::RegisterSyscall(Handler, FEX::HLE::x32::SYSCALL_x86_##name, (number), (flags), #name, (lambda)); \
  } while (false)
