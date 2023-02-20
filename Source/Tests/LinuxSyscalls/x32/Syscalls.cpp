/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#include "Tests/LinuxSyscalls/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/IoctlEmulation.h"
#include "Tests/LinuxSyscalls/x32/Syscalls.h"
#include "Tests/LinuxSyscalls/x32/SyscallsEnum.h"

#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/Utils/LogManager.h>

#include <bitset>
#include <cerrno>
#include <cstdint>
#include <limits>
#include <map>
#include <mutex>
#include <sys/mman.h>
#include <sys/shm.h>
#include <utility>
#include <vector>

namespace FEX::HLE::x32 {
  void RegisterEpoll(FEX::HLE::SyscallHandler *Handler);
  void RegisterFD(FEX::HLE::SyscallHandler *Handler);
  void RegisterFS(FEX::HLE::SyscallHandler *Handler);
  void RegisterInfo(FEX::HLE::SyscallHandler *Handler);
  void RegisterIO(FEX::HLE::SyscallHandler *Handler);
  void RegisterMemory(FEX::HLE::SyscallHandler *Handler);
  void RegisterMsg(FEX::HLE::SyscallHandler *Handler);
  void RegisterNotImplemented(FEX::HLE::SyscallHandler *Handler);
  void RegisterSched(FEX::HLE::SyscallHandler *Handler);
  void RegisterSemaphore(FEX::HLE::SyscallHandler *Handler);
  void RegisterSignals(FEX::HLE::SyscallHandler *Handler);
  void RegisterSocket(FEX::HLE::SyscallHandler *Handler);
  void RegisterStubs(FEX::HLE::SyscallHandler *Handler);
  void RegisterThread(FEX::HLE::SyscallHandler *Handler);
  void RegisterTime(FEX::HLE::SyscallHandler *Handler);
  void RegisterTimer(FEX::HLE::SyscallHandler *Handler);

  x32SyscallHandler::x32SyscallHandler(FEXCore::Context::Context *ctx, FEX::HLE::SignalDelegator *_SignalDelegation, std::unique_ptr<MemAllocator> Allocator)
    : SyscallHandler{ctx, _SignalDelegation}, AllocHandler{std::move(Allocator)} {
    OSABI = FEXCore::HLE::SyscallOSABI::OS_LINUX32;
    RegisterSyscallHandlers();
  }

  void x32SyscallHandler::RegisterSyscallHandlers() {
    auto cvt = [](auto in) {
      union {
        decltype(in) val;
        void *raw;
      } raw;
      raw.val = in;
      return raw.raw;
    };

    Definitions.resize(FEX::HLE::x32::SYSCALL_x86_MAX, SyscallFunctionDefinition {
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

    // 32bit specific
    FEX::HLE::x32::RegisterEpoll(this);
    FEX::HLE::x32::RegisterFD(this);
    FEX::HLE::x32::RegisterFS(this);
    FEX::HLE::x32::RegisterInfo(this);
    FEX::HLE::x32::RegisterIO(this);
    FEX::HLE::x32::RegisterMemory(this);
    FEX::HLE::x32::RegisterMsg(this);
    FEX::HLE::x32::RegisterNotImplemented(this);
    FEX::HLE::x32::RegisterSched(this);
    FEX::HLE::x32::RegisterSemaphore(this);
    FEX::HLE::x32::RegisterSignals(this);
    FEX::HLE::x32::RegisterSocket(this);
    FEX::HLE::x32::RegisterStubs(this);
    FEX::HLE::x32::RegisterThread(this);
    FEX::HLE::x32::RegisterTime(this);
    FEX::HLE::x32::RegisterTimer(this);

    FEX::HLE::x32::InitializeStaticIoctlHandlers();

#if PRINT_MISSING_SYSCALLS
    for (auto &Syscall: SyscallNames) {
      if (Definitions[Syscall.first].Ptr == cvt(&UnimplementedSyscall)) {
        LogMan::Msg::DFmt("Unimplemented syscall: {}: {}", Syscall.first, Syscall.second);
      }
    }
#endif
  }

  std::unique_ptr<FEX::HLE::SyscallHandler> CreateHandler(FEXCore::Context::Context *ctx, FEX::HLE::SignalDelegator *_SignalDelegation, std::unique_ptr<MemAllocator> Allocator) {
    return std::make_unique<x32SyscallHandler>(ctx, _SignalDelegation, std::move(Allocator));
  }

}
