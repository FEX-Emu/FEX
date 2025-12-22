// SPDX-License-Identifier: MIT

#include <array>
#include <mutex>
#include <filesystem>
#include <optional>
#include <string_view>
#include <cctype>

#include <FEXCore/Core/Context.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/HLE/SourcecodeResolver.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/SignalScopeGuards.h>
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>
#include "Common/Config.h"

#include <fcntl.h>
#include <io.h>
#include <winternl.h>
#include <xxhash.h>

#include "Handle.h"
#include "Module.h"
#include "Priv.h"
#include "ImageTracker.h"

namespace FEX::Windows {
static fextl::string ToLower(std::string_view String) {
  fextl::string Res;
  Res.resize(String.size());
  std::transform(String.begin(), String.end(), Res.begin(), [](unsigned char c) { return std::tolower(c); });
  return Res;
}

FEXCore::CodeMapFileId ComputeCodeMapId(std::string_view FileName, uint32_t TimeDateStamp, uint32_t SizeOfImage) {
  const auto Norm {ToLower(FileName)};
  return XXH3_64bits(Norm.data(), Norm.size()) ^ (static_cast<uint64_t>(SizeOfImage) << 32 | TimeDateStamp);
}

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

static fextl::unordered_map<uint32_t, FEXCore::GuestRelocationType> LoadImageRelocations(ArchImageNtHeaders* Nt, uint64_t Address) {
  const auto Module = reinterpret_cast<HMODULE>(Address);
  ULONG Size;

  const auto RelocationBlocksBegin =
    reinterpret_cast<uint64_t>(RtlImageDirectoryEntryToData(Module, true, IMAGE_DIRECTORY_ENTRY_BASERELOC, &Size));
  if (!RelocationBlocksBegin) {
    return {};
  }

  fextl::unordered_map<uint32_t, FEXCore::GuestRelocationType> Result;
  const uint64_t RelocationBlocksEnd = RelocationBlocksBegin + Size - sizeof(IMAGE_BASE_RELOCATION);
  for (uint64_t CurrentRelocation = RelocationBlocksBegin; CurrentRelocation < RelocationBlocksEnd;) {
    const auto* Block = reinterpret_cast<IMAGE_BASE_RELOCATION*>(CurrentRelocation);
    if (!Block->SizeOfBlock) {
      break;
    }
    const uint64_t BlockEnd = CurrentRelocation + Block->SizeOfBlock; // Includes the size of IMAGE_BASE_RELOCATION
    CurrentRelocation += sizeof(IMAGE_BASE_RELOCATION);

    for (; CurrentRelocation < BlockEnd; CurrentRelocation += 2) {
      auto PackedRelocation = *reinterpret_cast<uint16_t*>(CurrentRelocation);
      uint32_t RelocatedRVA = Block->VirtualAddress + (PackedRelocation & 0xfff);
      uint8_t Type = PackedRelocation >> 12;

      switch (Type) {
      case IMAGE_REL_BASED_ABSOLUTE: break;
      case IMAGE_REL_BASED_HIGHLOW: Result[RelocatedRVA] = FEXCore::GuestRelocationType::Rel32; break;
      case IMAGE_REL_BASED_DIR64: Result[RelocatedRVA] = FEXCore::GuestRelocationType::Rel64; break;
      default: ERROR_AND_DIE_FMT("Unhandled relocation");
      }
    }
  }

  return Result;
}

ImageTracker::ImageTracker(FEXCore::Context::Context& CTX, bool IsGeneratingCache)
  : CTX {CTX}
  , ExtendedMetaData {FEX::VolatileMetadata::ParseExtendedVolatileMetadata(ExtendedVolatileMetadataConfig())}
  , IsGeneratingCache {IsGeneratingCache} {}

ImageTracker::MappedImageInfo::MappedImageInfo(std::string_view Path, uint64_t Address, ArchImageNtHeaders* Nt,
                                               fextl::unordered_map<uint32_t, FEXCore::GuestRelocationType> Relocations)
  : Info {.FileId = ComputeCodeMapId(BaseName(Path), Nt->FileHeader.TimeDateStamp, Nt->OptionalHeader.SizeOfImage),
          .Filename = ToLower(Path), // Normalize path case as Windows paths are case-insensitive
          .Relocations = std::move(Relocations)}
  , SectionInfo {.FileInfo = Info, .FileStartVA = Address, .BeginVA = Address, .EndVA = Address + Nt->OptionalHeader.SizeOfImage} {}

FEXCore::ExecutableFileSectionInfo ImageTracker::HandleImageMap(std::string_view Path, uint64_t Address, bool MainImage) {
  std::scoped_lock Lock(CTX.GetCodeInvalidationMutex());
  const fextl::string ModuleName {BaseName(Path)};
  const auto Module = reinterpret_cast<HMODULE>(Address);
  auto* Nt = reinterpret_cast<ArchImageNtHeaders*>(RtlImageNtHeader(Module));
  MappedImageInfo* ImageInfo = nullptr;
  {
    auto Relocations = [&]() {
      if (IsGeneratingCache) {
        return LoadImageRelocations(Nt, Address);
      }
      return fextl::unordered_map<uint32_t, FEXCore::GuestRelocationType> {};
    }();
    std::unique_lock Lk {ImagesLock};
    auto [It, Inserted] = MappedImages.emplace(std::piecewise_construct, std::forward_as_tuple(Address),
                                               std::forward_as_tuple(Path, Address, Nt, std::move(Relocations)));

    if (!Inserted) {
      return It->second.SectionInfo;
    }

    ImageInfo = &It->second;
  }

  auto ID = FEXCore::CodeMap::GetBaseFilename(ImageInfo->Info, false);
  LogMan::Msg::DFmt("Load module {} ({}): {:X}", ModuleName, ID, Address);

  if (FEXCore::Config::Get_ENABLECODECACHINGWIP() && !IsGeneratingCache) {
    if (MainImage) {
      LARGE_INTEGER Time;
      NtQuerySystemTime(&Time);
      const auto CodeMapDir = fmt::format("{}codemap\\new\\", FEX::Config::GetCacheDirectory());
      std::error_code ec;
      if (!std::filesystem::exists(CodeMapDir, ec)) {
        std::filesystem::create_directories(CodeMapDir, ec);
      }
      if (!ec) {
        ActiveCodeMapPath = fmt::format("{}{}.{}.bin", CodeMapDir, ID, Time.QuadPart);

        auto Writer = fextl::make_unique<FEXCore::CodeMapWriter>(*this, false);
        Writer->AppendSetMainExecutable(ImageInfo->Info);
        CTX.SetCodeMapWriter(std::move(Writer));
      }
    }
  }

  uint64_t EndAddress = Address + Nt->OptionalHeader.SizeOfImage;
  fextl::set<uint64_t> VolatileInstructions {};
  FEXCore::IntervalList<uint64_t> VolatileValidRanges {};
  LoadImageVolatileMetadata(VolatileInstructions, VolatileValidRanges, Module, Nt, Address, EndAddress);
  if (auto It = ExtendedMetaData.find(ModuleName); It != ExtendedMetaData.end()) {
    FEX::VolatileMetadata::ApplyFEXExtendedVolatileMetadata(It->second, VolatileInstructions, VolatileValidRanges, Address, EndAddress);
  }

  if (!VolatileInstructions.empty() || !VolatileValidRanges.Empty()) {
    LogMan::Msg::DFmt("Loaded volatile metadata for {:X}: {} entries", Address, VolatileInstructions.size());
    CTX.AddForceTSOInformation(VolatileValidRanges, std::move(VolatileInstructions));
  }

  return ImageInfo->SectionInfo;
}

void ImageTracker::HandleImageUnmap(uint64_t Address, uint64_t Size) {
  std::scoped_lock Lock(CTX.GetCodeInvalidationMutex());
  CTX.RemoveForceTSOInformation(Address, Size);

  std::unique_lock Lk {ImagesLock};
  MappedImages.erase(Address);
}

std::optional<FEXCore::ExecutableFileSectionInfo> ImageTracker::LookupExecutableFileSection(uint64_t Address) {
  std::shared_lock Lk {ImagesLock};
  auto It = MappedImages.upper_bound(Address);
  if (It == MappedImages.begin() || std::prev(It)->second.SectionInfo.EndVA <= Address) {
    return {};
  }
  return std::prev(It)->second.SectionInfo;
}

int ImageTracker::OpenCodeMapFile() {
  if (ActiveCodeMapPath.empty()) {
    return -1;
  }
  return _sopen(ActiveCodeMapPath.c_str(), O_CREAT | O_TRUNC | O_WRONLY | O_APPEND, _SH_DENYRW, 0644);
}
} // namespace FEX::Windows
