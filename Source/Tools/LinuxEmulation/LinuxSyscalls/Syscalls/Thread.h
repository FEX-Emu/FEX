// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#pragma once

#include <cstdint>
#include <sys/types.h>

namespace FEXCore::Context {
class Context;
}

namespace FEXCore::Core {
struct CpuStateFrame;
struct InternalThreadState;
} // namespace FEXCore::Core

namespace FEX::HLE {
struct clone3_args;
struct ThreadStateObject;

FEX::HLE::ThreadStateObject* CreateNewThread(FEXCore::Context::Context* CTX, FEXCore::Core::CpuStateFrame* Frame, FEX::HLE::clone3_args* args);
uint64_t HandleNewClone(FEX::HLE::ThreadStateObject* Thread, FEXCore::Context::Context* CTX, FEXCore::Core::CpuStateFrame* Frame,
                        FEX::HLE::clone3_args* GuestArgs);
uint64_t ForkGuest(FEXCore::Core::InternalThreadState* Thread, FEXCore::Core::CpuStateFrame* Frame, FEX::HLE::clone3_args* args);
} // namespace FEX::HLE
