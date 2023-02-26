#pragma once
#include "ELFMapping.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace FileMapping {
  struct MemMapping;

  struct FileMapping {
    uint64_t Begin, End;
    std::string Path;
    std::vector<MemMapping*> MemMappings;
    std::optional<ELFMapping::ELFMemMapping*> ELFMapping;
  };

  struct MemMapping {
    uint64_t Begin, End;
    uint32_t permissions;
    FileMapping *FileMapping;
    std::string Path;
  };
}
