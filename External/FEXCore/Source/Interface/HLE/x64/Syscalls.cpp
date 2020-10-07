#include <map>

#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"

#include "LogManager.h"

namespace FEXCore::HLE {
  void RegisterEpoll();
  void RegisterFD();
  void RegisterFS();
  void RegisterInfo();
  void RegisterIO();
  void RegisterIoctl();
  void RegisterKey();
  void RegisterMemory();
  void RegisterMsg();
  void RegisterNuma();
  void RegisterSched();
  void RegisterSemaphore();
  void RegisterSHM();
  void RegisterSignals();
  void RegisterSocket();
  void RegisterThread();
  void RegisterTime();
  void RegisterTimer();
  void RegisterNotImplemented();
  void RegisterStubs();
}

namespace FEXCore::HLE::x64 {
  void RegisterFD();
  void RegisterInfo();
  void RegisterMemory();
  void RegisterSocket();
  void RegisterSemaphore();
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

  class x64SyscallHandler final : public FEXCore::SyscallHandler {
  public:
    x64SyscallHandler(FEXCore::Context::Context *ctx);

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

  x64SyscallHandler::x64SyscallHandler(FEXCore::Context::Context *ctx)
    : SyscallHandler {ctx} {
    RegisterSyscallHandlers();
  }

  void x64SyscallHandler::RegisterSyscallHandlers() {
    Definitions.resize(FEXCore::HLE::x64::SYSCALL_MAX);
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

    FEXCore::HLE::RegisterEpoll();
    FEXCore::HLE::RegisterFD();
    FEXCore::HLE::RegisterFS();
    FEXCore::HLE::RegisterInfo();
    FEXCore::HLE::RegisterIO();
    FEXCore::HLE::RegisterIoctl();
    FEXCore::HLE::RegisterKey();
    FEXCore::HLE::RegisterMemory();
    FEXCore::HLE::RegisterMsg();
    FEXCore::HLE::RegisterSched();
    FEXCore::HLE::RegisterSemaphore();
    FEXCore::HLE::RegisterSHM();
    FEXCore::HLE::RegisterSignals();
    FEXCore::HLE::RegisterSocket();
    FEXCore::HLE::RegisterThread();
    FEXCore::HLE::RegisterTime();
    FEXCore::HLE::RegisterTimer();
    FEXCore::HLE::RegisterNotImplemented();
    FEXCore::HLE::RegisterStubs();

    // 64bit specific
    FEXCore::HLE::x64::RegisterFD();
    FEXCore::HLE::x64::RegisterInfo();
    FEXCore::HLE::x64::RegisterMemory();
    FEXCore::HLE::x64::RegisterSocket();
    FEXCore::HLE::x64::RegisterSemaphore();
    FEXCore::HLE::x64::RegisterNotImplemented();

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

  FEXCore::SyscallHandler *CreateHandler(FEXCore::Context::Context *ctx) {
    return new x64SyscallHandler(ctx);
  }

}
