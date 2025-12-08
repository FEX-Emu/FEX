// SPDX-License-Identifier: MIT
#include "Utils/SpinWaitLock.h"

#include <Interface/Context/Context.h>
#include <Interface/Core/ArchHelpers/Arm64Emitter.h>
#include <Interface/Core/JIT/Relocations.h>
#include <Interface/Core/LookupCache.h>

#include <FEXCore/Core/Thunks.h>
#include <FEXCore/HLE/SourcecodeResolver.h>

#include <FEXHeaderUtils/Filesystem.h>

#include <git_version.h>

#include <xxhash.h>

#include <fstream>

namespace FEXCore {

#if __clang_major__ < 16
ExecutableFileInfo::ExecutableFileInfo(fextl::unique_ptr<HLE::SourcecodeMap> Map, uint64_t FileId, fextl::string Filename)
  : SourcecodeMap(std::move(Map))
  , FileId(FileId)
  , Filename(Filename) {}
#endif
ExecutableFileInfo::~ExecutableFileInfo() = default;

fextl::string CodeMap::GetBaseFilename(const ExecutableFileInfo& MainExecutable, bool AddNombSuffix) {
  auto FileId = MainExecutable.FileId;

  std::string_view base_filename = FHU::Filesystem::GetFilename(std::string_view {MainExecutable.Filename});
  if (FileId != 0xffff'ffff'ffff'ffff) {
    return fextl::fmt::format("{}-{:016x}{}", base_filename, MainExecutable.FileId, AddNombSuffix ? "-nomb" : "");
  }

  return "";
}

fextl::map<CodeMapFileId, CodeMap::ParsedContents> CodeMap::ParseCodeMap(std::ifstream& File) {
  fextl::map<CodeMapFileId, CodeMap::ParsedContents> Ret;
  while (true) {
    Entry Entry;
    File.read(reinterpret_cast<char*>(&Entry), sizeof(Entry));
    if (!File) {
      break;
    }

    if (Entry.FileId == LoadExternalLibrary.FileId && Entry.BlockOffset == LoadExternalLibrary.BlockOffset) {
      ExternalLibraryInfo Info;
      File.read(reinterpret_cast<char*>(&Info), sizeof(Info));

      fextl::string Filename;
      std::getline(File, Filename, '\0');

      // Align to 4-byte boundary
      char Null[4];
      File.read(Null, AlignUp(Filename.size() + 1, 4) - Filename.size() - 1);
      if (!File) {
        break;
      }
      Ret[Info.ExternalFileId].Filename = std::move(Filename);
    } else if (Entry.FileId == SetExecutableFileId {}.Marker.FileId && Entry.BlockOffset == SetExecutableFileId {}.Marker.BlockOffset) {
      CodeMapFileId ExecutableFileId;
      File.read(reinterpret_cast<char*>(&ExecutableFileId), sizeof(ExecutableFileId));
      if (!File) {
        break;
      }
      Ret[ExecutableFileId].IsExecutable = true;
    } else {
      if (!Ret.contains(Entry.FileId)) {
        LogMan::Msg::EFmt("Code map referenced unknown file id {:016x}", Entry.FileId);
      } else {
        Ret[Entry.FileId].Blocks.insert(Entry.BlockOffset);
      }
    }

    if (!File) {
      break;
    }
  }
  return Ret;
}

CodeMapWriter::CodeMapWriter(CodeMapOpener& Opener, bool OpenEagerly)
  : Buffer(4096)
  , FileOpener(Opener) {
  if (OpenEagerly) {
    CodeMapFD = FileOpener.OpenCodeMapFile();
  }
}

CodeMapWriter::~CodeMapWriter() {
  if (CodeMapFD.value_or(-1) != -1) {
    Flush(BufferOffset);
    close(*CodeMapFD);
  }
}

bool CodeMapWriter::IsWriteEnabled(const ExecutableFileSectionInfo& Section) {
  if (CodeMapFD == -1) {
    return false;
  }

  // PV libraries can't yet be read by FEXServer, so skip dumping them
  if (Section.FileInfo.Filename.starts_with("/run/pressure-vessel")) {
    return false;
  }

  if (CodeMapFD) {
    return true;
  }

  // Acquire mutex and re-check CodeMapFD to avoid race conditions
  auto lk = std::unique_lock {Mutex};
  if (!CodeMapFD) {
    CodeMapFD = FileOpener.OpenCodeMapFile();
  }

  return CodeMapFD != -1;
}

void CodeMapWriter::Flush(size_t Offset) {
  // Acquire exclusive lock and flush circular buffer
  std::unique_lock Lock {Mutex};
  Flush(Offset, Lock);
}

void CodeMapWriter::Flush(size_t Offset, std::unique_lock<std::shared_mutex>&) {
  write(*CodeMapFD, Buffer.data(), Offset);
  BufferOffset = 0;
}

void CodeMapWriter::AppendBlock(const FEXCore::ExecutableFileSectionInfo& SectionInfo, uint64_t BlockEntry) {
  if (!IsWriteEnabled(SectionInfo)) {
    return;
  }

  BlockEntry -= SectionInfo.FileStartVA;
  if (BlockEntry > std::numeric_limits<uint32_t>::max()) {
    ERROR_AND_DIE_FMT("Cannot write code map");
  }

  // Register new library if not already known
  bool NewLibraryLoad = false;
  {
    // Check prior registration with shared lock
    std::shared_lock Lock {Mutex};
    NewLibraryLoad = !KnownFileIds.contains(SectionInfo.FileInfo.FileId);
  }
  if (NewLibraryLoad) {
    // Register to map with exclusive lock
    std::unique_lock Lock {Mutex};
    NewLibraryLoad &= KnownFileIds.insert(SectionInfo.FileInfo.FileId).second;
  }
  if (NewLibraryLoad) {
    // Add entry to code map
    AppendLibraryLoad(SectionInfo.FileInfo);
  }

  // Register the actual code block
  CodeMap::Entry DataEntry {SectionInfo.FileInfo.FileId, static_cast<uint32_t>(BlockEntry)};
  AppendData(std::as_bytes(std::span {&DataEntry, 1}));
}

void CodeMapWriter::AppendLibraryLoad(const FEXCore::ExecutableFileInfo& FileInfo) {
  // See CodeMap::ExternalLibraryInfo
  auto ExternalFileId = FileInfo.FileId;
  auto TotalSize = AlignUp(sizeof(CodeMap::LoadExternalLibrary) + sizeof(ExternalFileId) + FileInfo.Filename.size() + 1, 4);
  const auto Data = reinterpret_cast<char*>(alloca(TotalSize));
  auto WritePtr = std::copy_n(reinterpret_cast<const char*>(&CodeMap::LoadExternalLibrary), sizeof(CodeMap::LoadExternalLibrary), Data);
  WritePtr = std::copy_n(reinterpret_cast<const char*>(&ExternalFileId), sizeof(ExternalFileId), WritePtr);
  WritePtr = std::copy(FileInfo.Filename.begin(), FileInfo.Filename.end(), WritePtr);
  std::fill(WritePtr, Data + TotalSize, 0);
  AppendData(std::as_bytes(std::span {Data, TotalSize}));
}

void CodeMapWriter::AppendSetMainExecutable(const FEXCore::ExecutableFileInfo& FileInfo) {
  CodeMap::SetExecutableFileId Data {.ExecutableFileId = FileInfo.FileId};
  AppendData(std::span {reinterpret_cast<const std::byte*>(&Data), sizeof(Data)});
}

void CodeMapWriter::AppendData(std::span<const std::byte> Data) {
  std::shared_lock Lock {Mutex};
  auto Offset = BufferOffset.fetch_add(Data.size_bytes());
  if (Offset + Data.size_bytes() > Buffer.size()) {
    // Acquire exclusive lock and flush the buffer.
    // Under heavy pressure, multiple threads may observe an exhausted buffer simultaneously.
    // The thread with the last in-bounds Offset is responsible for flushing the buffer.
    Lock.unlock();
    bool IsResponsibleForFlush = false;
    {
      std::unique_lock ExclusiveLock {Mutex};
      IsResponsibleForFlush = (Offset <= Buffer.size());
      if (IsResponsibleForFlush) {
        Flush(Offset, ExclusiveLock);
      }
    }
    if (!IsResponsibleForFlush) {
      // Wait for the buffer to be flushed on the responsible thread
      Utils::SpinWaitLock::WaitPred<std::less_equal<>, size_t>(reinterpret_cast<size_t*>(&BufferOffset), Buffer.size());
    }
    AppendData(Data);
    return;
  }

  memcpy(&Buffer.at(Offset), Data.data(), Data.size_bytes());
}

} // namespace FEXCore

