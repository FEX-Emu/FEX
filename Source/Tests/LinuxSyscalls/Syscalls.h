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
#include <FEXCore/Utils/CompilerDefs.h>

#include <mutex>

#include <errno.h>
#include <stdint.h>
#include <type_traits>
#include <vector>
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
    struct Context;
  }
  namespace Core {
    struct CpuStateFrame;
  }
}

namespace FEX::HLE {
class SyscallHandler;
class SignalDelegator;
  void RegisterEpoll();
  void RegisterFD(FEX::HLE::SyscallHandler *const Handler);
  void RegisterFS(FEX::HLE::SyscallHandler *const Handler);
  void RegisterInfo();
  void RegisterIO();
  void RegisterIOUring(FEX::HLE::SyscallHandler *const Handler);
  void RegisterKey();
  void RegisterMemory();
  void RegisterMsg();
  void RegisterNamespace(FEX::HLE::SyscallHandler *const Handler);
  void RegisterNuma();
  void RegisterSched();
  void RegisterSemaphore();
  void RegisterSHM();
  void RegisterSignals(FEX::HLE::SyscallHandler *const Handler);
  void RegisterSocket();
  void RegisterThread(FEX::HLE::SyscallHandler *const Handler);
  void RegisterTime();
  void RegisterTimer();
  void RegisterNotImplemented();
  void RegisterStubs();

uint64_t UnimplementedSyscall(FEXCore::Core::CpuStateFrame *Frame, uint64_t SyscallNumber);
uint64_t UnimplementedSyscallSafe(FEXCore::Core::CpuStateFrame *Frame, uint64_t SyscallNumber);

struct ExecveAtArgs {
  int dirfd;
  int flags;
};

uint64_t ExecveHandler(const char *pathname, char* const* argv, char* const* envp, ExecveAtArgs *Args);

class SyscallHandler : public FEXCore::HLE::SyscallHandler {
public:
  SyscallHandler(FEXCore::Context::Context *ctx, FEX::HLE::SignalDelegator *_SignalDelegation);
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

  uint64_t HandleBRK(FEXCore::Core::CpuStateFrame *Frame, void *Addr);

  FEX::HLE::FileManager FM;
  FEXCore::CodeLoader *GetCodeLoader() const { return LocalLoader; }
  void SetCodeLoader(FEXCore::CodeLoader *Loader) { LocalLoader = Loader; }
  FEX::HLE::SignalDelegator *GetSignalDelegator() { return SignalDelegation; }

  FEX_CONFIG_OPT(IsInterpreter, IS_INTERPRETER);
  FEX_CONFIG_OPT(IsInterpreterInstalled, INTERPRETER_INSTALLED);
  FEX_CONFIG_OPT(Filename, APP_FILENAME);
  FEX_CONFIG_OPT(RootFSPath, ROOTFS);
  FEX_CONFIG_OPT(ThreadsConfig, THREADS);
  FEX_CONFIG_OPT(Is64BitMode, IS64BIT_MODE);

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

protected:
  std::vector<SyscallFunctionDefinition> Definitions{};
  std::mutex MMapMutex;

  // BRK management
  uint64_t DataSpace {};
  uint64_t DataSpaceSize {};
  uint64_t DataSpaceMaxSize {};
  uint64_t DataSpaceStartingSize{};

  // (Major << 24) | (Minor << 16) | Patch
  uint32_t HostKernelVersion{};
  uint32_t GuestKernelVersion{};

private:

  FEX::HLE::SignalDelegator *SignalDelegation;

  std::mutex FutexMutex;
  std::mutex SyscallMutex;
  FEXCore::CodeLoader *LocalLoader{};

  #ifdef DEBUG_STRACE
    void Strace(FEXCore::HLE::SyscallArguments *Args, uint64_t Ret);
  #endif

  std::unique_ptr<FEX::HLE::MemAllocator> Alloc32Handler{};
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
struct ArgToFmtString {
  // fail on unknown types
};

#define ARG_TO_STR(tpy, str) template<> struct FEX::HLE::ArgToFmtString<tpy> { inline static const std::string Format = str; };

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
  inline static const std::string Format = "%p";
};

// Use ArgToFmtString and variadic template to create a format string from an args list
template<typename ...Args>
std::string CollectArgsFmtString() {
  std::string array[] = { ArgToFmtString<Args>::Format... };

  std::string rv{};
  bool first = true;

  for (auto &str: array) {
    if (!first) rv += ", ";
    first = false;
    rv += str;
  }

  return rv;
}
#else
#define ARG_TO_STR(tpy, str)
#endif

// Helper that allows us to create a variadic template lambda from a given signature
// by creating a function that expects a fuction pointer with the given signature as a parameter
template <typename T>
struct FunctionToLambda;

template<typename R, typename... Args>
struct FunctionToLambda<R(*)(Args...)> {
	using RType = R;

	static R(*ReturnFunctionPointer(R(*fn)(FEXCore::Core::CpuStateFrame *Frame, Args...)))(FEXCore::Core::CpuStateFrame *Frame, Args...) {
		return fn;
	}
};

// copy to match noexcept functions
template<typename R, typename... Args>
struct FunctionToLambda<R(*)(Args...) noexcept> {
	using RType = R;

	static R(*ReturnFunctionPointer(R(*fn)(FEXCore::Core::CpuStateFrame *Frame, Args...)))(FEXCore::Core::CpuStateFrame *Frame, Args...) {
		return fn;
	}
};

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

}

// Registers syscall for both 32bit and 64bit
#define REGISTER_SYSCALL_IMPL(name, lambda) \
  struct impl_##name { \
    impl_##name() \
    { \
      FEX::HLE::x64::RegisterSyscall(FEX::HLE::x64::SYSCALL_x64_##name, ~0, #name, lambda); \
      FEX::HLE::x32::RegisterSyscall(FEX::HLE::x32::SYSCALL_x86_##name, ~0, #name, lambda); \
    } } impl_##name

// Registers syscall for both 32bit and 64bit
#define REGISTER_SYSCALL_IMPL_PASS(name, lambda) \
  struct impl_##name { \
    impl_##name() \
    { \
      FEX::HLE::x64::RegisterSyscall(FEX::HLE::x64::SYSCALL_x64_##name, SYSCALL_DEF(name), #name, lambda); \
      FEX::HLE::x32::RegisterSyscall(FEX::HLE::x32::SYSCALL_x86_##name, SYSCALL_DEF(name), #name, lambda); \
    } } impl_##name
