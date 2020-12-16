#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"

#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/HLE/SyscallHandler.h>

#include <map>

namespace FEX::HLE::x64 {
  void RegisterFD();
  void RegisterInfo();
  void RegisterIO();
  void RegisterIoctl();
  void RegisterMemory();
  void RegisterMsg();
  void RegisterSched();
  void RegisterSocket();
  void RegisterSemaphore();
  void RegisterThread();
  void RegisterTime();
  void RegisterNotImplemented();

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
#ifdef DEBUG_STRACE
    std::string TraceFormatString;
#endif
  };

  std::vector<InternalSyscallDefinition> syscalls_x64;

  void RegisterSyscallInternal(int SyscallNumber,
#ifdef DEBUG_STRACE
    const std::string& TraceFormatString,
#endif
    void* SyscallHandler, int ArgumentCount) {
    syscalls_x64.push_back({SyscallNumber,
      SyscallHandler,
      ArgumentCount,
#ifdef DEBUG_STRACE
      TraceFormatString
#endif
      });
  }

  class x64SyscallHandler final : public FEX::HLE::SyscallHandler {
  public:
    x64SyscallHandler(FEXCore::Context::Context *ctx, FEX::HLE::SignalDelegator *_SignalDelegation);

    // In the case that the syscall doesn't hit the optimized path then we still need to go here
    uint64_t HandleSyscall(FEXCore::Core::InternalThreadState *Thread, FEXCore::HLE::SyscallArguments *Args) override;

  #ifdef DEBUG_STRACE
    void Strace(FEXCore::HLE::SyscallArguments *Args, uint64_t Ret) override;
  #endif

  private:
    void RegisterSyscallHandlers();
  };

  #ifdef DEBUG_STRACE
  void x64SyscallHandler::Strace(FEXCore::HLE::SyscallArguments *Args, uint64_t Ret) {
    auto &Def = Definitions[Args->Argument[0]];
    switch (Def.NumArgs) {
      case 0: LogMan::Msg::D(Def.StraceFmt.c_str(), Ret); break;
      case 1: LogMan::Msg::D(Def.StraceFmt.c_str(), Args->Argument[1], Ret); break;
      case 2: LogMan::Msg::D(Def.StraceFmt.c_str(), Args->Argument[1], Args->Argument[2], Ret); break;
      case 3: LogMan::Msg::D(Def.StraceFmt.c_str(), Args->Argument[1], Args->Argument[2], Args->Argument[3], Ret); break;
      case 4: LogMan::Msg::D(Def.StraceFmt.c_str(), Args->Argument[1], Args->Argument[2], Args->Argument[3], Args->Argument[4], Ret); break;
      case 5: LogMan::Msg::D(Def.StraceFmt.c_str(), Args->Argument[1], Args->Argument[2], Args->Argument[3], Args->Argument[4], Args->Argument[5], Ret); break;
      case 6: LogMan::Msg::D(Def.StraceFmt.c_str(), Args->Argument[1], Args->Argument[2], Args->Argument[3], Args->Argument[4], Args->Argument[5], Args->Argument[6], Ret); break;
      default: break;
    }
  }
  #endif

  uint64_t Unimplemented(FEXCore::Core::InternalThreadState *Thread, uint64_t SyscallNumber) {

    auto name = GetSyscallName(SyscallNumber);

    ERROR_AND_DIE("Unhandled system call: %d, %s", SyscallNumber, name);

    return -1;
  }

  x64SyscallHandler::x64SyscallHandler(FEXCore::Context::Context *ctx, FEX::HLE::SignalDelegator *_SignalDelegation)
    : SyscallHandler {ctx, _SignalDelegation} {
    OSABI = FEXCore::HLE::SyscallOSABI::OS_LINUX64;

    RegisterSyscallHandlers();
  }

  void x64SyscallHandler::RegisterSyscallHandlers() {
    Definitions.resize(FEX::HLE::x64::SYSCALL_MAX);
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
      Def.Ptr = cvt(&Unimplemented);
    }

    FEX::HLE::RegisterEpoll();
    FEX::HLE::RegisterFD();
    FEX::HLE::RegisterFS();
    FEX::HLE::RegisterInfo();
    FEX::HLE::RegisterIO();
    FEX::HLE::RegisterKey();
    FEX::HLE::RegisterMemory();
    FEX::HLE::RegisterMsg();
    FEX::HLE::RegisterSched();
    FEX::HLE::RegisterSemaphore();
    FEX::HLE::RegisterSHM();
    FEX::HLE::RegisterSignals();
    FEX::HLE::RegisterSocket();
    FEX::HLE::RegisterThread();
    FEX::HLE::RegisterTime();
    FEX::HLE::RegisterTimer();
    FEX::HLE::RegisterNotImplemented();
    FEX::HLE::RegisterStubs();

    // 64bit specific
    FEX::HLE::x64::RegisterFD();
    FEX::HLE::x64::RegisterInfo();
    FEX::HLE::x64::RegisterIO();
    FEX::HLE::x64::RegisterIoctl();
    FEX::HLE::x64::RegisterMemory();
    FEX::HLE::x64::RegisterMsg();
    FEX::HLE::x64::RegisterSched();
    FEX::HLE::x64::RegisterSocket();
    FEX::HLE::x64::RegisterSemaphore();
    FEX::HLE::x64::RegisterThread();
    FEX::HLE::x64::RegisterTime();
    FEX::HLE::x64::RegisterNotImplemented();

    // Set all the new definitions
    for (auto &Syscall : syscalls_x64) {
      auto SyscallNumber = Syscall.SyscallNumber;
      auto Name = GetSyscallName(SyscallNumber);
      auto &Def = Definitions.at(SyscallNumber);
      LogMan::Throw::A(Def.Ptr == cvt(&Unimplemented), "Oops overwriting sysall problem, %d, %s", SyscallNumber, Name);
      Def.Ptr = Syscall.SyscallHandler;
      Def.NumArgs = Syscall.ArgumentCount;
#ifdef DEBUG_STRACE
      Def.StraceFmt = Syscall.TraceFormatString;
#endif
    }

