#pragma once
#include <cstdint>
#include <functional>
namespace FileMapping {
  struct FileMapping;
}

namespace ELFMapping {
  struct ELFMemMapping;

  struct ELFSymbol {
    uint64_t FileOffset;
    uint64_t Address;
    uint64_t Size;
    uint8_t Type;
    uint8_t Bind;
    uint16_t SectionIndex;
    char const *Name;
  };

  ELFMemMapping *LoadELFMapping(const FileMapping::FileMapping *Mapping, int FD);
  ELFSymbol *GetSymbolFromAddress(ELFMemMapping *Mapping, uint64_t Addr);
}
