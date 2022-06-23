/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#pragma once
#include <stdint.h>
#include <sys/types.h>

namespace FEXCore::Core {
struct InternalThreadState;
struct CPUState;
}

namespace FEX::HLE {
  FEXCore::Core::InternalThreadState *CreateNewThread(FEXCore::Context::Context *CTX, FEXCore::Core::CpuStateFrame *Frame, FEX::HLE::clone3_args *args);
  uint64_t HandleNewClone(FEXCore::Core::InternalThreadState *Thread, FEXCore::Context::Context *CTX, FEXCore::Core::CpuStateFrame *Frame, FEX::HLE::clone3_args *GuestArgs);
  uint64_t ForkGuest(FEXCore::Core::InternalThreadState *Thread, FEXCore::Core::CpuStateFrame *Frame, uint32_t flags, void *stack, size_t StackSize, pid_t *parent_tid, pid_t *child_tid, void *tls);
}
