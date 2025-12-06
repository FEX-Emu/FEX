// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#pragma once

#include "LinuxSyscalls/FileManagement.h"
#include "LinuxSyscalls/Syscalls.h"

#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/string.h>

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>

namespace FEX::HLE {
class SignalDelegator;
class SyscallHandler;
class ThunkHandler;
} // namespace FEX::HLE

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEX::HLE::x64 {
class x64SyscallHandler final : public FEX::HLE::SyscallHandler {
public:
  x64SyscallHandler(FEXCore::Context::Context* ctx, FEX::HLE::SignalDelegator* _SignalDelegation, FEX::HLE::ThunkHandler* ThunkHandler);

  void* GuestMmap(FEXCore::Core::InternalThreadState* Thread, void* addr, size_t length, int prot, int flags, int fd, off_t offset) override {
    return FEX::HLE::SyscallHandler::GuestMmap(true, Thread, addr, length, prot, flags, fd, offset);
  }
  uint64_t GuestMunmap(FEXCore::Core::InternalThreadState* Thread, void* addr, uint64_t length) override {
    return FEX::HLE::SyscallHandler::GuestMunmap(true, Thread, addr, length);
  }


  void RegisterSyscall_64(int SyscallNumber,
#ifdef DEBUG_STRACE
                          const fextl::string& TraceFormatString,
#endif
                          void* SyscallHandler, int ArgumentCount) override {
    auto& Def = Definitions.at(SyscallNumber);
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
    LOGMAN_THROW_A_FMT(Def.Ptr == reinterpret_cast<void*>(&UnimplementedSyscall), "Oops overwriting sysall problem, {}", SyscallNumber);
#endif
    Def.Ptr = SyscallHandler;
    Def.NumArgs = ArgumentCount;
#ifdef DEBUG_STRACE
    Def.StraceFmt = TraceFormatString;
#endif
  }

private:
  void RegisterSyscallHandlers();
};

fextl::unique_ptr<FEX::HLE::SyscallHandler>
CreateHandler(FEXCore::Context::Context* ctx, FEX::HLE::SignalDelegator* _SignalDelegation, FEX::HLE::ThunkHandler* ThunkHandler);

//////
// REGISTER_SYSCALL_IMPL implementation
// Given a syscall name + a lambda, and it will generate an strace string, extract number of arguments
// and register it as a syscall handler
//////

// RegisterSyscall base
// Deduces return, args... from the function passed
// Does not work with lambas, because they are objects with operator (), not functions
template<typename R, typename... Args>
void RegisterSyscall(SyscallHandler* Handler, int SyscallNumber, const char* Name, R (*fn)(FEXCore::Core::CpuStateFrame* Frame, Args...)) {
#ifdef DEBUG_STRACE
  auto TraceFormatString = fextl::string(Name) + "(" + CollectArgsFmtString<Args...>() + ") = {}";
#endif
  Handler->RegisterSyscall_64(SyscallNumber,
#ifdef DEBUG_STRACE
                              TraceFormatString,
#endif
                              reinterpret_cast<void*>(fn), sizeof...(Args));
}

// Generic RegisterSyscall for lambdas
// Non-capturing lambdas can be cast to function pointers, but this does not happen on argument matching
// This is some glue logic that will cast a lambda and call the base RegisterSyscall implementation
template<class F>
void RegisterSyscall(SyscallHandler* _Handler, int num, const char* name, F f) {
  RegisterSyscall(_Handler, num, name, +f);
}

} // namespace FEX::HLE::x64

// Registers syscall for 64bit only
#define REGISTER_SYSCALL_IMPL_X64(name, lambda)                                        \
  do {                                                                                 \
    FEX::HLE::x64::RegisterSyscall(Handler, x64::SYSCALL_x64_##name, #name, (lambda)); \
  } while (false)
