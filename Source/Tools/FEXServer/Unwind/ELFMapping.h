#pragma once
#include <cstdint>
#include <elf.h>
#include <functional>
#include <map>
#include <string>

namespace FileMapping {
  struct FileMapping;
}

namespace ELFMapping {
  struct ELFSymbol {
    uint64_t FileOffset;
    uint64_t Address;
    uint64_t Size;
    uint8_t Type;
    uint8_t Bind;
    uint16_t SectionIndex;
    char const *Name;
  };

  struct ELFMemMapping {
    ELFMemMapping() = default;
    ELFMemMapping(const ELFMemMapping&) = delete;
    ~ELFMemMapping();

    int FD;
    uint64_t OriginalBase;
    char *BaseRemapped;
    size_t TotalSize;

    struct MappedRemapping {
      uint64_t OriginalBase;
      void* Remapped;
      size_t Size;
    };
    std::vector<MappedRemapping> ReadMappings;

    union {
      Elf32_Ehdr _32;
      Elf64_Ehdr _64;
    } Header;

    union SectionHeader {
      const Elf32_Shdr *_32;
      const Elf64_Shdr *_64;
    };

    union ProgramHeader {
      const Elf32_Phdr *_32;
      const Elf64_Phdr *_64;
    };

    std::vector<SectionHeader> SectionHeaders;
    std::vector<ProgramHeader> ProgramHeaders;
    std::vector<ELFSymbol> Symbols;
    std::vector<std::string> AddressSymbolNames;
    std::vector<uintptr_t> UnwindEntries;
    std::unordered_map<std::string, ELFSymbol *> SymbolMap;
    std::map<uint64_t, ELFSymbol *> SymbolMapByAddress;
  };

  ELFMemMapping *LoadELFMapping(const FileMapping::FileMapping *Mapping, int FD);
  ELFSymbol *GetSymbolFromAddress(ELFMemMapping *Mapping, uint64_t Addr);
}
