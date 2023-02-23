#pragma once
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>

#include <cstdint>

namespace ELFMapping {
  struct ELFMemMapping;
}

namespace FileMapping {
  struct MemMapping;

  struct FileMapping {
    uint64_t Begin, End;
    fextl::string Path;
    fextl::vector<MemMapping*> MemMappings;
    ELFMapping::ELFMemMapping *ELFMapping{};
  };

  struct MemMapping {
    uint64_t Begin, End;
    uint32_t permissions;
    FileMapping *FileMapping;
    fextl::string Path;
  };
}
