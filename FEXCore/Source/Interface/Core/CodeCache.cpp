// SPDX-License-Identifier: MIT
#include "Utils/SpinWaitLock.h"

#include <Interface/Context/Context.h>
#include <Interface/Core/ArchHelpers/Arm64Emitter.h>
#include <Interface/Core/Dispatcher/Dispatcher.h>
#include <Interface/Core/JIT/DebugData.h>
#include <Interface/Core/JIT/Relocations.h>
#include <Interface/Core/LookupCache.h>
#include <Interface/Core/OpcodeDispatcher.h>
#include <Interface/IR/PassManager.h>

#include <FEXCore/Core/Thunks.h>
#include <FEXCore/HLE/SourcecodeResolver.h>
#include <FEXCore/HLE/SyscallHandler.h>

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
  std::array<char, 4> Magic = ExpectedMagic;
  uint32_t FormatVersion = 1;
  char FEXVersion[8] = {};
  uint32_t NumBlocks;
  uint32_t NumCodePages;
  uint32_t CodeBufferSize;
  uint32_t NumRelocations;
  uint64_t SerializedBaseAddress;
  // TODO: Consider including information from LookupCache.BlockLinks

  static constexpr std::array<char, 4> ExpectedMagic = {'F', 'X', 'C', 'C'};
};

template<typename T>
concept OrderedContainer = requires { typename T::key_compare; };

bool CodeCache::SaveData(Core::InternalThreadState& Thread, int fd, const ExecutableFileSectionInfo& SourceBinary, uint64_t SerializedBaseAddress) {
  auto CodeBuffer = CTX.GetLatest();
  auto& LookupCache = *Thread.LookupCache->Shared;
  auto Relocations = Thread.CPUBackend->TakeRelocations(SourceBinary.FileStartVA);

  // Write file header
  CodeCacheHeader header {};
  constexpr std::string_view git_hash = GIT_SHORT_HASH;
  static_assert(git_hash.size() <= sizeof(header.FEXVersion));
  std::ranges::copy(git_hash, header.FEXVersion);
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
  for (auto& [PageIndex, Entrypoints] : LookupCache.CodePages) {
    uint64_t PageAddr = PageIndex << 12 - SourceBinary.FileStartVA;
    ::write(fd, &PageAddr, sizeof(PageAddr));
    uint64_t NumEntrypoints = Entrypoints.size();
    ::write(fd, &NumEntrypoints, sizeof(NumEntrypoints));
    for (uint64_t Entrypoint : Entrypoints) {
      Entrypoint -= SourceBinary.FileStartVA;
      ::write(fd, &Entrypoint, sizeof(Entrypoint));
    }
  }

  return true;
}

