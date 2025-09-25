// SPDX-License-Identifier: MIT
#include <Interface/Context/Context.h>

#include <FEXCore/HLE/SourcecodeResolver.h>

#include <FEXHeaderUtils/Filesystem.h>

#include <Common/FDUtils.h>

#include <fstream>

#include <xxhash.h>

namespace FEXCore {

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
      // TODO: Check for conflict if a previous entry already existed?
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

CodeMapWriter::CodeMapWriter(size_t BufferSize)
  : Buffer(BufferSize) {}

CodeMapWriter::~CodeMapWriter() {
  // TODO: Must flush in the implementation instead, since we can't call virtual member functions in the base destructor
}

void CodeMapWriter::Flush(size_t Offset) {
  // Acquire exclusive lock and flush circular buffer
  std::unique_lock Lock {Mutex};
  Flush(Offset, Lock);
}

void CodeMapWriter::Flush(size_t Offset, std::unique_lock<std::shared_mutex>&) {
  CommitData(std::span {Buffer}.subspan(FlushCursor, Offset - FlushCursor));
  BufferOffset = 0;
  FlushCursor = 0;
}

void CodeMapWriter::AppendBlock(const FEXCore::ExecutableFileSectionInfo& SectionInfo, uint64_t BlockEntry) {
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

void CodeMapWriter::AppendData(std::span<const std::byte> Data) {
  std::shared_lock Lock {Mutex};
  auto Offset = BufferOffset.fetch_add(Data.size_bytes());
  if (Offset + Data.size_bytes() > Buffer.size()) {
    Lock.unlock();
    Flush(Offset);
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

uint64_t CodeCache::ComputeCodeMapId(int FD) {
  char Tmp[PATH_MAX];
  auto PathLength = FEX::get_fdpath(FD, Tmp);
  auto Filename = std::string_view(Tmp, PathLength);

  if (Filename.empty()) {
    return 0xffff'ffff'ffff'ffff;
  }

  // For now, we just use the file path as an identifier.
  // TODO: Ensure the hash is unique enough to distinguish executables while remaining independent of the installation location
  return XXH3_64bits(Filename.data(), Filename.size());
}

void CodeCache::LoadData(Core::InternalThreadState& Thread, std::byte* MappedCacheFile, const ExecutableFileSectionInfo& GuestRIPLookup) {
  // TODO
}

bool CodeCache::SaveData(Core::InternalThreadState& Thread, int fd, const ExecutableFileSectionInfo& SourceBinary, uint64_t SerializedBaseAddress) {
  // TODO
  return true;
}

} // namespace FEXCore::Context
