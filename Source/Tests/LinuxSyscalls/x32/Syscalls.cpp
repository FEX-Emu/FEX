/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#include "Common/MathUtils.h"
#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/IoctlEmulation.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/SyscallsEnum.h"

#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/HLE/SyscallHandler.h>

#include <bitset>
#include <cstdint>
#include <errno.h>
#include <limits>
#include <map>
#include <mutex>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <utility>
#include <vector>

namespace FEXCore::Context {
  struct Context;
}

namespace FEX::HLE::x32 {
  void RegisterEpoll(FEX::HLE::SyscallHandler *const Handler);
  void RegisterFD();
  void RegisterFS();
  void RegisterInfo();
  void RegisterMemory();
  void RegisterMsg();
  void RegisterNotImplemented();
  void RegisterSched();
  void RegisterSemaphore();
  void RegisterSignals();
  void RegisterSocket();
  void RegisterThread();
  void RegisterTime();
  void RegisterTimer();

  std::map<int, const char*> SyscallNames = {
    #include "SyscallsNames.inl"
  };

  const char* GetSyscallName(int SyscallNumber) {
    const char* name = "[unknown syscall]";

    if (SyscallNames.count(SyscallNumber))
      name = SyscallNames[SyscallNumber];

    return name;
  }

  struct InternalSyscallDefinition {
    int SyscallNumber;
    void* SyscallHandler;
    int ArgumentCount;
    int32_t HostSyscallNumber;
#ifdef DEBUG_STRACE
    std::string TraceFormatString;
#endif
  };

  std::vector<InternalSyscallDefinition> syscalls_x32;

  void RegisterSyscallInternal(int SyscallNumber,
    int32_t HostSyscallNumber,
#ifdef DEBUG_STRACE
    const std::string& TraceFormatString,
#endif
    void* SyscallHandler, int ArgumentCount) {
    syscalls_x32.push_back({SyscallNumber,
      SyscallHandler,
      ArgumentCount,
      HostSyscallNumber,
#ifdef DEBUG_STRACE
      TraceFormatString
#endif
      });
  }

  x32SyscallHandler::x32SyscallHandler(FEXCore::Context::Context *ctx, FEX::HLE::SignalDelegator *_SignalDelegation, std::unique_ptr<MemAllocator> Allocator)
    : SyscallHandler{ctx, _SignalDelegation}, AllocHandler{std::move(Allocator)} {
    OSABI = FEXCore::HLE::SyscallOSABI::OS_LINUX32;
    RegisterSyscallHandlers();
  }

  void x32SyscallHandler::RegisterSyscallHandlers() {
    Definitions.resize(FEX::HLE::x32::SYSCALL_x86_MAX);
    auto cvt = [](auto in) {
      union {
        decltype(in) val;
        void *raw;
      } raw;
      raw.val = in;
      return raw.raw;
    };

    // Clear all definitions
    for (auto &Def : Definitions) {
      Def.NumArgs = 255;
      Def.Ptr = cvt(&UnimplementedSyscall);
    }

    FEX::HLE::RegisterEpoll();
    FEX::HLE::RegisterFD(this);
    FEX::HLE::RegisterFS(this);
    FEX::HLE::RegisterInfo();
    FEX::HLE::RegisterIO();
    FEX::HLE::RegisterIOUring(this);
    FEX::HLE::RegisterKey();
    FEX::HLE::RegisterMemory();
    FEX::HLE::RegisterMsg();
    FEX::HLE::RegisterNamespace(this);
    FEX::HLE::RegisterSched();
    FEX::HLE::RegisterSemaphore();
    FEX::HLE::RegisterSHM();
    FEX::HLE::RegisterSignals(this);
    FEX::HLE::RegisterSocket();
    FEX::HLE::RegisterThread();
    FEX::HLE::RegisterTime();
    FEX::HLE::RegisterTimer();
    FEX::HLE::RegisterNotImplemented();
    FEX::HLE::RegisterStubs();

    // 32bit specific
    FEX::HLE::x32::RegisterEpoll(this);
    FEX::HLE::x32::RegisterFD();
    FEX::HLE::x32::RegisterFS();
    FEX::HLE::x32::RegisterInfo();
    FEX::HLE::x32::RegisterMemory();
    FEX::HLE::x32::RegisterMsg();
    FEX::HLE::x32::RegisterNotImplemented();
    FEX::HLE::x32::RegisterSched();
    FEX::HLE::x32::RegisterSemaphore();
    FEX::HLE::x32::RegisterSignals();
    FEX::HLE::x32::RegisterSocket();
    FEX::HLE::x32::RegisterThread();
    FEX::HLE::x32::RegisterTime();
    FEX::HLE::x32::RegisterTimer();

    FEX::HLE::x32::InitializeStaticIoctlHandlers();

    // Set all the new definitions
    for (auto &Syscall : syscalls_x32) {
      auto SyscallNumber = Syscall.SyscallNumber;
      auto &Def = Definitions.at(SyscallNumber);
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
      auto Name = GetSyscallName(SyscallNumber);
      LOGMAN_THROW_A(Def.Ptr == cvt(&UnimplementedSyscall), "Oops overwriting sysall problem, %d, %s", SyscallNumber, Name);
#endif
      Def.Ptr = Syscall.SyscallHandler;
      Def.NumArgs = Syscall.ArgumentCount;
      Def.HostSyscallNumber = Syscall.HostSyscallNumber;
#ifdef DEBUG_STRACE
      Def.StraceFmt = Syscall.TraceFormatString;
#endif
    }

#if PRINT_MISSING_SYSCALLS
    for (auto &Syscall: SyscallNames) {
      if (Definitions[Syscall.first].Ptr == cvt(&UnimplementedSyscall)) {
        LogMan::Msg::D("Unimplemented syscall: %d: %s", Syscall.first, Syscall.second);
      }
    }
#endif
  }

  std::unique_ptr<FEX::HLE::SyscallHandler> CreateHandler(FEXCore::Context::Context *ctx, FEX::HLE::SignalDelegator *_SignalDelegation, std::unique_ptr<MemAllocator> Allocator) {
    return std::make_unique<x32SyscallHandler>(ctx, _SignalDelegation, std::move(Allocator));
  }

}
