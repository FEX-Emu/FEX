// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/IR/IR.h>

#include <cstddef>
#include <cstdint>
#include <span>

namespace FEX::HLE {
class SyscallHandler;
}

#include <span>

namespace FEX::VDSO {
struct VDSOMapping {
  void* VDSOBase {};
  size_t VDSOSize {};
  void* OptionalSigReturnMapping {};
  size_t OptionalMappingSize {};
};

struct VDSOSigReturn {
  void* VDSO_kernel_sigreturn;
  void* VDSO_kernel_rt_sigreturn;
};
VDSOMapping LoadVDSOThunks(bool Is64Bit, FEX::HLE::SyscallHandler* const Handler);
void UnloadVDSOMapping(const VDSOMapping& Mapping);

uint64_t GetVSyscallEntry(const void* VDSOBase);

const std::span<FEXCore::IR::ThunkDefinition> GetVDSOThunkDefinitions(bool Is64Bit);
const VDSOSigReturn& GetVDSOSymbols();
} // namespace FEX::VDSO
