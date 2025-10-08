// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x64/Syscalls.h"
#include "LinuxSyscalls/x64/SyscallsEnum.h"

#include <FEXCore/HLE/SyscallHandler.h>

namespace FEX::HLE::x64 {
void RegisterEpoll(FEX::HLE::SyscallHandler* Handler);
void RegisterFD(FEX::HLE::SyscallHandler* Handler);
void RegisterInfo(FEX::HLE::SyscallHandler* Handler);
void RegisterMemory(FEX::HLE::SyscallHandler* Handler);
void RegisterSemaphore(FEX::HLE::SyscallHandler* Handler);
void RegisterSignals(FEX::HLE::SyscallHandler* Handler);
void RegisterThread(FEX::HLE::SyscallHandler* Handler);
void RegisterTime(FEX::HLE::SyscallHandler* Handler);
void RegisterNotImplemented(FEX::HLE::SyscallHandler* Handler);
void RegisterPassthrough(FEX::HLE::SyscallHandler* Handler);

x64SyscallHandler::x64SyscallHandler(FEXCore::Context::Context* ctx, FEX::HLE::SignalDelegator* _SignalDelegation, FEX::HLE::ThunkHandler* ThunkHandler)
  : SyscallHandler {ctx, _SignalDelegation, ThunkHandler} {
  OSABI = FEXCore::HLE::SyscallOSABI::OS_LINUX64;

  RegisterSyscallHandlers();
}

void x64SyscallHandler::RegisterSyscallHandlers() {
  FEX::HLE::RegisterEpoll(this);
  FEX::HLE::RegisterFD(this);
  FEX::HLE::RegisterFS(this);
  FEX::HLE::RegisterInfo(this);
  FEX::HLE::RegisterIO(this);
  FEX::HLE::RegisterMemory(this);
  FEX::HLE::RegisterSignals(this);
  FEX::HLE::RegisterThread(this);
  FEX::HLE::RegisterTimer(this);
  FEX::HLE::RegisterNotImplemented(this);
  FEX::HLE::RegisterStubs(this);

  // 64bit specific
  FEX::HLE::x64::RegisterEpoll(this);
  FEX::HLE::x64::RegisterFD(this);
  FEX::HLE::x64::RegisterInfo(this);
  FEX::HLE::x64::RegisterMemory(this);
  FEX::HLE::x64::RegisterSemaphore(this);
  FEX::HLE::x64::RegisterSignals(this);
  FEX::HLE::x64::RegisterThread(this);
  FEX::HLE::x64::RegisterTime(this);
  FEX::HLE::x64::RegisterNotImplemented(this);
  FEX::HLE::x64::RegisterPassthrough(this);

  // x86-64 has a gap of syscalls in the range of [335, 424) where there aren't any
  // These are defined that these must return -ENOSYS
  // This allows x86-64 to start using the common syscall numbers
  // Fill the gap to ensure that FEX doesn't assert
  constexpr int SYSCALL_GAP_BEGIN = 335;
  constexpr int SYSCALL_GAP_END = 424;

  const SyscallFunctionDefinition InvalidSyscall {
    .Ptr = reinterpret_cast<void*>(&UnimplementedSyscall),
    .HostSyscallNumber = SYSCALL_DEF(MAX),
    .NumArgs = 0,
    .Flags = FEXCore::IR::SyscallFlags::DEFAULT,
#ifdef DEBUG_STRACE
    .StraceFmt = "Invalid",
#endif
  };
  std::fill(Definitions.begin() + SYSCALL_GAP_BEGIN, Definitions.begin() + SYSCALL_GAP_END, InvalidSyscall);

#if PRINT_MISSING_SYSCALLS
  for (auto& Syscall : SyscallNames) {
    if (Definitions[Syscall.first].Ptr == reinterpret_cast<void*>(&UnimplementedSyscall)) {
      LogMan::Msg::DFmt("Unimplemented syscall: {}", Syscall.second);
    }
  }
#endif
}

fextl::unique_ptr<FEX::HLE::SyscallHandler>
CreateHandler(FEXCore::Context::Context* ctx, FEX::HLE::SignalDelegator* _SignalDelegation, FEX::HLE::ThunkHandler* ThunkHandler) {
  return fextl::make_unique<x64SyscallHandler>(ctx, _SignalDelegation, ThunkHandler);
}
} // namespace FEX::HLE::x64
