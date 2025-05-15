// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#pragma once

#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/string.h>

#include "LinuxSyscalls/Syscalls.h"

#include <memory>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

namespace FEXCore {
namespace Context {
  class Context;
}
namespace Core {
  struct CpuStateFrame;
}
} // namespace FEXCore

namespace FEX::HLE {
class SignalDelegator;
class ThunkHandler;
} // namespace FEX::HLE

namespace FEX::HLE::x32 {

class x32SyscallHandler final : public FEX::HLE::SyscallHandler {
public:
  x32SyscallHandler(FEXCore::Context::Context* ctx, FEX::HLE::SignalDelegator* _SignalDelegation, FEX::HLE::ThunkHandler* ThunkHandler,
                    fextl::unique_ptr<MemAllocator> Allocator);

  FEX::HLE::MemAllocator* GetAllocator() {
    return AllocHandler.get();
  }
  void* GuestMmap(FEXCore::Core::InternalThreadState* Thread, void* addr, size_t length, int prot, int flags, int fd, off_t offset) override;
  uint64_t GuestMunmap(FEXCore::Core::InternalThreadState* Thread, void* addr, uint64_t length) override;

  void RegisterSyscall_32(int SyscallNumber, int32_t HostSyscallNumber, FEXCore::IR::SyscallFlags Flags,
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
    Def.Flags = Flags;
    Def.HostSyscallNumber = HostSyscallNumber;
#ifdef DEBUG_STRACE
    Def.StraceFmt = TraceFormatString;
#endif
  }

private:
  void RegisterSyscallHandlers();
  fextl::unique_ptr<MemAllocator> AllocHandler {};
};

fextl::unique_ptr<FEX::HLE::SyscallHandler> CreateHandler(FEXCore::Context::Context* ctx, FEX::HLE::SignalDelegator* _SignalDelegation,
                                                          FEX::HLE::ThunkHandler* ThunkHandler, fextl::unique_ptr<MemAllocator> Allocator);
//////
// REGISTER_SYSCALL_IMPL implementation
// Given a syscall name + a lambda, and it will generate an strace string, extract number of arguments
// and register it as a syscall handler
//////

// RegisterSyscall base
// Deduces return, args... from the function passed
// Does not work with lambas, because they are objects with operator (), not functions
template<typename R, typename... Args>
void RegisterSyscall(SyscallHandler* Handler, int SyscallNumber, int32_t HostSyscallNumber, FEXCore::IR::SyscallFlags Flags,
                     const char* Name, R (*fn)(FEXCore::Core::CpuStateFrame* Frame, Args...)) {
#ifdef DEBUG_STRACE
  auto TraceFormatString = fextl::string(Name) + "(" + CollectArgsFmtString<Args...>() + ") = %ld";
#endif
  Handler->RegisterSyscall_32(SyscallNumber, HostSyscallNumber, Flags,
#ifdef DEBUG_STRACE
                              TraceFormatString,
#endif
                              reinterpret_cast<void*>(fn), sizeof...(Args));
}

// Generic RegisterSyscall for lambdas
// Non-capturing lambdas can be cast to function pointers, but this does not happen on argument matching
// This is some glue logic that will cast a lambda and call the base RegisterSyscall implementation
template<class F>
void RegisterSyscall(SyscallHandler* _Handler, int num, int32_t HostSyscallNumber, FEXCore::IR::SyscallFlags Flags, const char* name, F f) {
  RegisterSyscall(_Handler, num, HostSyscallNumber, Flags, name, +f);
}

} // namespace FEX::HLE::x32

// Registers syscall for 32bit only
#define REGISTER_SYSCALL_IMPL_X32(name, lambda) REGISTER_SYSCALL_IMPL_X32_INTERNAL(name, ~0, FEXCore::IR::SyscallFlags::DEFAULT, lambda)

#define REGISTER_SYSCALL_IMPL_X32_PASS(name, lambda) \
  REGISTER_SYSCALL_IMPL_X32_INTERNAL(name, SYSCALL_DEF(name), FEXCore::IR::SyscallFlags::DEFAULT, lambda)

#define REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL(name, hostname, lambda) \
  REGISTER_SYSCALL_IMPL_X32_INTERNAL(name, SYSCALL_DEF(hostname), FEXCore::IR::SyscallFlags::DEFAULT, lambda)

#define REGISTER_SYSCALL_IMPL_X32_FLAGS(name, flags, lambda) REGISTER_SYSCALL_IMPL_X32_INTERNAL(name, ~0, flags, lambda)

#define REGISTER_SYSCALL_IMPL_X32_PASS_FLAGS(name, flags, lambda) REGISTER_SYSCALL_IMPL_X32_INTERNAL(name, SYSCALL_DEF(name), flags, lambda)

#define REGISTER_SYSCALL_IMPL_X32_PASS_MANUAL_FLAGS(name, hostname, flags, lambda) \
  REGISTER_SYSCALL_IMPL_X32_INTERNAL(name, SYSCALL_DEF(hostname), flags, lambda)

#define REGISTER_SYSCALL_IMPL_X32_INTERNAL(name, number, flags, lambda)                             \
  do {                                                                                              \
    FEX::HLE::x32::RegisterSyscall(Handler, x32::SYSCALL_x86_##name, number, flags, #name, lambda); \
  } while (false)
