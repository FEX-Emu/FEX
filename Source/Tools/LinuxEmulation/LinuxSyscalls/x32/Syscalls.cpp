// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-x86-32
$end_info$
*/

#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x32/IoctlEmulation.h"
#include "LinuxSyscalls/x32/Syscalls.h"
#include "LinuxSyscalls/x32/SyscallsEnum.h"

#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/memory.h>

#include <bitset>
#include <cerrno>
#include <cstdint>
#include <limits>
#include <mutex>
#include <sys/mman.h>
#include <sys/shm.h>
#include <utility>

namespace FEX::HLE::x32 {

x32SyscallHandler::x32SyscallHandler(FEXCore::Context::Context* ctx, FEX::HLE::SignalDelegator* _SignalDelegation,
                                     FEX::HLE::ThunkHandler* ThunkHandler, fextl::unique_ptr<MemAllocator> Allocator)
  : SyscallHandler {ctx, _SignalDelegation, ThunkHandler}
  , AllocHandler {std::move(Allocator)} {
  OSABI = FEXCore::HLE::SyscallOSABI::OS_LINUX32;
  RegisterSyscallHandlers();
}

void x32SyscallHandler::RegisterSyscallHandlers() {
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
  FEX::HLE::x32::RegisterPassthrough(this);

#if PRINT_MISSING_SYSCALLS
  for (auto& Syscall : SyscallNames) {
    if (Definitions[Syscall.first].Ptr == reinterpret_cast<void*>(&UnimplementedSyscall)) {
      LogMan::Msg::DFmt("Unimplemented syscall: {}: {}", Syscall.first, Syscall.second);
    }
  }
#endif
}

fextl::unique_ptr<FEX::HLE::SyscallHandler> CreateHandler(FEXCore::Context::Context* ctx, FEX::HLE::SignalDelegator* _SignalDelegation,
                                                          FEX::HLE::ThunkHandler* ThunkHandler, fextl::unique_ptr<MemAllocator> Allocator) {
  return fextl::make_unique<x32SyscallHandler>(ctx, _SignalDelegation, ThunkHandler, std::move(Allocator));
}

} // namespace FEX::HLE::x32
