#pragma once

#include "Interface/HLE/FileManagement.h"
#include <FEXCore/HLE/SyscallHandler.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <unordered_map>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore {
class SyscallHandler;
}

namespace FEXCore::HLE::x64 {

#include "SyscallsEnum.h"

FEXCore::SyscallHandler *CreateHandler(FEXCore::Context::Context *ctx);
void RegisterSyscallInternal(int SyscallNumber, const std::string& TraceFormatString, void* SyscallHandler, int ArgumentCount);

}

//////
/// Templates to map parameters to format string for syscalls
//////

template<typename T>
struct ArgToFmtString {
  // fail on unknown types
};

#define ARG_TO_STR(tpy, str) template<> struct ArgToFmtString<tpy> { inline static const std::string Format = str; };

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

//////
// REGISTER_SYSCALL_IMPL implementation
// Given a syscall name + a lambda, and it will generate an strace string, extract number of arguments
// and register it as a syscall handler
//////

// RegisterSyscall base
// Deduces return, args... from the function passed
// Does not work with lambas, because they are objects with operator (), not functions
template<typename R, typename ...Args>
bool RegisterSyscall(int SyscallNumber, const char *Name, R(*fn)(FEXCore::Core::InternalThreadState *Thread, Args...)) {
  auto TraceFormatString = std::string(Name) + "(" + CollectArgsFmtString<Args...>() + ") = %ld";
  FEXCore::HLE::x64::RegisterSyscallInternal(SyscallNumber, TraceFormatString, reinterpret_cast<void*>(fn), sizeof...(Args));
  return true;
}

//LambdaTraits extracts the function singature of a lambda from operator()
template<typename FPtr>
struct LambdaTraits;

template<typename T, typename C, typename ...Args>
struct LambdaTraits<T (C::*)(Args...) const>
{
    typedef T(*Type)(Args...);
};

// Generic RegisterSyscall for lambdas
// Non-capturing lambdas can be cast to function pointers, but this does not happen on argument matching
// This is some glue logic that will cast a lambda and call the base RegisterSyscall implementation
template<class F>
bool RegisterSyscall(int num, const char *name, F f){
  typedef typename LambdaTraits<decltype(&F::operator())>::Type Signature;
  return RegisterSyscall(num, name, (Signature)f);
}

// Helper to register a syscall implementation
#define REGISTER_SYSCALL_IMPL(name, lambda) do { RegisterSyscall(x64::SYSCALL_x64_##name, #name, lambda); } while(0)


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
	using r_type = R;

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

// Creates a variadic template lambda from a global function (via FunctionToLambda), then forwards the arguments to the specified function
// also handles errno
#define SYSCALL_FORWARD_ERRNO(function) \
  FunctionToLambda<decltype(&::function)>::ReturnFunctionPointer([](FEXCore::Core::InternalThreadState *Thread, auto... Args) { \
    FunctionToLambda<decltype(&::function)>::RType Result = ::function(args...); \
    do { if (Result == -1) return (FI<decltype(&::function)>::RType)-errno; return Result; } while(0); \
  })

// Creates a syscall forward from a glibc wrapper, and registers it
#define REGISTER_SYSCALL_FORWARD_ERRNO(function) do { RegisterSyscall(x64::SYSCALL_x64_##function, #function, SYSCALL_FORWARD_ERRNO(function)); } while(0)