bool CodeCache::LoadData(Core::InternalThreadState* Thread, std::byte* MappedCacheFile, const ExecutableFileSectionInfo& BinarySection) {
  if (!EnableCodeCaching) {
    return true;
  }

  namespace ranges = std::ranges;

  // Read file header
  CodeCacheHeader header {};
  ::memcpy(&header, MappedCacheFile, sizeof(header));
  MappedCacheFile += sizeof(header);

  LogMan::Msg::IFmt("Cache load: {:5} blocks; base={:#14x}; off={:#9x}-{:#09x}; {:016x} {}", header.NumBlocks, BinarySection.FileStartVA,
                    BinarySection.BeginVA - BinarySection.FileStartVA, BinarySection.EndVA - BinarySection.FileStartVA,
                    BinarySection.FileInfo.FileId, BinarySection.FileInfo.Filename);

  if (!ranges::equal(header.Magic, header.ExpectedMagic)) {
    LogMan::Msg::EFmt("Invalid cache file header");
    return false;
  }

  char ExpectedVersion[8] = GIT_SHORT_HASH;
  ranges::fill(ranges::find(ExpectedVersion, 0), std::end(ExpectedVersion), 0);
  if (!ranges::equal(header.FEXVersion, ExpectedVersion)) {
    LogMan::Msg::IFmt("Cache generated from old FEX version {}, current is {}; skipping", fmt::join(header.FEXVersion, ""),
                      fmt::join(ExpectedVersion, ""));
    return false;
  }

  if (header.NumBlocks == 0) {
    // Valid caches are never empty
    LogMan::Msg::IFmt("Code cache empty, aborting");
    return false;
  }

  // Read guest<->host block mappings
  using BlockListEntry = decltype(GuestToHostMap::BlockList)::value_type;
  fextl::vector<BlockListEntry> BlockList(header.NumBlocks);
  {
    for (auto& BlockPtr : BlockList) {
      ::memcpy(&BlockPtr.first, MappedCacheFile, sizeof(BlockPtr.first));
      MappedCacheFile += sizeof(BlockPtr.first);
      ::memcpy(&BlockPtr.second.HostCode, MappedCacheFile, sizeof(BlockPtr.second.HostCode));
      MappedCacheFile += sizeof(BlockPtr.second.HostCode);
      uint64_t NumGuestPages;
      ::memcpy(&NumGuestPages, MappedCacheFile, sizeof(NumGuestPages));
      MappedCacheFile += sizeof(NumGuestPages);

      BlockPtr.second.CodePages.resize(NumGuestPages);
      ::memcpy(BlockPtr.second.CodePages.data(), MappedCacheFile, std::span {BlockPtr.second.CodePages}.size_bytes());
      MappedCacheFile += std::span {BlockPtr.second.CodePages}.size_bytes();
    }

    // Consistency check: VMA regions at the top and end should belong to the same file
    auto [min_val, max_val] = ranges::minmax_element(BlockList, std::less {}, &decltype(BlockList)::value_type::first);
    auto MinBound = CTX.SyscallHandler->LookupExecutableFileSection(Thread, min_val->first + BinarySection.FileStartVA);
    auto MaxBound = CTX.SyscallHandler->LookupExecutableFileSection(Thread, max_val->first + BinarySection.FileStartVA);
    if (&MinBound->FileInfo != &BinarySection.FileInfo || &MaxBound->FileInfo != &BinarySection.FileInfo) {
      ERROR_AND_DIE_FMT("Cached blocks offsets {:#x}-{:#x} out of bounds for guest library {} ({:016x} @ {:#x}) while trying to load "
                        "section {:#x}-{:#x}!",
                        min_val->first, max_val->first, BinarySection.FileInfo.Filename, BinarySection.FileInfo.FileId,
                        BinarySection.FileStartVA, BinarySection.BeginVA, BinarySection.EndVA);
    }

    // Constrain BlockList to the given ExecutableFileSectionInfo
    LOGMAN_THROW_A_FMT(ranges::is_sorted(BlockList, [](auto& a, auto& b) { return a.first < b.first; }), "Expected sorted block list");
    auto begin = ranges::lower_bound(BlockList, BinarySection.BeginVA - BinarySection.FileStartVA, std::less {}, &BlockListEntry::first);
    auto end =
      ranges::upper_bound(begin, BlockList.end(), BinarySection.EndVA - BinarySection.FileStartVA - 1, std::less {}, &BlockListEntry::first);
    BlockList.erase(end, BlockList.end());
    BlockList.erase(BlockList.begin(), begin);
    if (BlockList.empty()) {
      // Not an error since there is just no data to load
      LogMan::Msg::IFmt("No blocks cached in this range, aborting");
      return true;
    }
  }

  // Read relocations
  fextl::vector<FEXCore::CPU::Relocation> Relocations(header.NumRelocations, FEXCore::CPU::Relocation::Default());
  ::memcpy(Relocations.data(), MappedCacheFile, Relocations.size() * sizeof(Relocations[0]));
  MappedCacheFile += Relocations.size() * sizeof(Relocations[0]);

  // Pad to next page in file, which contains CodeBuffer data
  MappedCacheFile = reinterpret_cast<std::byte*>(AlignUp(reinterpret_cast<uintptr_t>(MappedCacheFile), Utils::FEX_PAGE_SIZE));

  // Prepare CodeBuffer: Page aligned and big enough to hold all cached data
  auto Lock = std::unique_lock {CTX.CodeBufferWriteMutex};
  if (Thread) {
    if (auto Prev = Thread->CPUBackend->CheckCodeBufferUpdate()) {
      Allocator::VirtualDontNeed(Thread->CallRetStackBase, FEXCore::Core::InternalThreadState::CALLRET_STACK_SIZE);
      auto lk = Thread->LookupCache->AcquireWriteLock();
      Thread->LookupCache->ChangeGuestToHostMapping(*Prev, *CTX.GetLatest()->LookupCache, lk);
    }
  }

  auto CodeBuffer = CTX.GetLatest();
  LOGMAN_THROW_A_FMT(header.CodeBufferSize <= CodeBuffer->Size, "CodeBuffer too small to load code cache");
  LOGMAN_THROW_A_FMT(reinterpret_cast<uintptr_t>(CodeBuffer->Ptr) % 0x1000 == 0, "Expected CodeBuffer base to be page-aligned");
  const auto Delta = AlignUp(CTX.LatestOffset, 0x1000) - CTX.LatestOffset;
  CTX.LatestOffset += Delta;

  while (CTX.LatestOffset + header.CodeBufferSize > CodeBuffer->Size - Utils::FEX_PAGE_SIZE) {
    if (Thread) {
      CTX.ClearCodeCache(Thread);
      CodeBuffer = CTX.GetLatest();
      LogMan::Msg::IFmt("Increased code buffer size to {} MiB for cache load", CodeBuffer->Size / 1024 / 1024);
    } else {
      ERROR_AND_DIE_FMT("Cannot extend codebuffer without thread!");
    }
  }

  // Read CodeBuffer data from file. Make sure the destination is page-aligned.
  // TODO: Only load the data needed for the selected section
  auto CodeBufferRange = std::as_writable_bytes(std::span {CodeBuffer->Ptr, CodeBuffer->Size}).subspan(CTX.LatestOffset, header.CodeBufferSize);
  ::memcpy(CodeBufferRange.data(), MappedCacheFile, header.CodeBufferSize);
  MappedCacheFile += header.CodeBufferSize;
  CTX.LatestOffset += header.CodeBufferSize;

  // Apply FEX relocations
  auto Ret = ApplyCodeRelocations(BinarySection.FileStartVA, CodeBufferRange, Relocations, false);
  LOGMAN_THROW_A_FMT(Ret == true, "Failed to apply code cache relocations");

  {
    auto& LookupCache = *CodeBuffer->LookupCache;
    auto WriteLock = LookupCache.AcquireWriteLock();

    // Register blocks to LookupCache
    for (auto& [Guest, Host] : BlockList) {
      for (auto& CodePage : Host.CodePages) {
        CodePage += BinarySection.FileStartVA;
      }
      auto HostCode = reinterpret_cast<void*>(Host.HostCode + reinterpret_cast<uintptr_t>(CodeBufferRange.data()));
      LookupCache.AddBlockMapping(Guest + BinarySection.FileStartVA, std::move(Host.CodePages), HostCode, WriteLock);
    }

    // Register loaded code ranges
    fextl::vector<uint64_t> Entrypoints;
    for (uint32_t i = 0; i < header.NumCodePages; ++i) {
      uint64_t CodePage;
      memcpy(&CodePage, MappedCacheFile, sizeof(CodePage));
      CodePage += BinarySection.FileStartVA;
      MappedCacheFile += sizeof(CodePage);

      uint64_t NumEntrypoints;
      memcpy(&NumEntrypoints, MappedCacheFile, sizeof(NumEntrypoints));
      MappedCacheFile += sizeof(NumEntrypoints);

      Entrypoints.resize(NumEntrypoints);
      memcpy(Entrypoints.data(), MappedCacheFile, NumEntrypoints * sizeof(Entrypoints[0]));
      MappedCacheFile += NumEntrypoints * sizeof(Entrypoints[0]);
      for (auto& Entrypoint : Entrypoints) {
        Entrypoint += BinarySection.FileStartVA;
      }

      if (LookupCache.AddBlockExecutableRange(Entrypoints, CodePage, FEXCore::Utils::FEX_PAGE_SIZE, WriteLock)) {
        CTX.SyscallHandler->MarkGuestExecutableRange(Thread, CodePage, FEXCore::Utils::FEX_PAGE_SIZE);
      }
    }
  }

  if (EnableCodeCacheValidation) {
    fextl::set<uint64_t> GuestBlocks, HostBlocks;
    for (auto& [Guest, Host] : BlockList) {
      GuestBlocks.insert(Guest + BinarySection.FileStartVA);
      HostBlocks.insert(Host.HostCode);
    }

    Validate(BinarySection, std::move(GuestBlocks), HostBlocks, CodeBufferRange);
  }

  return true;
}

