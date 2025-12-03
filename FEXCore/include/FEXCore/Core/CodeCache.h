// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/fextl/functional.h>
#include <FEXCore/fextl/map.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/set.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>

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

// Generic information associated with an executable file.
struct ExecutableFileInfo {
  ~ExecutableFileInfo();

  fextl::unique_ptr<HLE::SourcecodeMap> SourcecodeMap;
  uint64_t FileId = 0;
  fextl::string Filename;
};

// Information associated with a specific section of an executable file
struct ExecutableFileSectionInfo {
  ExecutableFileInfo& FileInfo;

  // Start address that the file is mapped to.
  uintptr_t FileStartVA;
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

class AbstractCodeCache {
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
   * Loads a code cache from mapped memory and appends it to the current Core state.
   * TODO: Optionally recompiles all contained code blocks at runtime for validation.
   */
  virtual void LoadData(Core::InternalThreadState&, std::byte* MappedCacheFile, const ExecutableFileSectionInfo&) = 0;

  /**
   * Bundles the current Core state (CodeBuffer, GuestToHostMapping, ...) to a code cache and writes it to the given file descriptor.
   * Returns true on success.
   */
  virtual bool SaveData(Core::InternalThreadState&, int TargetFD, const ExecutableFileSectionInfo&, uint64_t SerializedBaseAddress) = 0;

  /**
   * Function to be called before compiling any code for caching purposes
   */
  virtual void InitiateCacheGeneration() = 0;
};

} // namespace FEXCore
