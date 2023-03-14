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


  void RegisterSyscall_64(int SyscallNumber,
    int32_t HostSyscallNumber,
    FEXCore::IR::SyscallFlags Flags,
#ifdef DEBUG_STRACE
    const std::string& TraceFormatString,
#endif
    void* SyscallHandler, int ArgumentCount) override {
    auto &Def = Definitions.at(SyscallNumber);
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
    auto cvt = [](auto in) {
      union {
        decltype(in) val;
        void *raw;
      } raw;
      raw.val = in;
      return raw.raw;
    };
    LOGMAN_THROW_A_FMT(Def.Ptr == cvt(&UnimplementedSyscall), "Oops overwriting sysall problem, {}", SyscallNumber);
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
};

std::unique_ptr<FEX::HLE::SyscallHandler> CreateHandler(FEXCore::Context::Context *ctx, FEX::HLE::SignalDelegator *_SignalDelegation);

//////
// REGISTER_SYSCALL_IMPL implementation
// Given a syscall name + a lambda, and it will generate an strace string, extract number of arguments
// and register it as a syscall handler
//////

// RegisterSyscall base
// Deduces return, args... from the function passed
// Does not work with lambas, because they are objects with operator (), not functions
template<typename R, typename ...Args>
void RegisterSyscall(SyscallHandler *Handler, int SyscallNumber, int32_t HostSyscallNumber, FEXCore::IR::SyscallFlags Flags, const char *Name, R(*fn)(FEXCore::Core::CpuStateFrame *Frame, Args...)) {
#ifdef DEBUG_STRACE
  auto TraceFormatString = std::string(Name) + "(" + CollectArgsFmtString<Args...>() + ") = %ld";
#endif
  Handler->RegisterSyscall_64(SyscallNumber,
    HostSyscallNumber,
    Flags,
#ifdef DEBUG_STRACE
    TraceFormatString,
#endif
    reinterpret_cast<void*>(fn), sizeof...(Args));
}

// Generic RegisterSyscall for lambdas
// Non-capturing lambdas can be cast to function pointers, but this does not happen on argument matching
// This is some glue logic that will cast a lambda and call the base RegisterSyscall implementation
template<class F>
void RegisterSyscall(SyscallHandler *_Handler, int num, int32_t HostSyscallNumber, FEXCore::IR::SyscallFlags Flags, const char *name, F f){
  RegisterSyscall(_Handler, num, HostSyscallNumber, Flags, name, +f);
}

}

// Registers syscall for 64bit only
#define REGISTER_SYSCALL_IMPL_X64(name, lambda) \
  REGISTER_SYSCALL_IMPL_X64_INTERNAL(name, ~0, FEXCore::IR::SyscallFlags::DEFAULT, lambda)

#define REGISTER_SYSCALL_IMPL_X64_PASS(name, lambda) \
  REGISTER_SYSCALL_IMPL_X64_INTERNAL(name, SYSCALL_DEF(name), FEXCore::IR::SyscallFlags::DEFAULT, lambda)

#define REGISTER_SYSCALL_IMPL_X64_FLAGS(name, flags, lambda) \
  REGISTER_SYSCALL_IMPL_X64_INTERNAL(name, ~0, flags, lambda)

#define REGISTER_SYSCALL_IMPL_X64_PASS_FLAGS(name, flags, lambda) \
  REGISTER_SYSCALL_IMPL_X64_INTERNAL(name, SYSCALL_DEF(name), flags, lambda)

#define REGISTER_SYSCALL_IMPL_X64_INTERNAL(name, number, flags, lambda) \
  do { \
    FEX::HLE::x64::RegisterSyscall(Handler, x64::SYSCALL_x64_##name, (number), (flags), #name, (lambda)); \
  } while (false)
