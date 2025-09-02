// SPDX-License-Identifier: MIT
#include "FEXHeaderUtils/Filesystem.h"
#include "Interface/Context/Context.h"
#include "Interface/IR/AOTIR.h"

#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/fextl/fmt.h>

#include <Interface/Core/LookupCache.h>
#include <Interface/GDBJIT/GDBJIT.h>

#include <xxhash.h>

namespace FEXCore::IR {

bool AOTIRCaptureCache::PostCompileCode(FEXCore::Core::InternalThreadState* Thread, void* CodePtr, uint64_t GuestRIP, uint64_t StartAddr,
                                        uint64_t Length, FEXCore::Core::DebugData* DebugData) {

  // Both generated ir and LibraryJITName need a named region lookup
  if (CTX->Config.LibraryJITNaming() || CTX->Config.GDBSymbols()) {

    auto AOTIRCacheEntry = CTX->SyscallHandler->LookupAOTIRCacheEntry(Thread, GuestRIP);

    if (AOTIRCacheEntry.Entry) {
      if (DebugData && CTX->Config.LibraryJITNaming()) {
        CTX->Symbols.RegisterNamedRegion(Thread->SymbolBuffer.get(), CodePtr, DebugData->HostCodeSize, AOTIRCacheEntry.Entry->Filename);
      }

      if (CTX->Config.GDBSymbols()) {
        GDBJITRegister(AOTIRCacheEntry.Entry, AOTIRCacheEntry.VAFileStart, GuestRIP, (uintptr_t)CodePtr, DebugData);
      }
    }
  }

  return false;
}

AOTIRCacheEntry* AOTIRCaptureCache::LoadAOTIRCacheEntry(const fextl::string& filename) {
  fextl::string base_filename = FHU::Filesystem::GetFilename(filename);

  if (!base_filename.empty()) {
    auto filename_hash = XXH3_64bits(filename.c_str(), filename.size());

    auto fileid = fextl::fmt::format("{}-{}-{}{}{}", base_filename, filename_hash,
                                     (CTX->Config.SMCChecks == FEXCore::Config::CONFIG_SMC_FULL) ? 'S' : 's',
                                     CTX->Config.TSOEnabled ? 'T' : 't', CTX->Config.ABILocalFlags ? 'L' : 'l');

    std::unique_lock lk(AOTIRCacheLock);

    auto Inserted = AOTIRCache.insert({fileid, AOTIRCacheEntry {.FileId = fileid, .Filename = filename}});
    auto Entry = &(Inserted.first->second);

    return Entry;
  }

  return nullptr;
}

} // namespace FEXCore::IR
