// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Utils/AllocatorHooks.h>
#include <FEXCore/fextl/vector.h>

#include <cstdint>

namespace FEXCore::CPU {
union Relocation;
} // namespace FEXCore::CPU

namespace FEXCore::Core {
struct DebugDataSubblock {
  uint32_t HostCodeOffset;
  uint32_t HostCodeSize;
};

struct DebugDataGuestOpcode {
  uint64_t GuestEntryOffset;
  ptrdiff_t HostEntryOffset;
};

/**
 * @brief Contains debug data for a block of code for later debugger analysis
 *
 * Needs to remain around for as long as the code could be executed at least
 */
struct DebugData : public FEXCore::Allocator::FEXAllocOperators {
  uint64_t HostCodeSize; ///< The size of the code generated in the host JIT
  fextl::vector<DebugDataSubblock> Subblocks;
  fextl::vector<DebugDataGuestOpcode> GuestOpcodes;
  fextl::vector<FEXCore::CPU::Relocation>* Relocations;
};
} // namespace FEXCore::Core