void CodeCache::Validate(const ExecutableFileSectionInfo& Section, fextl::set<uint64_t> GuestBlocks, const fextl::set<uint64_t>& HostBlocks,
                         std::span<std::byte> CachedCode) {
  LOGMAN_THROW_A_FMT(!HostBlocks.empty(), "Tried to validate without any host blocks");
  // Skip any cached data before the first host block
  CachedCode = CachedCode.subspan(*HostBlocks.begin() - sizeof(CPU::CPUBackend::JITCodeHeader));

  if (!ValidationCTX) {
    ValidationCTX.reset(static_cast<ContextImpl*>(FEXCore::Context::Context::CreateNewContext(CTX.HostFeatures).release()));
    ValidationCTX->SetSignalDelegator(CTX.SignalDelegation);
    ValidationCTX->SetSyscallHandler(CTX.SyscallHandler);
    ValidationCTX->SetThunkHandler(CTX.ThunkHandler);
    if (!ValidationCTX->InitCore()) {
      ERROR_AND_DIE_FMT("Failed to create cache load validation context");
    }

    ValidationThread.reset(ValidationCTX->CreateThread(0, 0, nullptr));

    auto Frame = ValidationThread->CurrentFrame;
    Frame->State.segment_arrays[FEXCore::Core::CPUState::SEGMENT_ARRAY_INDEX_GDT] = &ValidationGDT[0];
    Frame->State.segment_arrays[FEXCore::Core::CPUState::SEGMENT_ARRAY_INDEX_LDT] = &ValidationGDT[0];
    Frame->State.cs_idx = 0;
    Frame->State.cs_cached = 0;

    if (ValidationCTX->Config.Is64BitMode()) {
      ValidationGDT[0].L = 1; // L = Long Mode = 64-bit
      ValidationGDT[0].D = 0; // D = Default Operand Size = Reserved
    } else {
      ValidationGDT[0].L = 0; // L = Long Mode = 32-bit
      ValidationGDT[0].D = 1; // D = Default Operand Size = 32-bit
    }
  }

  auto NewCodeBuffer = ValidationCTX->GetLatest();

  std::span<std::byte> CodeBufferRangeRef =
    std::as_writable_bytes(std::span {NewCodeBuffer->Ptr, NewCodeBuffer->Ptr + NewCodeBuffer->Size}).subspan(0, CachedCode.size_bytes());

  while (!GuestBlocks.empty()) {
    auto [CompiledBlocks, _, _2, _3, _4] = ValidationCTX->CompileCode(ValidationThread.get(), *GuestBlocks.begin(), 0 /* TODO: Set MaxInst? */);
    for (auto& Entry : CompiledBlocks.EntryPoints) {
      GuestBlocks.erase(Entry.first);
    }
  }

  // Patch FEX-internal function addresses with values from the main Context to ensure the code blocks are comparable
  auto NewRelocations = ValidationThread->CPUBackend->TakeRelocations(Section.FileStartVA);
  NewRelocations.erase(std::remove_if(NewRelocations.begin(), NewRelocations.end(), [](const CPU::Relocation& Reloc) {
    return Reloc.Header.Type != CPU::RelocationTypes::RELOC_NAMED_SYMBOL_LITERAL && Reloc.Header.Type != CPU::RelocationTypes::RELOC_NAMED_THUNK_MOVE;
  }));
  (void)ApplyCodeRelocations(Section.FileStartVA, CodeBufferRangeRef, NewRelocations, false);

  if (ValidationCTX->LatestOffset <= CodeBufferRangeRef.size()) {
    // Reference compilation produced fewer bytes than our cache, so validation is going to fail.
    // Make sure we don't output any garbage bytes though.
    CodeBufferRangeRef = CodeBufferRangeRef.subspan(0, ValidationCTX->LatestOffset);
  }

  auto [Mismatch, _] = std::mismatch(CodeBufferRangeRef.begin(), CodeBufferRangeRef.end(), CachedCode.begin());
  if (Mismatch != CodeBufferRangeRef.end()) {
    // Align down to instruction size
    auto Idx = AlignDown(std::distance(CodeBufferRangeRef.begin(), Mismatch), 4);

    auto BlockIt = std::prev(HostBlocks.lower_bound(*HostBlocks.begin() + Idx + 1));
    std::optional<uint64_t> GuestBlockAddr;
    std::optional<uint64_t> GuestBlockAddrRef;
    if (BlockIt != HostBlocks.end()) {
      for (int i : {0, 1}) {
        std::span Buffer = (i == 0 ? CachedCode : CodeBufferRangeRef);

        // Second instruction is always a constant load for relative offset to the (multi)block start
        int32_t addr = (*reinterpret_cast<uint32_t*>(&Buffer[*BlockIt - *HostBlocks.begin() + 4]) & 0x3ff'ffe0) << 11;
        addr >>= 14;
        auto header = reinterpret_cast<CPU::CPUBackend::JITCodeHeader*>(&Buffer[*BlockIt - *HostBlocks.begin() + 4 + addr]);
        auto tail = reinterpret_cast<CPU::CPUBackend::JITCodeTail*>(reinterpret_cast<uintptr_t>(header) + header->OffsetToBlockTail);
        (i == 0 ? GuestBlockAddr : GuestBlockAddrRef) = tail->RIP - Section.FileStartVA;
        LogMan::Msg::EFmt("Recorded rip {}: {:#x} (offset {:#x})", i, tail->RIP, tail->RIP - Section.FileStartVA);

        if (i == 1) {
          if (tail->RIP >= Section.BeginVA && tail->RIP < Section.EndVA) {
            auto [IRView, TotalInstructions, TotalInstructionsLength, StartAddr, Length, _] =
              ValidationCTX->GenerateIR(ValidationThread.get(), tail->RIP, false, FEXCore::Config::Get_MAXINST());
            fextl::stringstream ss;
            FEXCore::IR::Dump(&ss, &*IRView);
            LogMan::Msg::EFmt("IR:\n{}", ss.str());
          } else {
            LogMan::Msg::EFmt("Can't dump IR for out-of-range RIP {:#x}", tail->RIP);
          }
        }
      }
    }

    fextl::string GuestBlockInfo = "UNKNOWN";
    if (GuestBlockAddr) {
      GuestBlockInfo = fextl::fmt::format("{:#x}", GuestBlockAddr.value());
    }
    if (GuestBlockAddr != GuestBlockAddrRef) {
      GuestBlockInfo += " (MISMATCH)";
    }
    ERROR_AND_DIE_FMT("Cache validation failed at offset {:#x}: {:02x} <-> {:02x} (at {} <-> {}, guest block {})", Idx,
                      fmt::join(CachedCode.subspan(Idx, 4), ""), fmt::join(CodeBufferRangeRef.subspan(Idx, 4), ""),
                      fmt::ptr(CachedCode.data()), fmt::ptr(CodeBufferRangeRef.data()), GuestBlockInfo);
  }

  // Reset Context state for next validation
  ValidationThread->LookupCache->ClearCache(ValidationThread->LookupCache->AcquireWriteLock());
  ValidationCTX->LatestOffset = 0;

  LogMan::Msg::IFmt("\tSuccessfully validated cache");
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
      // Pointers are required to fit within 48-bit VA space.
      Emitter.LoadConstant(ARMEmitter::Size::i64Bit, ARMEmitter::Register(Reloc.NamedThunkMove.RegisterIndex), Pointer,
                           CPU::Arm64Emitter::PadType::DOPAD, 6);
      break;
    }
    case FEXCore::CPU::RelocationTypes::RELOC_GUEST_RIP_LITERAL: {
      Emitter.dc64(GuestEntry + Reloc.GuestRIP.GuestRIP);
      break;
    }
    case FEXCore::CPU::RelocationTypes::RELOC_GUEST_RIP_MOVE: {
      uint64_t Pointer = Reloc.GuestRIP.GuestRIP + GuestEntry;
      // Pointers are required to fit within 48-bit VA space.
      Emitter.LoadConstant(ARMEmitter::Size::i64Bit, ARMEmitter::Register(Reloc.GuestRIP.RegisterIndex), Pointer,
                           CPU::Arm64Emitter::PadType::DOPAD, 6);
      break;
    }

    default: ERROR_AND_DIE_FMT("Unknown relocation type {}", ToUnderlying(Reloc.Header.Type));
    }
  }

  return true;
}

} // namespace FEXCore::Context
