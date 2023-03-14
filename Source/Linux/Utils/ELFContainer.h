#pragma once

#include <FEXCore/fextl/unordered_map.h>

#include <cstdint>
#include <elf.h>
#include <functional>
#include <map>
#include <stddef.h>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

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
  char const *Name;
};

class ELFContainer {
public:
  ELFContainer(std::string const &Filename, std::string const &RootFS, bool CustomInterpreter);
  ~ELFContainer();

  uint64_t GetEntryPoint() const {
    if (Mode == MODE_32BIT) {
      return Header._32.e_entry;
    }
    else {
      return Header._64.e_entry;
    }
  }

  using MemoryLayout = std::tuple<uint64_t, uint64_t, uint64_t>;

  MemoryLayout GetLayout() const {
    return std::make_tuple(MinPhysicalMemoryLocation, MaxPhysicalMemoryLocation,
                           PhysicalMemorySize);
  }

  struct BRKInfo {
    uint64_t Base;
    uint64_t Size;
  };

  BRKInfo GetBRKInfo() const {
    return {BRKBase, BRKSize};
  }

  // Data, Physical, Size
  using MemoryWriter = std::function<void(void *, uint64_t, uint64_t)>;
  void WriteLoadableSections(MemoryWriter Writer, uint64_t Offset = 0);

  ELFSymbol const *GetSymbol(char const *Name);
  ELFSymbol const *GetSymbol(uint64_t Address);

  using RangeType = std::pair<uint64_t, uint64_t>;
  ELFSymbol const *GetSymbolInRange(RangeType Address);

  bool WasDynamic() const { return DynamicProgram; }
  bool HasDynamicLinker() const { return !DynamicLinker.empty(); }
  bool WasLoaded() const { return Loaded; }
  std::string &InterpreterLocation() { return DynamicLinker; }

  std::vector<char const*> const *GetNecessaryLibs() const { return &NecessaryLibs; }

  void PrintRelocationTable() const;

  using SymbolGetter = std::function<ELFSymbol*(char const*, uint8_t)>;
  void FixupRelocations(void *ELFBase, uint64_t GuestELFBase, SymbolGetter Getter);

  using SymbolAdder = std::function<void(ELFSymbol*)>;
  void AddSymbols(SymbolAdder Adder);

  using UnwindAdder = std::function<void(uintptr_t)>;
  void AddUnwindEntries(UnwindAdder Adder);

  void GetInitLocations(uint64_t GuestELFBase, std::vector<uint64_t> *Locations);


  bool HasTLS() const { return TLSHeader._64 != nullptr; }
  uint64_t GetTLSBase() const {
    if (GetMode() == ELFMode::MODE_64BIT) {
      return TLSHeader._64->p_vaddr;
    }
    else {
      return TLSHeader._32->p_vaddr;
    }
  }

  enum ELFMode {
    MODE_32BIT,
    MODE_64BIT,
  };

  ELFMode GetMode() const { return Mode; }
  size_t GetProgramHeaderCount() const { return ProgramHeaders.size(); }

  enum ELFType {
    TYPE_NONE,
    TYPE_X86_64,
    TYPE_X86_32,
    TYPE_OTHER_ELF,
  };
  static ELFType GetELFType(std::string const &Filename);
  static ELFType GetELFType(int FD);
  static bool IsSupportedELF(std::string const &Filename) {
    ELFType Type = GetELFType(Filename);
    return Type == TYPE_X86_64 || Type == TYPE_X86_32;
  }

private:
  bool LoadELF(std::string const &Filename);
  bool LoadELF_32();
  bool LoadELF_64();
  void CalculateMemoryLayouts();
  void CalculateSymbols();
  void GetDynamicLibs();

  // Information functions
  void PrintHeader() const;
  void PrintSectionHeaders() const;
  void PrintProgramHeaders() const;
  void PrintSymbolTable() const;
  void PrintInitArray() const;
  void PrintDynamicTable() const;

  std::vector<char> RawFile;
  union {
    Elf32_Ehdr _32;
    Elf64_Ehdr _64;
  } Header;

  union SectionHeader {
    Elf32_Shdr *_32;
    Elf64_Shdr *_64;
  };

  union ProgramHeader {
    Elf32_Phdr *_32;
    Elf64_Phdr *_64;
  };

  ELFMode Mode;
  std::vector<SectionHeader> SectionHeaders;
  std::vector<ProgramHeader> ProgramHeaders;
  std::vector<ELFSymbol> Symbols;
  std::vector<uintptr_t> UnwindEntries;
  fextl::unordered_map<std::string, ELFSymbol *> SymbolMap;
  std::map<uint64_t, ELFSymbol *> SymbolMapByAddress;

  std::vector<char const*> NecessaryLibs;

  uint64_t MinPhysicalMemoryLocation{0};
  uint64_t MaxPhysicalMemoryLocation{0};
  uint64_t PhysicalMemorySize{0};

  uint64_t BRKBase{};
  uint64_t BRKSize{};
  ProgramHeader InterpreterHeader{};
  bool DynamicProgram{false};
  std::string DynamicLinker;
  ProgramHeader TLSHeader{};
  bool Loaded {false};
};

} // namespace ELFLoader
