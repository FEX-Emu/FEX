#include <FEXCore/Utils/LogManager.h>
#include "Common/MathUtils.h"

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x64/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"

#include <FEXCore/Core/X86Enums.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/futex.h>
#include <numaif.h>
#include <poll.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/random.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <sys/shm.h>
#include <sys/syscall.h>
#include <unistd.h>

#define BRK_SIZE 0x1000'0000

namespace FEX::HLE {
SyscallHandler *_SyscallHandler{};

uint64_t SyscallHandler::HandleBRK(FEXCore::Core::InternalThreadState *Thread, void *Addr) {
  std::lock_guard<std::mutex> lk(MMapMutex);
  uint64_t Result;

  if (DataSpace == 0) {
    DefaultProgramBreak(Thread);
  }

  if (Addr == nullptr) { // Just wants to get the location of the program break atm
    Result = DataSpace + DataSpaceSize;
  }
  else {
    // Allocating out data space
    uint64_t NewEnd = reinterpret_cast<uint64_t>(Addr);
    if (NewEnd < DataSpace) {
      // Not allowed to move brk end below original start
      // Set the size to zero
      DataSpaceSize = 0;
    }
    else {
      uint64_t NewSize = NewEnd - DataSpace;

      // make sure we don't overflow to TLS storage
      if (NewSize >= BRK_SIZE)
        return -ENOMEM;

      DataSpaceSize = NewSize;
    }
    Result = DataSpace + DataSpaceSize;
  }
  return Result;
}

void SyscallHandler::DefaultProgramBreak(FEXCore::Core::InternalThreadState *Thread) {
  DataSpaceSize = 0;
  DataSpace = (uint64_t)mmap(nullptr, BRK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

SyscallHandler::SyscallHandler(FEXCore::Context::Context *ctx, FEX::HLE::SignalDelegator *_SignalDelegation)
  : FM {ctx}
  , SignalDelegation {_SignalDelegation} {
  FEX::HLE::_SyscallHandler = this;
}

uint64_t SyscallHandler::HandleSyscall(FEXCore::Core::InternalThreadState *Thread, FEXCore::HLE::SyscallArguments *Args) {
  auto &Def = Definitions[Args->Argument[0]];
  uint64_t Result{};
  switch (Def.NumArgs) {
  case 0: Result = std::invoke(Def.Ptr0, Thread); break;
  case 1: Result = std::invoke(Def.Ptr1, Thread, Args->Argument[1]); break;
  case 2: Result = std::invoke(Def.Ptr2, Thread, Args->Argument[1], Args->Argument[2]); break;
  case 3: Result = std::invoke(Def.Ptr3, Thread, Args->Argument[1], Args->Argument[2], Args->Argument[3]); break;
  case 4: Result = std::invoke(Def.Ptr4, Thread, Args->Argument[1], Args->Argument[2], Args->Argument[3], Args->Argument[4]); break;
  case 5: Result = std::invoke(Def.Ptr5, Thread, Args->Argument[1], Args->Argument[2], Args->Argument[3], Args->Argument[4], Args->Argument[5]); break;
  case 6: Result = std::invoke(Def.Ptr6, Thread, Args->Argument[1], Args->Argument[2], Args->Argument[3], Args->Argument[4], Args->Argument[5], Args->Argument[6]); break;
  // for missing syscalls
  case 255: return std::invoke(Def.Ptr1, Thread, Args->Argument[0]);
  default:
    LogMan::Msg::A("Unhandled syscall: %d", Args->Argument[0]);
    return -1;
  break;
  }
#ifdef DEBUG_STRACE
  Strace(Args, Result);
#endif
  return Result;
}

#ifdef DEBUG_STRACE
void SyscallHandler::Strace(FEXCore::HLE::SyscallArguments *Args, uint64_t Ret) {
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

FEX::HLE::SyscallHandler *CreateHandler(FEXCore::Context::OperatingMode Mode, FEXCore::Context::Context *ctx, FEX::HLE::SignalDelegator *_SignalDelegation) {
  if (Mode == FEXCore::Context::MODE_64BIT) {
    return FEX::HLE::x64::CreateHandler(ctx, _SignalDelegation);
  }
  else {
    return FEX::HLE::x32::CreateHandler(ctx, _SignalDelegation);
  }
}

}
