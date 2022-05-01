/*
$info$
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/SyscallsEnum.h"

#include <FEXCore/HLE/SyscallHandler.h>

#include <map>

namespace FEX::HLE::x64 {
  void RegisterEpoll(FEX::HLE::SyscallHandler *const Handler);
  void RegisterFD();
  void RegisterInfo();
  void RegisterIO();
  void RegisterIoctl();
  void RegisterMemory(FEX::HLE::SyscallHandler *const Handler);
  void RegisterMsg();
  void RegisterSched();
  void RegisterSocket();
  void RegisterSemaphore();
  void RegisterSignals(FEX::HLE::SyscallHandler *const Handler);
  void RegisterThread();
  void RegisterTime();
  void RegisterNotImplemented();

#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
  [[nodiscard]] static const char* GetSyscallName(int SyscallNumber) {
    static const std::map<int, const char*> SyscallNames = {
      #include "SyscallsNames.inl"
    };

    const auto EntryIter = SyscallNames.find(SyscallNumber);
    if (EntryIter == SyscallNames.cend()) {
      return "[unknown syscall]";
    }

    return EntryIter->second;
  }
#endif

  struct InternalSyscallDefinition {
    int SyscallNumber;
    void* SyscallHandler;
    int ArgumentCount;
    int32_t HostSyscallNumber;
    FEXCore::IR::SyscallFlags Flags;
#ifdef DEBUG_STRACE
    std::string TraceFormatString;
#endif
  };

  std::vector<InternalSyscallDefinition> syscalls_x64;

  void RegisterSyscallInternal(int SyscallNumber,
    int32_t HostSyscallNumber,
    FEXCore::IR::SyscallFlags Flags,
#ifdef DEBUG_STRACE
    const std::string& TraceFormatString,
#endif
    void* SyscallHandler, int ArgumentCount) {
    syscalls_x64.push_back({SyscallNumber,
      SyscallHandler,
      ArgumentCount,
      HostSyscallNumber,
      Flags,
#ifdef DEBUG_STRACE
      TraceFormatString
#endif
      });
  }


  x64SyscallHandler::x64SyscallHandler(FEXCore::Context::Context *ctx, FEX::HLE::SignalDelegator *_SignalDelegation)
    : SyscallHandler {ctx, _SignalDelegation} {
    OSABI = FEXCore::HLE::SyscallOSABI::OS_LINUX64;

    RegisterSyscallHandlers();
  }

  void x64SyscallHandler::RegisterSyscallHandlers() {
    Definitions.resize(FEX::HLE::x64::SYSCALL_x64_MAX);
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
    FEX::HLE::RegisterMemory(this);
    FEX::HLE::RegisterMsg();
    FEX::HLE::RegisterNamespace(this);
    FEX::HLE::RegisterSched();
    FEX::HLE::RegisterSemaphore();
    FEX::HLE::RegisterSHM();
    FEX::HLE::RegisterSignals(this);
    FEX::HLE::RegisterSocket();
    FEX::HLE::RegisterThread(this);
    FEX::HLE::RegisterTime();
    FEX::HLE::RegisterTimer();
    FEX::HLE::RegisterNotImplemented();
    FEX::HLE::RegisterStubs();

    // 64bit specific
    FEX::HLE::x64::RegisterEpoll(this);
    FEX::HLE::x64::RegisterFD();
    FEX::HLE::x64::RegisterInfo();
    FEX::HLE::x64::RegisterIO();
    FEX::HLE::x64::RegisterIoctl();
    FEX::HLE::x64::RegisterMemory(this);
    FEX::HLE::x64::RegisterMsg();
    FEX::HLE::x64::RegisterSched();
    FEX::HLE::x64::RegisterSocket();
    FEX::HLE::x64::RegisterSemaphore();
    FEX::HLE::x64::RegisterSignals(this);
    FEX::HLE::x64::RegisterThread();
    FEX::HLE::x64::RegisterTime();
    FEX::HLE::x64::RegisterNotImplemented();

    // Set all the new definitions
    for (auto &Syscall : syscalls_x64) {
      auto SyscallNumber = Syscall.SyscallNumber;
      auto &Def = Definitions.at(SyscallNumber);
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
      auto Name = GetSyscallName(SyscallNumber);
      LOGMAN_THROW_A_FMT(Def.Ptr == cvt(&UnimplementedSyscall), "Oops overwriting sysall problem, {}, {}", SyscallNumber, Name);
#endif
      Def.Ptr = Syscall.SyscallHandler;
      Def.NumArgs = Syscall.ArgumentCount;
      Def.Flags = Syscall.Flags;
      Def.HostSyscallNumber = Syscall.HostSyscallNumber;
#ifdef DEBUG_STRACE
      Def.StraceFmt = Syscall.TraceFormatString;
#endif
    }

    // x86-64 has a gap of syscalls in the range of [335, 424) where there aren't any
    // These are defined that these must return -ENOSYS
    // This allows x86-64 to start using the common syscall numbers
    // Fill the gap to ensure that FEX doesn't assert
    constexpr int SYSCALL_GAP_BEGIN = 335;
    constexpr int SYSCALL_GAP_END = 424;
    for (int SyscallNumber = SYSCALL_GAP_BEGIN; SyscallNumber < SYSCALL_GAP_END; ++SyscallNumber) {
      auto &Def = Definitions.at(SyscallNumber);
      Def.Ptr = cvt(&UnimplementedSyscallSafe);
      Def.NumArgs = 0;
      Def.Flags = FEXCore::IR::SyscallFlags::DEFAULT;
      // This will allow our syscall optimization code to make this code more optimal
      // Unlikely to hit a hot path though
      Def.HostSyscallNumber = SYSCALL_DEF(MAX);
#ifdef DEBUG_STRACE
      Def.StraceFmt = "Invalid";
#endif
    }

#if PRINT_MISSING_SYSCALLS
    for (auto &Syscall: SyscallNames) {
      if (Definitions[Syscall.first].Ptr == cvt(&UnimplementedSyscall)) {
        LogMan::Msg::DFmt("Unimplemented syscall: %s", Syscall.second);
      }
    }
#endif
  }

  std::unique_ptr<FEX::HLE::SyscallHandler> CreateHandler(FEXCore::Context::Context *ctx, FEX::HLE::SignalDelegator *_SignalDelegation) {
    return std::make_unique<x64SyscallHandler>(ctx, _SignalDelegation);
  }
}