namespace FEXCore::Context {

CodeCache::CodeCache(ContextImpl& CTX_)
  : CTX(CTX_) {}
CodeCache::~CodeCache() = default;

uint64_t CodeCache::ComputeCodeMapId(std::string_view Filename, int FD) {
  if (Filename.empty()) {
    return 0xffff'ffff'ffff'ffff;
  }

  // For now, we just use the file path as an identifier.
  // TODO: Ensure the hash is unique enough to distinguish executables while remaining independent of the installation location
  return XXH3_64bits(Filename.data(), Filename.size());
}

struct CodeCacheHeader {
  char Magic[4] = {'F', 'X', 'C', 'C'};
  uint32_t FormatVersion = 1;
  char FEXVersion[8] = {};
  uint32_t NumBlocks;
  uint32_t NumCodePages;
  uint32_t CodeBufferSize;
  uint32_t NumRelocations;
  uint64_t SerializedBaseAddress;
  // TODO: Consider including information from LookupCache.BlockLinks
};

void CodeCache::LoadData(Core::InternalThreadState& Thread, std::byte* MappedCacheFile, const ExecutableFileSectionInfo& GuestRIPLookup) {
  // TODO
}

template<typename T>
concept OrderedContainer = requires { typename T::key_compare; };

bool CodeCache::SaveData(Core::InternalThreadState& Thread, int fd, const ExecutableFileSectionInfo& SourceBinary, uint64_t SerializedBaseAddress) {
  auto CodeBuffer = CTX.GetLatest();
  auto& LookupCache = *Thread.LookupCache->Shared;

  auto Relocations = Thread.CPUBackend->TakeRelocations(SourceBinary.FileStartVA);

  // Write file header
  CodeCacheHeader header;
  memcpy(&header.FEXVersion[0], GIT_SHORT_HASH, strlen(GIT_SHORT_HASH));
  header.NumBlocks = LookupCache.BlockList.size();
  header.NumCodePages = LookupCache.CodePages.size();
  header.CodeBufferSize = CTX.LatestOffset;
  header.NumRelocations = Relocations.size();
  header.SerializedBaseAddress = SerializedBaseAddress;
  ::write(fd, &header, sizeof(header));

  // Dump guest<->host block mappings
  {
    // Cache contents must be deterministic, so copy the unordered block list and then sort by key
    static_assert(!OrderedContainer<decltype(LookupCache.BlockList)>, "Already deterministic; drop temporary container");
    fextl::vector<std::pair<uint64_t, const GuestToHostMap::BlockEntry*>> BlockList;
    BlockList.reserve(LookupCache.BlockList.size());
    for (auto& [Guest, BlockEntry] : LookupCache.BlockList) {
      static_assert(sizeof(Guest) == 8, "Breaking change in code cache data layout");
      BlockList.emplace_back(Guest, &BlockEntry);
    }
    std::ranges::sort(BlockList);

    for (auto [Guest, Host] : BlockList) {
      static_assert(sizeof(Host->HostCode) == 8, "Breaking change in code cache data layout");
      static_assert(sizeof(Host->CodePages[0]) == 8, "Breaking change in code cache data layout");

      Guest -= SourceBinary.FileStartVA;
      ::write(fd, &Guest, sizeof(Guest));
      uint64_t HostCode = Host->HostCode - reinterpret_cast<uintptr_t>(CodeBuffer->Ptr);
      ::write(fd, &HostCode, sizeof(HostCode));
      uint64_t NumCodePages = Host->CodePages.size();
      ::write(fd, &NumCodePages, sizeof(NumCodePages));
      LOGMAN_THROW_A_FMT(std::ranges::is_sorted(Host->CodePages), "Code pages aren't sorted");
      for (auto CodePage : Host->CodePages) {
        CodePage -= SourceBinary.FileStartVA;
        ::write(fd, &CodePage, sizeof(CodePage));
      }
    }
  }

  // Dump relocations
  static_assert(sizeof(Relocations[0]) == 48, "Breaking change in code cache data layout");
  ::write(fd, Relocations.data(), Relocations.size() * sizeof(Relocations[0]));

  // Pad to next page in file so that the CodeBuffer can be mmap'ed into process on load
  char Zero[64] {};
  auto Off = lseek(fd, 0, SEEK_CUR);
  while (Off != AlignUp(Off, Utils::FEX_PAGE_SIZE)) {
    auto BytesToWrite = std::min(AlignUp(Off, Utils::FEX_PAGE_SIZE) - Off, sizeof(Zero));
    ::write(fd, Zero, BytesToWrite);
    Off += BytesToWrite;
  }

  // Dump the host code (relocated for position-independent serialization)
  std::vector CodeBufferData(reinterpret_cast<std::byte*>(CodeBuffer->Ptr), reinterpret_cast<std::byte*>(CodeBuffer->Ptr) + CTX.LatestOffset);
  if (!ApplyCodeRelocations(SerializedBaseAddress, CodeBufferData, Relocations, true)) {
    LOGMAN_THROW_A_FMT(false, "Failed to apply code relocations");
    return false;
  }
  ::write(fd, CodeBufferData.data(), CodeBufferData.size());

  // Dump code pages
  static_assert(OrderedContainer<decltype(LookupCache.CodePages)>, "Non-deterministic data source");
  for (auto& [Page, Entrypoints] : LookupCache.CodePages) {
    static_assert(sizeof(Page) == 8, "Breaking change in code cache data layout");
    ::write(fd, &Page, sizeof(Page));
    uint64_t NumEntrypoints = Entrypoints.size();
    ::write(fd, &NumEntrypoints, sizeof(NumEntrypoints));
    ::write(fd, Entrypoints.data(), Entrypoints.size() * sizeof(Entrypoints[0]));
  }

  return true;
}

bool CodeCache::ApplyCodeRelocations(uint64_t GuestEntry, std::span<std::byte> Code,
                                     std::span<const FEXCore::CPU::Relocation> EntryRelocations, bool ForStorage) {
  CPU::Arm64Emitter Emitter(&CTX, Code.data(), Code.size_bytes());
  for (size_t j = 0; j < EntryRelocations.size(); ++j) {
    const FEXCore::CPU::Relocation& Reloc = EntryRelocations[j];
    Emitter.SetCursorOffset(Reloc.Header.Offset);

    switch (Reloc.Header.Type) {
    case FEXCore::CPU::RelocationTypes::RELOC_NAMED_SYMBOL_LITERAL: {
      // Generate a literal so we can place it
      uint64_t Pointer = ForStorage ? 0 : GetNamedSymbolLiteral(CTX, Reloc.NamedSymbolLiteral.Symbol);
      Emitter.dc64(Pointer);
      break;
    }
    case FEXCore::CPU::RelocationTypes::RELOC_NAMED_THUNK_MOVE: {
      uint64_t Pointer = ForStorage ? 0 : reinterpret_cast<uint64_t>(CTX.ThunkHandler->LookupThunk(Reloc.NamedThunkMove.Symbol));
      if (Pointer == ~0ULL) {
        return false;
      }

      Emitter.LoadConstant(ARMEmitter::Size::i64Bit, ARMEmitter::Register(Reloc.NamedThunkMove.RegisterIndex), Pointer, true);
      break;
    }
    case FEXCore::CPU::RelocationTypes::RELOC_GUEST_RIP_LITERAL: {
      Emitter.dc64(GuestEntry + Reloc.GuestRIP.GuestRIP);
      break;
    }
    case FEXCore::CPU::RelocationTypes::RELOC_GUEST_RIP_MOVE: {
      uint64_t Pointer = Reloc.GuestRIP.GuestRIP + GuestEntry;
      Emitter.LoadConstant(ARMEmitter::Size::i64Bit, ARMEmitter::Register(Reloc.GuestRIP.RegisterIndex), Pointer, true);
      break;
    }

    default: ERROR_AND_DIE_FMT("Unknown relocation type {}", ToUnderlying(Reloc.Header.Type));
    }
  }

  return true;
}

} // namespace FEXCore::Context
