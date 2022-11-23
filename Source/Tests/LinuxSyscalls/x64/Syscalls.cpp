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
  void RegisterEpoll(FEX::HLE::SyscallHandler *Handler);
  void RegisterFD(FEX::HLE::SyscallHandler *Handler);
  void RegisterInfo(FEX::HLE::SyscallHandler *Handler);
  void RegisterIO(FEX::HLE::SyscallHandler *Handler);
  void RegisterIoctl(FEX::HLE::SyscallHandler *Handler);
  void RegisterMemory(FEX::HLE::SyscallHandler *Handler);
  void RegisterMsg(FEX::HLE::SyscallHandler *Handler);
  void RegisterSched(FEX::HLE::SyscallHandler *Handler);
  void RegisterSocket(FEX::HLE::SyscallHandler *Handler);
  void RegisterSemaphore(FEX::HLE::SyscallHandler *Handler);
  void RegisterSignals(FEX::HLE::SyscallHandler *Handler);
  void RegisterThread(FEX::HLE::SyscallHandler *Handler);
  void RegisterTime(FEX::HLE::SyscallHandler *Handler);
  void RegisterNotImplemented(FEX::HLE::SyscallHandler *Handler);

  x64SyscallHandler::x64SyscallHandler(FEXCore::Context::Context *ctx, FEX::HLE::SignalDelegator *_SignalDelegation)
    : SyscallHandler {ctx, _SignalDelegation} {
    OSABI = FEXCore::HLE::SyscallOSABI::OS_LINUX64;

    RegisterSyscallHandlers();
  }

  void x64SyscallHandler::RegisterSyscallHandlers() {
    auto cvt = [](auto in) {
      union {
        decltype(in) val;
        void *raw;
      } raw;
      raw.val = in;
      return raw.raw;
    };

    Definitions.resize(FEX::HLE::x64::SYSCALL_x64_MAX, SyscallFunctionDefinition {
      .NumArgs = 255,
      .Ptr = cvt(&UnimplementedSyscall),
    });

    FEX::HLE::RegisterEpoll(this);
    FEX::HLE::RegisterFD(this);
    FEX::HLE::RegisterFS(this);
    FEX::HLE::RegisterInfo(this);
    FEX::HLE::RegisterIO(this);
    FEX::HLE::RegisterIOUring(this);
    FEX::HLE::RegisterKey(this);
    FEX::HLE::RegisterMemory(this);
    FEX::HLE::RegisterMsg(this);
    FEX::HLE::RegisterNamespace(this);
    FEX::HLE::RegisterSched(this);
    FEX::HLE::RegisterSemaphore(this);
    FEX::HLE::RegisterSHM(this);
    FEX::HLE::RegisterSignals(this);
    FEX::HLE::RegisterSocket(this);
    FEX::HLE::RegisterThread(this);
    FEX::HLE::RegisterTime(this);
    FEX::HLE::RegisterTimer(this);
    FEX::HLE::RegisterNotImplemented(this);
    FEX::HLE::RegisterStubs(this);

    // 64bit specific
    FEX::HLE::x64::RegisterEpoll(this);
    FEX::HLE::x64::RegisterFD(this);
    FEX::HLE::x64::RegisterInfo(this);
    FEX::HLE::x64::RegisterIO(this);
    FEX::HLE::x64::RegisterIoctl(this);
    FEX::HLE::x64::RegisterMemory(this);
    FEX::HLE::x64::RegisterMsg(this);
    FEX::HLE::x64::RegisterSched(this);
    FEX::HLE::x64::RegisterSocket(this);
    FEX::HLE::x64::RegisterSemaphore(this);
    FEX::HLE::x64::RegisterSignals(this);
    FEX::HLE::x64::RegisterThread(this);
    FEX::HLE::x64::RegisterTime(this);
    FEX::HLE::x64::RegisterNotImplemented(this);

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
