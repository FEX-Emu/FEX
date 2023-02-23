#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace ELFMapping {
  struct ELFMemMapping;
}

namespace FileMapping {
  struct MemMapping;

  struct FileMapping {
    uint64_t Begin, End;
    std::string Path;
    std::vector<MemMapping*> MemMappings;
    ELFMapping::ELFMemMapping *ELFMapping{};
  };

  struct MemMapping {
    uint64_t Begin, End;
    uint32_t permissions;
    FileMapping *FileMapping;
    std::string Path;
  };
}
