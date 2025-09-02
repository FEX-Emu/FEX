// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/fextl/memory.h>
#include <FEXCore/fextl/string.h>

#include <cstdint>

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
  fextl::string FileId;
  fextl::string Filename;
};

// Information associated with a specific section of an executable file
struct ExecutableFileSectionInfo {
  ExecutableFileInfo& FileInfo;

  // Start address that the file is mapped to.
  uintptr_t FileStartVA;
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
  virtual fextl::string ComputeCodeMapId(std::string_view Filename) = 0;

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
