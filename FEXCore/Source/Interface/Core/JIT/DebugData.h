// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Utils/AllocatorHooks.h>
#include <FEXCore/HLE/SourcecodeResolver.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/unordered_map.h>
#include <FEXCore/fextl/vector.h>

#include <cstdint>
#include <shared_mutex>

namespace FEXCore::CPU {
union Relocation;
} // namespace FEXCore::CPU

namespace FEXCore::Core {
struct InternalThreadState;
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

namespace FEXCore::Context {
class ContextImpl;
}

namespace FEXCore::IR {

struct AOTIRCacheEntry {
  fextl::unique_ptr<FEXCore::HLE::SourcecodeMap> SourcecodeMap;
  fextl::string FileId;
  fextl::string Filename;
};

class AOTIRCaptureCache final {
public:
  AOTIRCaptureCache(FEXCore::Context::ContextImpl* ctx)
    : CTX {ctx} {}

  bool PostCompileCode(FEXCore::Core::InternalThreadState* Thread, void* CodePtr, uint64_t GuestRIP, uint64_t StartAddr, uint64_t Length,
                       FEXCore::Core::DebugData* DebugData);

  AOTIRCacheEntry* LoadAOTIRCacheEntry(const fextl::string& filename);

private:
  FEXCore::Context::ContextImpl* CTX;

  std::shared_mutex AOTIRCacheLock;

  fextl::unordered_map<fextl::string, FEXCore::IR::AOTIRCacheEntry> AOTIRCache;
};

} // namespace FEXCore::IR
