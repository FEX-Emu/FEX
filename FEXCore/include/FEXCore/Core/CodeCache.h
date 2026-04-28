// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/fextl/functional.h>
#include <FEXCore/fextl/map.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/set.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>
#include <FEXCore/fextl/robin_map.h>
#include <FEXCore/Utils/TypeDefines.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <span>
#include <unistd.h>

namespace FEXCore {

namespace Core {
  struct InternalThreadState;
} // namespace Core

namespace HLE {
  struct SourcecodeMap;
} // namespace HLE

enum class GuestRelocationType : uint32_t {
  Rel32,
  Rel64,
  // Skip blocks containing this relocation
  Skip,
};

// Generic information associated with an executable file.
struct ExecutableFileInfo {
  ~ExecutableFileInfo();

#if __clang_major__ < 16
  // Workaround for broken aggregate-initialization with std::piecewise_construct
  ExecutableFileInfo(fextl::unique_ptr<HLE::SourcecodeMap>, uint64_t, fextl::string);
  ExecutableFileInfo() = default;
#endif

  // This legacy field must be assignable through const-references
  mutable fextl::unique_ptr<HLE::SourcecodeMap> SourcecodeMap;

  uint64_t FileId = 0;
  fextl::string Filename;
  fextl::robin_map<uint32_t, GuestRelocationType> Relocations;
};

// Information associated with a specific section of an executable file
struct ExecutableFileSectionInfo {
  const ExecutableFileInfo& FileInfo;

  // Start address that the file is mapped to.
  // NOTE: Since executable files may be mapped multiple times, this can depend on the queried section.
  uintptr_t FileStartVA;

  // Start address of the section mapping
  uintptr_t BeginVA;

  // End address that of the section mapping
  uintptr_t EndVA;
};

using CodeMapFileId = uint64_t;

/**
 * Code maps capture information required for offline code cache generation
 * and are written to disk during execution of FEX.
 *
 * Almost all CodeMap data will be an Entry that indicates blocks to be
 * compiled for cache generation. The reserved value `LoadExternalLibrary`
 * indicates that an instance of ExternalLibraryInfo follows (the entry data
 * itself should be skipped in that case).
 */
struct CodeMap {
  // Describes the location of an entry block compiled during execution
  struct FEX_PACKED Entry {
    CodeMapFileId FileId;
    uint32_t BlockOffset;
  };

  // Describes an external library referenced during execution
  struct ExternalLibraryInfo {
    CodeMapFileId ExternalFileId;

    // null-terminated file path; EITHER relative to the main executable OR an absolute path OR starting with a magic identifier:
    // - WINE/: Path to Wine/Proton installation
    // - WINEPREFIX/: Path to Wine/Proton prefix
    // - SLR/: Path to Steam Linux Runtime
    // At runtime, FEX will always dump absolute paths
    char Path[];
    // Followed by padding to a 4 byte boundary
  };

  // Followed by ExternalLibraryInfo
  static constexpr Entry LoadExternalLibrary = {0xffff'ffff'ffff'ffff, 0xffff'ffff};

  struct FEX_PACKED SetExecutableFileId {
    Entry Marker = {0xffff'ffff'ffff'ffff, 0xffff'fffe};
    CodeMapFileId ExecutableFileId;
  };

  struct ParsedContents {
    fextl::string Filename;
    fextl::set<uint64_t> Blocks;
    bool IsExecutable = false;
  };

  // Follows scheme fileid[-nomb]
  // The nomb ("no multiblock") suffix signifies that the code map is for use without multiblock, only.
  static fextl::string GetBaseFilename(const ExecutableFileInfo& MainExecutable, bool AddNombSuffix);

  static fextl::map<CodeMapFileId, ParsedContents> ParseCodeMap(std::ifstream& File);
};

struct CodeMapOpener {
  virtual ~CodeMapOpener() = default;
  virtual int OpenCodeMapFile() = 0;
};

class CodeMapWriter {
public:
  CodeMapWriter(CodeMapOpener&, bool OpenEagerly = false);
  ~CodeMapWriter();

  // Checks if writing is enabled. Calls to this functions may also be interpreted as signals that writes are about to happen
  bool IsWriteEnabled(const ExecutableFileSectionInfo&);

  void ResetAfterFork() {
    if (CodeMapFD.value_or(-1) != -1) {
      close(CodeMapFD.value());
      CodeMapFD.reset();
    }
    BufferOffset = 0;
    KnownFileIds.clear();
  }

  bool IsBackingFD(int FD) const {
    if (FD == CodeMapFD) {
      LogMan::Msg::DFmt("Hiding directory entry for code map FD");
      return true;
    }
    return false;
  }

  void AppendBlock(const FEXCore::ExecutableFileSectionInfo&, uint64_t Entry);
  void AppendLibraryLoad(const FEXCore::ExecutableFileInfo&);
  void AppendSetMainExecutable(const FEXCore::ExecutableFileInfo&);

  // Thread-safely commit any pending data to disk
  void Flush(size_t Offset);

private:
  // Queues data into an internal ring buffer.
  // Call Flush() to commit the data to disk.
  void AppendData(std::span<const std::byte> Data);

