// SPDX-License-Identifier: MIT
#include <FEXHeaderUtils/Filesystem.h>
#include <Interface/Context/Context.h>

#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/HLE/SourcecodeResolver.h>

#include <Interface/GDBJIT/GDBJIT.h>

#include <xxhash.h>

namespace FEXCore {

ExecutableFileInfo::~ExecutableFileInfo() = default;

} // namespace FEXCore

namespace FEXCore::Context {

CodeCache::CodeCache(ContextImpl& CTX_)
  : CTX(CTX_) {}
CodeCache::~CodeCache() = default;

void CodeCache::LoadData(Core::InternalThreadState& Thread, std::byte* MappedCacheFile, const ExecutableFileSectionInfo& GuestRIPLookup) {
  // TODO
}

bool CodeCache::SaveData(Core::InternalThreadState& Thread, int fd, const ExecutableFileSectionInfo& SourceBinary, uint64_t SerializedBaseAddress) {
  // TODO
  return true;
}

void CodeCache::PostCompileCode(FEXCore::Core::InternalThreadState& Thread, void* CodePtr, uint64_t GuestRIP, FEXCore::Core::DebugData& DebugData) {
  if (CTX.Config.LibraryJITNaming() || CTX.Config.GDBSymbols()) {
    auto MappedSection = CTX.SyscallHandler->LookupExecutableFileSection(Thread, GuestRIP);
    if (MappedSection) {
      if (CTX.Config.LibraryJITNaming()) {
        CTX.Symbols.RegisterNamedRegion(Thread.SymbolBuffer.get(), CodePtr, DebugData.HostCodeSize, MappedSection->FileInfo.Filename);
      }

      if (CTX.Config.GDBSymbols()) {
        GDBJITRegister(MappedSection->FileInfo, MappedSection->FileStartVA, GuestRIP, (uintptr_t)CodePtr, DebugData);
      }
    }
  }
}

fextl::string CodeCache::ComputeCodeMapId(std::string_view Filename) {
  auto base_filename = FHU::Filesystem::GetFilename(Filename);
  if (!base_filename.empty()) {
    auto filename_hash = XXH3_64bits(Filename.data(), Filename.size());

    return fextl::fmt::format("{}-{:016x}-{}{}{}", base_filename, filename_hash,
                              (CTX.Config.SMCChecks == FEXCore::Config::CONFIG_SMC_FULL) ? 'S' : 's', CTX.Config.TSOEnabled ? 'T' : 't',
                              CTX.Config.ABILocalFlags ? 'L' : 'l');
  }

  return nullptr;
}

} // namespace FEXCore::Context
