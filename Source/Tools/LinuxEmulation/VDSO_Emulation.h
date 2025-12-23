// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/IR/IR.h>

#include <cstddef>
#include <cstdint>
#include <span>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEX::HLE {
class SyscallHandler;
}

namespace FEX::VDSO {
struct VDSOMapping {
  void* VDSOBase {};
  size_t VDSOSize {};
  void* X86GeneratedCodePtr {};
  size_t X86GeneratedCodeSize {};
};

struct VDSOEntrypoints {
  void* VDSO_kernel_sigreturn;
  void* VDSO_kernel_rt_sigreturn;
  void* VDSO_FEX_CallbackRET;
};
VDSOMapping LoadVDSOThunks(FEXCore::Core::InternalThreadState* Thread, bool Is64Bit, FEX::HLE::SyscallHandler* const Handler);
void UnloadVDSOMapping(FEXCore::Core::InternalThreadState* Thread, FEX::HLE::SyscallHandler* const Handler, const VDSOMapping& Mapping);

uint64_t GetVSyscallEntry(const void* VDSOBase);

const std::span<FEXCore::IR::ThunkDefinition> GetVDSOThunkDefinitions(bool Is64Bit);
const VDSOEntrypoints& GetVDSOSymbols();
} // namespace FEX::VDSO