#if PRINT_MISSING_SYSCALLS
    for (auto &Syscall: SyscallNames) {
      if (Definitions[Syscall.first].Ptr == cvt(&Unimplemented)) {
        LogMan::Msg::D("Unimplemented syscall: %s", Syscall.second);
      }
    }
#endif
  }

  uint64_t x64SyscallHandler::HandleSyscall(FEXCore::Core::InternalThreadState *Thread, FEXCore::HLE::SyscallArguments *Args) {
    auto &Def = Definitions[Args->Argument[0]];
    switch (Def.NumArgs) {
    case 0: return std::invoke(Def.Ptr0, Thread);
    case 1: return std::invoke(Def.Ptr1, Thread, Args->Argument[1]);
    case 2: return std::invoke(Def.Ptr2, Thread, Args->Argument[1], Args->Argument[2]);
    case 3: return std::invoke(Def.Ptr3, Thread, Args->Argument[1], Args->Argument[2], Args->Argument[3]);
    case 4: return std::invoke(Def.Ptr4, Thread, Args->Argument[1], Args->Argument[2], Args->Argument[3], Args->Argument[4]);
    case 5: return std::invoke(Def.Ptr5, Thread, Args->Argument[1], Args->Argument[2], Args->Argument[3], Args->Argument[4], Args->Argument[5]);
    case 6: return std::invoke(Def.Ptr6, Thread, Args->Argument[1], Args->Argument[2], Args->Argument[3], Args->Argument[4], Args->Argument[5], Args->Argument[6]);
    // for missing syscalls
    case 255: return std::invoke(Def.Ptr1, Thread, Args->Argument[0]);
    default: break;
    }

    LogMan::Msg::A("Unhandled syscall");
    return -1;
  }

  FEX::HLE::SyscallHandler *CreateHandler(FEXCore::Context::Context *ctx, FEX::HLE::SignalDelegator *_SignalDelegation) {
    return new x64SyscallHandler(ctx, _SignalDelegation);
  }
}
