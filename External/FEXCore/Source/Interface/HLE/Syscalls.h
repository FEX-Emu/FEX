#pragma once

#include "Interface/HLE/FileManagement.h"

#include <FEXCore/Core/Context.h>
#include <FEXCore/HLE/SyscallHandler.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <unordered_map>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore {

// #define DEBUG_STRACE
class SyscallHandler {
public:
  SyscallHandler(FEXCore::Context::Context *ctx);
  virtual ~SyscallHandler() = default;

  virtual uint64_t HandleSyscall(FEXCore::Core::InternalThreadState *Thread, FEXCore::HLE::SyscallArguments *Args) = 0;

  void DefaultProgramBreak(FEXCore::Core::InternalThreadState *Thread, uint64_t Addr);

  void SetFilename(std::string const &File) { FM.SetFilename(File); }
  std::string const & GetFilename() const { return FM.GetFilename(); }

  using SyscallPtrArg0 = uint64_t(*)(FEXCore::Core::InternalThreadState *Thread);
  using SyscallPtrArg1 = uint64_t(*)(FEXCore::Core::InternalThreadState *Thread, uint64_t);
  using SyscallPtrArg2 = uint64_t(*)(FEXCore::Core::InternalThreadState *Thread, uint64_t, uint64_t);
  using SyscallPtrArg3 = uint64_t(*)(FEXCore::Core::InternalThreadState *Thread, uint64_t, uint64_t, uint64_t);
  using SyscallPtrArg4 = uint64_t(*)(FEXCore::Core::InternalThreadState *Thread, uint64_t, uint64_t, uint64_t, uint64_t);
  using SyscallPtrArg5 = uint64_t(*)(FEXCore::Core::InternalThreadState *Thread, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
  using SyscallPtrArg6 = uint64_t(*)(FEXCore::Core::InternalThreadState *Thread, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

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
#ifdef DEBUG_STRACE
    std::string StraceFmt;
#endif
  };

  SyscallFunctionDefinition const *GetDefinition(uint64_t Syscall) {
    return &Definitions.at(Syscall);
  }

  uint64_t HandleBRK(FEXCore::Core::InternalThreadState *Thread, void *Addr);
  uint64_t HandleMMAP(FEXCore::Core::InternalThreadState *Thread, void *addr, size_t length, int prot, int flags, int fd, off_t offset);

  FileManager FM;

#ifdef DEBUG_STRACE
  virtual void Strace(FEXCore::HLE::SyscallArguments *Args, uint64_t Ret) = 0;
#endif

protected:
  std::vector<SyscallFunctionDefinition> Definitions;
  std::mutex MMapMutex;

  // BRK management
  uint64_t DataSpace {};
  uint64_t DataSpaceSize {};
  uint64_t DefaultProgramBreakAddress {};

  // MMap management
  uint64_t LastMMAP = 0x1'0000'0000;
  uint64_t ENDMMAP  = 0x2'0000'0000;

private:

  FEXCore::Context::Context *CTX;

  std::mutex FutexMutex;
  std::mutex SyscallMutex;
};

SyscallHandler *CreateHandler(Context::OperatingMode Mode, FEXCore::Context::Context *ctx);
uint64_t HandleSyscall(SyscallHandler *Handler, FEXCore::Core::InternalThreadState *Thread, FEXCore::HLE::SyscallArguments *Args);

#define SYSCALL_ERRNO() do { if (Result == -1) return -errno; return Result; } while(0)
#define SYSCALL_ERRNO_NULL() do { if (Result == 0) return -errno; return Result; } while(0)
}

namespace FEXCore::HLE {

#ifdef DEBUG_STRACE
//////
/// Templates to map parameters to format string for syscalls
//////

template<typename T>
struct ArgToFmtString {
  // fail on unknown types
};

#define ARG_TO_STR(tpy, str) template<> struct FEXCore::HLE::ArgToFmtString<tpy> { inline static const std::string Format = str; };

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

  std::string rv;
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

/////
// REGISTER_SYSCALL_FORWARD_ERRNO implementation
// Given a syscall wrapper, it generate a syscall implementation using the wrapper's signature, forward the arguments
// and register to syscalls via RegisterSyscall
/////

// Helper that allows us to create a variadic template lambda from a given signature
// by creating a function that expects a fuction pointer with the given signature as a parameter
template <typename T>
struct FunctionToLambda;

template<typename R, typename... Args>
struct FunctionToLambda<R(*)(Args...)> {
	using RType = R;

	static R(*ReturnFunctionPointer(R(*fn)(FEXCore::Core::InternalThreadState *Thread, Args...)))(FEXCore::Core::InternalThreadState *Thread, Args...) {
		return fn;
	}
};

// copy to match noexcept functions
template<typename R, typename... Args>
struct FunctionToLambda<R(*)(Args...) noexcept> {
	using RType = R;

	static R(*ReturnFunctionPointer(R(*fn)(FEXCore::Core::InternalThreadState *Thread, Args...)))(FEXCore::Core::InternalThreadState *Thread, Args...) {
		return fn;
	}
};
}

// Creates a variadic template lambda from a global function (via FunctionToLambda), then forwards the arguments to the specified function
// also handles errno
#define SYSCALL_FORWARD_ERRNO(function) \
  FEXCore::HLE::FunctionToLambda<decltype(&::function)>::ReturnFunctionPointer([](FEXCore::Core::InternalThreadState *Thread, auto... Args) { \
    FEXCore::HLE::FunctionToLambda<decltype(&::function)>::RType Result = ::function(Args...); \
    do { if (Result == -1) return (FEXCore::HLE::FunctionToLambda<decltype(&::function)>::RType)-errno; return Result; } while(0); \
  })

// Helpers to register a syscall implementation
// Creates a syscall forward from a glibc wrapper, and registers it
#define REGISTER_SYSCALL_FORWARD_ERRNO(function) do { \
  FEXCore::HLE::x64::RegisterSyscall(x64::SYSCALL_x64_##function, #function, SYSCALL_FORWARD_ERRNO(function)); \
  FEXCore::HLE::x32::RegisterSyscall(x32::SYSCALL_x86_##function, #function, SYSCALL_FORWARD_ERRNO(function)); \
  } while(0)

// Registers syscall for both 32bit and 64bit
#define REGISTER_SYSCALL_IMPL(name, lambda) \
  struct impl_##name { \
    impl_##name() \
    { \
      FEXCore::HLE::x64::RegisterSyscall(x64::SYSCALL_x64_##name, #name, lambda); \
      FEXCore::HLE::x32::RegisterSyscall(x32::SYSCALL_x86_##name, #name, lambda); \
    } } impl_##name

