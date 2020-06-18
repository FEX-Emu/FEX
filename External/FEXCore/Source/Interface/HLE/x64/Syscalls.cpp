#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"

#include "LogManager.h"

namespace FEXCore::HLE {
    void RegisterEpoll();
    void RegisterFD();
    void RegisterFS();
    void RegisterInfo();
    void RegisterIoctl();
    void RegisterMemory();
    void RegisterNuma();
    void RegisterSched();
    void RegisterSemaphore();
    void RegisterSHM();
    void RegisterSignals();
    void RegisterSocket();
    void RegisterThread();
    void RegisterTime();
    void RegisterTimer();
}

namespace {
  uint64_t Unimplemented(FEXCore::Core::InternalThreadState *Thread, uint64_t SyscallNumber) {
    ERROR_AND_DIE("Unhandled system call: %d", SyscallNumber);
    return -1;
  }

  uint64_t NopSuccess(FEXCore::Core::InternalThreadState *Thread) {
    return 0;
  }
  uint64_t NopFail(FEXCore::Core::InternalThreadState *Thread) {
    return -1ULL;
  }
  uint64_t NopPerm(FEXCore::Core::InternalThreadState *Thread) {
    return -EPERM;
  }
}

namespace FEXCore::HLE::x64 {


std::vector<std::tuple<int, void*, int, std::string>> syscalls_x64;

void RegisterSyscallInternal(int num, const std::string& trace_fmt, void* fn, int nargs) {
  syscalls_x64.push_back({num, fn, nargs, trace_fmt});
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

  RegisterEpoll();
  RegisterFD();
  RegisterFS();
  RegisterInfo();
  RegisterIoctl();
  RegisterMemory();
  RegisterSched();
  RegisterSemaphore();
  RegisterSHM();
  RegisterSignals();
  RegisterSocket();
  RegisterThread();
  RegisterTime();
  RegisterTimer();

  // Set all the new definitions
  for (auto &Syscall : syscalls_x64) {
    auto &Def = Definitions.at(std::get<0>(Syscall));
    LogMan::Throw::A(Def.Ptr == cvt(&Unimplemented), "Oops overwriting sysall problem");
    Def.Ptr = std::get<1>(Syscall);
    Def.NumArgs = std::get<2>(Syscall);
    Def.StraceFmt = std::get<3>(Syscall);
  }
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
