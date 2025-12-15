// SPDX-License-Identifier: MIT

#include <array>
#include <mutex>
#include <optional>
#include <string_view>

#include <FEXCore/Core/Context.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/HLE/SourcecodeResolver.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/SignalScopeGuards.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>

#include <winternl.h>

#include "Module.h"
#include "ImageTracker.h"

namespace FEX::Windows {
static void LoadImageVolatileMetadata(fextl::set<uint64_t>& VolatileInstructions, FEXCore::IntervalList<uint64_t>& VolatileValidRanges,
                                      HMODULE Module, ArchImageNtHeaders* Nt, uint64_t Address, uint64_t EndAddress) {
  ULONG Size;

  const auto* LoadConfig =
    reinterpret_cast<ArchImageLoadConfigDirectory*>(RtlImageDirectoryEntryToData(Module, true, IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG, &Size));
  if (!LoadConfig || LoadConfig->Size <= offsetof(ArchImageLoadConfigDirectory, VolatileMetadataPointer)) {
    return;
  }

  if (LoadConfig->VolatileMetadataPointer < Address || LoadConfig->VolatileMetadataPointer + sizeof(IMAGE_VOLATILE_METADATA) >= EndAddress) {
    return;
  }

  const auto* VolatileMetadata = reinterpret_cast<IMAGE_VOLATILE_METADATA*>(LoadConfig->VolatileMetadataPointer);
  if (!VolatileMetadata || Address + VolatileMetadata->VolatileAccessTable + VolatileMetadata->VolatileAccessTableSize >= EndAddress ||
      Address + VolatileMetadata->VolatileInfoRangeTable + VolatileMetadata->VolatileInfoRangeTableSize >= EndAddress) {
    return;
  }

  const auto* VolatileAccessTableBegin = reinterpret_cast<IMAGE_VOLATILE_RVA_METADATA*>(Address + VolatileMetadata->VolatileAccessTable);
  const auto* VolatileAccessTableEnd =
    VolatileAccessTableBegin + (VolatileMetadata->VolatileAccessTableSize / sizeof(IMAGE_VOLATILE_RVA_METADATA));
  for (auto It = VolatileAccessTableBegin; It != VolatileAccessTableEnd; It++) {
    VolatileInstructions.emplace(Address + It->Rva);
  }

  const auto* VolatileInfoRangeTableBegin = reinterpret_cast<IMAGE_VOLATILE_RANGE_METADATA*>(Address + VolatileMetadata->VolatileInfoRangeTable);
  const auto* VolatileInfoRangeTableEnd =
    VolatileInfoRangeTableBegin + (VolatileMetadata->VolatileInfoRangeTableSize / sizeof(IMAGE_VOLATILE_RANGE_METADATA));
  for (auto It = VolatileInfoRangeTableBegin; It != VolatileInfoRangeTableEnd; It++) {
    VolatileValidRanges.Insert({Address + It->Rva, Address + It->Rva + It->Size});
  }
}

ImageTracker::ImageTracker(FEXCore::Context::Context& CTX)
  : CTX {CTX}
  , ExtendedMetaData {FEX::VolatileMetadata::ParseExtendedVolatileMetadata(ExtendedVolatileMetadataConfig())} {}

void ImageTracker::HandleImageMap(std::string_view Path, uint64_t Address, bool MainImage) {
  fextl::string ModuleName {BaseName(Path)};
  LogMan::Msg::DFmt("Load module {}: {:X}", ModuleName, Address);

  const auto Module = reinterpret_cast<HMODULE>(Address);
  auto* Nt = reinterpret_cast<ArchImageNtHeaders*>(RtlImageNtHeader(Module));
  uint64_t EndAddress = Address + Nt->OptionalHeader.SizeOfImage;
  fextl::set<uint64_t> VolatileInstructions {};
  FEXCore::IntervalList<uint64_t> VolatileValidRanges {};
  LoadImageVolatileMetadata(VolatileInstructions, VolatileValidRanges, Module, Nt, Address, EndAddress);
  if (auto It = ExtendedMetaData.find(ModuleName); It != ExtendedMetaData.end()) {
    FEX::VolatileMetadata::ApplyFEXExtendedVolatileMetadata(It->second, VolatileInstructions, VolatileValidRanges, Address, EndAddress);
  }

  if (!VolatileInstructions.empty() || !VolatileValidRanges.Empty()) {
    LogMan::Msg::DFmt("Loaded volatile metadata for {:X}: {} entries", Address, VolatileInstructions.size());
    std::scoped_lock Lock(CTX.GetCodeInvalidationMutex());
    CTX.AddForceTSOInformation(VolatileValidRanges, std::move(VolatileInstructions));
  }
}

void ImageTracker::HandleImageUnmap(uint64_t Address, uint64_t Size) {
  std::scoped_lock Lock(CTX.GetCodeInvalidationMutex());
  CTX.RemoveForceTSOInformation(Address, Size);
}
} // namespace FEX::Windows
