/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#pragma once

#include "Tests/LinuxSyscalls/Syscalls.h"

#include <memory>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

namespace FEXCore {
  namespace Context {
    struct Context;
  }
  namespace Core {
    struct CpuStateFrame;
  }
}

namespace FEX::HLE {
class SignalDelegator;
}

namespace FEX::HLE::x32 {
#include "SyscallsEnum.h"

class MemAllocator {
public:
  virtual ~MemAllocator() = default;
  virtual void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) = 0;
  virtual int munmap(void *addr, size_t length) = 0;
  virtual void *mremap(void *old_address, size_t old_size, size_t new_size, int flags, void *new_address) = 0;
  virtual uint64_t shmat(int shmid, const void* shmaddr, int shmflg, uint32_t *ResultAddress) = 0;
  virtual uint64_t shmdt(const void* shmaddr) = 0;
};

class x32SyscallHandler final : public FEX::HLE::SyscallHandler {
public:
  x32SyscallHandler(FEXCore::Context::Context *ctx, FEX::HLE::SignalDelegator *_SignalDelegation, std::unique_ptr<MemAllocator> Allocator);

  FEX::HLE::x32::MemAllocator *GetAllocator() { return AllocHandler.get(); }

private:
  void RegisterSyscallHandlers();
  std::unique_ptr<MemAllocator> AllocHandler{};
};

std::unique_ptr<FEX::HLE::x32::MemAllocator> CreateAllocator(bool Use32BitAllocator);
std::unique_ptr<FEX::HLE::SyscallHandler> CreateHandler(FEXCore::Context::Context *ctx,
                                                        FEX::HLE::SignalDelegator *_SignalDelegation,
                                                        std::unique_ptr<MemAllocator> Allocator);

void RegisterSyscallInternal(int SyscallNumber,
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
bool RegisterSyscall(int SyscallNumber, const char *Name, R(*fn)(FEXCore::Core::CpuStateFrame *Frame, Args...)) {
#ifdef DEBUG_STRACE
  auto TraceFormatString = std::string(Name) + "(" + CollectArgsFmtString<Args...>() + ") = %ld";
#endif
  FEX::HLE::x32::RegisterSyscallInternal(SyscallNumber,
#ifdef DEBUG_STRACE
    TraceFormatString,
#endif
    reinterpret_cast<void*>(fn), sizeof...(Args));
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

}

// Registers syscall for 32bit only
#define REGISTER_SYSCALL_IMPL_X32(name, lambda) \
  struct impl_##name { \
    impl_##name() \
    { \
      FEX::HLE::x32::RegisterSyscall(x32::SYSCALL_x86_##name, #name, lambda); \
    } } impl_##name
