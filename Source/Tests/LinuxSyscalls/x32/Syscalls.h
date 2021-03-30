/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#pragma once

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/FileManagement.h"
#include <FEXCore/HLE/SyscallHandler.h>
#include "Tests/LinuxSyscalls/x32/Types.h"

#include <atomic>
#include <bitset>
#include <condition_variable>
#include <map>
#include <mutex>
#include <unordered_map>

namespace FEX::HLE {
class SignalDelegator;
}

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEX::HLE::x32 {
#include "SyscallsEnum.h"

class MemAllocator final {
private:
  static constexpr uint64_t PAGE_SHIFT = 12;
  static constexpr uint64_t PAGE_SIZE = 1 << PAGE_SHIFT;
  static constexpr uint64_t PAGE_MASK = (1 << PAGE_SHIFT) - 1;
  static constexpr uint64_t BASE_KEY = 16;
  const uint64_t TOP_KEY = 0xFFFF'F000ULL >> PAGE_SHIFT;

public:
  MemAllocator() {
    // First 16 pages are taken by the Linux kernel
    for (size_t i = 0; i < 16; ++i) {
      MappedPages.set(i);
    }
    // Take the top page as well
    MappedPages.set(TOP_KEY);
    if (SearchDown) {
      LastScanLocation = TOP_KEY;
      LastKeyLocation = TOP_KEY;
      FindPageRangePtr = &MemAllocator::FindPageRange_TopDown;
    }
    else {
      LastScanLocation = BASE_KEY;
      LastKeyLocation = BASE_KEY;
      FindPageRangePtr = &MemAllocator::FindPageRange;
    }
  }
  void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
  int munmap(void *addr, size_t length);
  void *mremap(void *old_address, size_t old_size, size_t new_size, int flags, void *new_address);
  uint64_t shmat(int shmid, const void* shmaddr, int shmflg, uint32_t *ResultAddress);
  uint64_t shmdt(const void* shmaddr);
  static constexpr bool SearchDown = true;

  // PageAddr is a page already shifted to page index
  // PagesLength is the number of pages
  void SetUsedPages(uint64_t PageAddr, size_t PagesLength) {
    // Set the range as mapped
    for (size_t i = 0; i < PagesLength; ++i) {
      MappedPages.set(PageAddr + i);
    }
  }

  // PageAddr is a page already shifted to page index
  // PagesLength is the number of pages
  void SetFreePages(uint64_t PageAddr, size_t PagesLength) {
    // Set the range as unused
    for (size_t i = 0; i < PagesLength; ++i) {
      MappedPages.reset(PageAddr + i);
    }
  }

private:
  // Set that contains 4k mapped pages
  // This is the full 32bit memory range
  std::bitset<0x10'0000> MappedPages;
  std::map<uint32_t, int> PageToShm{};
  uint64_t LastScanLocation{};
  uint64_t LastKeyLocation{};
  std::mutex AllocMutex{};
  uint64_t FindPageRange(uint64_t Start, size_t Pages);
  uint64_t FindPageRange_TopDown(uint64_t Start, size_t Pages);
  using FindHandler = uint64_t(MemAllocator::*)(uint64_t Start, size_t Pages);
  FindHandler FindPageRangePtr{};
};

class x32SyscallHandler final : public FEX::HLE::SyscallHandler {
public:
  x32SyscallHandler(FEXCore::Context::Context *ctx, FEX::HLE::SignalDelegator *_SignalDelegation);

  FEX::HLE::x32::MemAllocator *GetAllocator() { return AllocHandler.get(); }

private:
  void RegisterSyscallHandlers();
  std::unique_ptr<MemAllocator> AllocHandler{};
};

FEX::HLE::SyscallHandler *CreateHandler(FEXCore::Context::Context *ctx, FEX::HLE::SignalDelegator *_SignalDelegation);
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

// Helpers to register a syscall implementation
// Creates a syscall forward from a glibc wrapper, and registers it
#define REGISTER_SYSCALL_FORWARD_ERRNO_X32(function) do { RegisterSyscall(x32::SYSCALL_x86_##function, #function, SYSCALL_FORWARD_ERRNO(function)); } while(0)

// Registers syscall for 32bit only
#define REGISTER_SYSCALL_IMPL_X32(name, lambda) \
  struct impl_##name { \
    impl_##name() \
    { \
      FEX::HLE::x32::RegisterSyscall(x32::SYSCALL_x86_##name, #name, lambda); \
    } } impl_##name
