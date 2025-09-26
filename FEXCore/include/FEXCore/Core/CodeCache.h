// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/fextl/map.h>
#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/set.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <shared_mutex>
#include <span>

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
  uint64_t FileId;
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
 * TODO. Captures information required for offline compilation during execution.
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
    // At runtime, FEX will always dump absolute paths.
    char Path[];
    // Followed by padding to a 4 byte boundary
  };

  static constexpr Entry LoadExternalLibrary = {0xffff'ffff'ffff'ffff, 0xffff'ffff};

  struct ParsedContents {
    fextl::string Filename;
    fextl::set<uint64_t> Blocks;
  };

  // Follows scheme fileid[-nomb]
  // The nomb ("no multiblock") suffix signifies that the code map is for use without multiblock, only.
  static fextl::string GetBaseFilename(const ExecutableFileInfo& MainExecutable, bool AddNombSuffix);

  static fextl::map<CodeMapFileId, ParsedContents> ParseCodeMap(std::ifstream& File);
};

class CodeMapWriter {
public:
  CodeMapWriter();
  virtual ~CodeMapWriter() = default;

  // Checks if writing is enabled. Calls to this functions may also be interpreted as signals that writes are about to happen
  virtual bool IsWriteEnabled(const ExecutableFileSectionInfo&) = 0;

  void AppendBlock(const FEXCore::ExecutableFileSectionInfo&, uint64_t Entry);
  void AppendLibraryLoad(const FEXCore::ExecutableFileInfo&);

  // Returns a lower bound of the number of pending bytes that will be written on the next call to Flush().
  // This number isn't exact, but it can be used to decide when to trigger asynchronous flushes.
  size_t NumPendingBytes() const;

  // Thread-safely commit any pending data to disk
  void Flush(size_t Offset);

protected:
  size_t GetBufferOffset() const {
    return BufferOffset;
  }

private:
  // Queues data into an internal ring buffer.
  // Call Flush() to commit the data to disk.
  void AppendData(std::span<const std::byte> Data);

  void Flush(size_t Offset, std::unique_lock<std::shared_mutex>&);

  // Commit given data range to disk
  virtual void CommitData(std::span<const std::byte> Data) = 0;

  std::shared_mutex Mutex;
  fextl::vector<std::byte> Buffer;
  std::atomic<size_t> BufferOffset {0};

  fextl::set<CodeMapFileId> KnownFileIds;
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
  virtual uint64_t ComputeCodeMapId(int FD) = 0;

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