  // Commit given data range to disk
  void Flush(size_t Offset, std::unique_lock<std::shared_mutex>&);

  std::shared_mutex Mutex;
  fextl::vector<std::byte> Buffer;
  std::atomic<size_t> BufferOffset {0};

  fextl::set<CodeMapFileId> KnownFileIds;

  // std::nullopt: We haven't requested a CodeMapFD yet
  // value is -1:  We requested a CodeMapFD but FEXServer told us not to write any data
  // other values: Code map writing is active
  std::optional<int> CodeMapFD;

  CodeMapOpener& FileOpener;
};

class AbstractCodeCache;

/**
 * Manages runtime state associated with a mapped code cache file.
 *
 * The mapped file pointer is managed by the frontend and must be valid
 * throughout the lifetime of this object.
 */
struct MappedCodeCacheFile {
  // Calls UnregisterMappedCodeBuffer internally, see its docstring about synchronization requirements
  ~MappedCodeCacheFile();

  // If not nullptr, the MappedCodeCacheFile will be unregistered from this on destruction
  AbstractCodeCache* CacheManager;

  std::span<std::byte> MappedFile; // Mapped data of the whole cache file
  std::span<std::byte> CodeBuffer; // Subspan of cached ARM64 data within MappedFile
  std::byte* BlockListInFile;      // Pointer to BlockListEntry data within MappedFile
  uint32_t NumBlocks;              // Number of BlockListEntry objects
  uint32_t NumCodePages;           // Number of code page entrypoint mappings

  struct PageRelocationRange {
    uint32_t Offset; // In bytes from start of file
    uint32_t Length; // Number of relocations
  };

  // List of relocation ranges in the mapped cache file, grouped by the code page they apply to.
  // This vector is indexed by the relative page offset from the start of the ARM64 code data.
  //
  // For example PageRelocationRanges[1] == { 0x100, 0x20 } means:
  // - there are 0x20 bytes of relocation data at offset 0x100 in the cache file
  // - these 0x20 bytes of relocation data will patch data at CodeBuffer[0x1000..0x2000]
  fextl::vector<PageRelocationRange> PageRelocationRanges;
  fextl::vector<bool> LoadedPages;

  uint64_t GuestBase {}; // Guest base address for relocation application

  // Helper member to prevent moving/copying without disallowing aggregate-construction
  std::atomic<int> disallow_copy_or_move;

  size_t NumPages() const {
    return CodeBuffer.size_bytes() / FEXCore::Utils::FEX_PAGE_SIZE;
  }
};

class AbstractCodeCache {
  fextl::vector<std::span<std::byte>> MappedCodeBuffers;

public:
  virtual ~AbstractCodeCache() = default;

  /**
   * Computes a unique identifier for the referenced binary file to be used for
   * generating the code map.
   * This identifier is independent of FEX build/runtime configuration and
   * stable across FEX updates.
   */
  virtual uint64_t ComputeCodeMapId(std::string_view Filename, int FD) = 0;

  /**
   * Bundles the current Core state (CodeBuffer, GuestToHostMapping, ...) to a code cache and writes it to the given file descriptor.
   * Returns true on success.
   */
  virtual bool SaveData(Core::InternalThreadState&, int TargetFD, const ExecutableFileSectionInfo&, uint64_t SerializedBaseAddress) = 0;

  /**
   * Function to be called before compiling any code for caching purposes
   */
  virtual void InitiateCacheGeneration() = 0;

  /**
   * Loads a code cache from mapped memory.
   *
   * Code sections must be enabled in a second step (see EnableLoadedSection).
   * Afterwards, individual code pages must be finalized using FinalizeCodePages.
   *
   * On success, this returns a MappedCodeCacheFile that must be kept alive
   * as long the cache is in use.
   */
  virtual fextl::unique_ptr<MappedCodeCacheFile> LoadCache(std::span<std::byte> CacheFile, const ExecutableFileInfo&, uint64_t FileStartVA) = 0;

  /**
   * Registers cached blocks for the given file section to the LookupCache.
   *
   * Also runs extended cache validation if enabled.
   */
  virtual bool EnableLoadedSection(Core::InternalThreadState*, MappedCodeCacheFile&, const ExecutableFileSectionInfo&) = 0;

  /**
   * Extend the given code range so that it can be safely finalized.
   *
   * This is required for example to avoid dangling page-crossing FEX relocations on the edges
   *
   * StartPage and EndPage a 0-based relative page offsets into the cached code.
   */
  static std::span<std::byte> SelectCodeRangeToFinalize(MappedCodeCacheFile&, size_t StartPage, size_t EndPage);

  /**
   * Finalize code pages in the given range (see SelectCodePagesToFinalize) for execution.
   */
  virtual void FinalizeCodePages(MappedCodeCacheFile&, std::span<std::byte> CodeRange) = 0;

  void RegisterMappedCodeBuffer(MappedCodeCacheFile&);
  void UnregisterMappedCodeBuffer(MappedCodeCacheFile&);
  bool IsAddressInMappedCodeBuffer(uintptr_t Address) const;
};

} // namespace FEXCore
