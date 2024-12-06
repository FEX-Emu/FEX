// SPDX-License-Identifier: MIT
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
} // namespace FEXCore::Core

namespace FEX::HLE {
struct ThreadStateObject;

FEX::HLE::ThreadStateObject* CreateNewThread(FEXCore::Context::Context* CTX, FEXCore::Core::CpuStateFrame* Frame, FEX::HLE::clone3_args* args);
uint64_t HandleNewClone(FEX::HLE::ThreadStateObject* Thread, FEXCore::Context::Context* CTX, FEXCore::Core::CpuStateFrame* Frame,
                        FEX::HLE::clone3_args* GuestArgs);
uint64_t ForkGuest(FEXCore::Core::InternalThreadState* Thread, FEXCore::Core::CpuStateFrame* Frame, uint32_t flags, void* stack,
                   size_t StackSize, pid_t* parent_tid, pid_t* child_tid, void* tls, uint64_t exit_signal);
} // namespace FEX::HLE
