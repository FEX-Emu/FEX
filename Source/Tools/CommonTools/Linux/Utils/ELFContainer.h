// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/fextl/map.h>
#include <FEXCore/fextl/unordered_map.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>

#include <cstdint>
#include <elf.h>
#include <functional>
#include <stddef.h>
#include <utility>

// Add macros which are missing in some versions of <elf.h>
#ifndef ELF32_ST_VISIBILITY
#define ELF32_ST_VISIBILITY(o) ((o) & 0x3)
#endif

#ifndef ELF64_ST_VISIBILITY
#define ELF64_ST_VISIBILITY(o) ((o) & 0x3)
#endif

namespace ELFLoader {
struct ELFSymbol {
  uint64_t FileOffset;
  uint64_t Address;
  uint64_t Size;
  uint8_t Type;
  uint8_t Bind;
  uint16_t SectionIndex;
  const char* Name;
};

class ELFContainer {
public:
  ELFContainer(const fextl::string& Filename, const fextl::string& RootFS, bool CustomInterpreter);
  ~ELFContainer();

  uint64_t GetEntryPoint() const {
    if (Mode == MODE_32BIT) {
      return Header._32.e_entry;
    } else {
      return Header._64.e_entry;
    }
  }

  struct MemoryLayout final {
    uint64_t MinPhysicalMemoryLocation;
    uint64_t MaxPhysicalMemoryLocation;
    uint64_t PhysicalMemorySize;
  };

  MemoryLayout GetLayout() const {
    return {MinPhysicalMemoryLocation, MaxPhysicalMemoryLocation, PhysicalMemorySize};
  }

  struct BRKInfo {
    uint64_t Base;
    uint64_t Size;
  };

  BRKInfo GetBRKInfo() const {
    return {BRKBase, BRKSize};
  }

  // Data, Physical, Size
  using MemoryWriter = std::function<void(void*, uint64_t, uint64_t)>;
  void WriteLoadableSections(MemoryWriter Writer, uint64_t Offset = 0);

  const ELFSymbol* GetSymbol(const char* Name);
  const ELFSymbol* GetSymbol(uint64_t Address);

  using RangeType = std::pair<uint64_t, uint64_t>;
  const ELFSymbol* GetSymbolInRange(RangeType Address);

  bool WasDynamic() const {
    return DynamicProgram;
  }
  bool HasDynamicLinker() const {
    return !DynamicLinker.empty();
  }
  bool WasLoaded() const {
    return Loaded;
  }
  fextl::string& InterpreterLocation() {
    return DynamicLinker;
  }

  const fextl::vector<const char*>* GetNecessaryLibs() const {
    return &NecessaryLibs;
  }

  using SymbolGetter = std::function<ELFSymbol*(const char*, uint8_t)>;
  void FixupRelocations(void* ELFBase, uint64_t GuestELFBase, SymbolGetter Getter);

  using SymbolAdder = std::function<void(ELFSymbol*)>;
  void AddSymbols(SymbolAdder Adder);

  using UnwindAdder = std::function<void(uintptr_t)>;
  void AddUnwindEntries(UnwindAdder Adder);

  void GetInitLocations(uint64_t GuestELFBase, fextl::vector<uint64_t>* Locations);


  bool HasTLS() const {
    return TLSHeader._64 != nullptr;
  }
  uint64_t GetTLSBase() const {
    if (GetMode() == ELFMode::MODE_64BIT) {
      return TLSHeader._64->p_vaddr;
    } else {
      return TLSHeader._32->p_vaddr;
    }
  }

  enum ELFMode {
    MODE_32BIT,
    MODE_64BIT,
  };

  ELFMode GetMode() const {
    return Mode;
  }
  size_t GetProgramHeaderCount() const {
    return ProgramHeaders.size();
  }

  enum ELFType {
    TYPE_NONE,
    TYPE_X86_64,
    TYPE_X86_32,
    TYPE_OTHER_ELF,
  };
  static ELFType GetELFType(const fextl::string& Filename);
  static ELFType GetELFType(int FD);
  static bool IsSupportedELF(const fextl::string& Filename) {
    ELFType Type = GetELFType(Filename);
    return Type == TYPE_X86_64 || Type == TYPE_X86_32;
  }

private:
  bool LoadELF(const fextl::string& Filename);
  bool LoadELF_32();
  bool LoadELF_64();
  void CalculateMemoryLayouts();
  void CalculateSymbols();
  void GetDynamicLibs();

  fextl::vector<char> RawFile;
  union {
    Elf32_Ehdr _32;
    Elf64_Ehdr _64;
  } Header;

  union SectionHeader {
    Elf32_Shdr* _32;
    Elf64_Shdr* _64;
  };

  union ProgramHeader {
    Elf32_Phdr* _32;
    Elf64_Phdr* _64;
  };

  ELFMode Mode;
  fextl::vector<SectionHeader> SectionHeaders;
  fextl::vector<ProgramHeader> ProgramHeaders;
  fextl::vector<ELFSymbol> Symbols;
  fextl::vector<uintptr_t> UnwindEntries;
  fextl::unordered_map<fextl::string, ELFSymbol*> SymbolMap;
  fextl::map<uint64_t, ELFSymbol*> SymbolMapByAddress;

  fextl::vector<const char*> NecessaryLibs;

  uint64_t MinPhysicalMemoryLocation {0};
  uint64_t MaxPhysicalMemoryLocation {0};
  uint64_t PhysicalMemorySize {0};

  uint64_t BRKBase {};
  uint64_t BRKSize {};
  ProgramHeader InterpreterHeader {};
  bool DynamicProgram {false};
  fextl::string DynamicLinker;
  ProgramHeader TLSHeader {};
  bool Loaded {false};
};

} // namespace ELFLoader
