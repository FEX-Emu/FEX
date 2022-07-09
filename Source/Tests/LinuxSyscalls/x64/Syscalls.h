/*
$info$
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#pragma once

#include "Tests/LinuxSyscalls/FileManagement.h"
#include "Tests/LinuxSyscalls/Syscalls.h"

#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/IR/IR.h>

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace FEX::HLE {
class SignalDelegator;
class SyscallHandler;
}

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEX::HLE::x64 {
#include "SyscallsEnum.h"

class x64SyscallHandler final : public FEX::HLE::SyscallHandler {
  public:
    x64SyscallHandler(FEXCore::Context::Context *ctx, FEX::HLE::SignalDelegator *_SignalDelegation);

    void *GuestMmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) override;
    int GuestMunmap(void *addr, uint64_t length) override;

  private:
    void RegisterSyscallHandlers();
};

std::unique_ptr<FEX::HLE::SyscallHandler> CreateHandler(FEXCore::Context::Context *ctx, FEX::HLE::SignalDelegator *_SignalDelegation);

extern RegisterSyscallInternalType SyscallRegisterHandler;
void RegisterSyscallInternal(int SyscallNumber,
  int32_t HostSyscallNumber,
  FEXCore::IR::SyscallFlags Flags,
#ifdef DEBUG_STRACE
  const std::string& TraceFormatString,
#endif
  void* SyscallHandler, int ArgumentCount);

//////
// REGISTER_SYSCALL_IMPL implementation
// Given a syscall name + a lambda, and it will generate an strace string, extract number of arguments
// and register it as a syscall handler
//////

// RegisterSyscall base
// Deduces return, args... from the function passed
// Does not work with lambas, because they are objects with operator (), not functions
template<typename R, typename ...Args>
void RegisterSyscall(int SyscallNumber, int32_t HostSyscallNumber, FEXCore::IR::SyscallFlags Flags, const char *Name, R(*fn)(FEXCore::Core::CpuStateFrame *Frame, Args...)) {
#ifdef DEBUG_STRACE
  auto TraceFormatString = std::string(Name) + "(" + CollectArgsFmtString<Args...>() + ") = %ld";
#endif
  FEX::HLE::x64::SyscallRegisterHandler(SyscallNumber,
    HostSyscallNumber,
    Flags,
#ifdef DEBUG_STRACE
    TraceFormatString,
#endif
    reinterpret_cast<void*>(fn), sizeof...(Args));
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
void RegisterSyscall(int num, int32_t HostSyscallNumber, FEXCore::IR::SyscallFlags Flags, const char *name, F f){
  typedef typename LambdaTraits<decltype(&F::operator())>::Type Signature;
  RegisterSyscall(num, HostSyscallNumber, Flags, name, (Signature)f);
}

}

// Registers syscall for 64bit only
#define REGISTER_SYSCALL_IMPL_X64(name, lambda) \
  struct impl_##name { \
    impl_##name() \
    { \
      FEX::HLE::x64::RegisterSyscall(x64::SYSCALL_x64_##name, ~0, FEXCore::IR::SyscallFlags::DEFAULT, #name, lambda); \
    } } impl_##name

#define REGISTER_SYSCALL_IMPL_X64_PASS(name, lambda) \
  struct impl_##name { \
    impl_##name() \
    { \
      FEX::HLE::x64::RegisterSyscall(x64::SYSCALL_x64_##name, SYSCALL_DEF(name), FEXCore::IR::SyscallFlags::DEFAULT, #name, lambda); \
    } } impl_##name

#define REGISTER_SYSCALL_IMPL_X64_FLAGS(name, flags, lambda) \
  struct impl_##name { \
    impl_##name() \
    { \
      FEX::HLE::x64::RegisterSyscall(x64::SYSCALL_x64_##name, ~0, flags, #name, lambda); \
    } } impl_##name

#define REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(name, flags, lambda) \
  struct impl_##name { \
    impl_##name() \
    { \
      FEX::HLE::x64::RegisterSyscall(x64::SYSCALL_x64_##name, SYSCALL_DEF(name), flags, #name, lambda); \
    } } impl_##name
