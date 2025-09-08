// SPDX-License-Identifier: MIT
#include <Interface/Context/Context.h>

#include <FEXCore/HLE/SourcecodeResolver.h>

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

} // namespace FEXCore::Context
