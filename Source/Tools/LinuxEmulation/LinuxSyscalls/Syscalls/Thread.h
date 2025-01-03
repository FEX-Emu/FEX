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
uint64_t ForkGuest(FEXCore::Core::InternalThreadState* Thread, FEXCore::Core::CpuStateFrame* Frame, FEX::HLE::clone3_args* args);
} // namespace FEX::HLE
