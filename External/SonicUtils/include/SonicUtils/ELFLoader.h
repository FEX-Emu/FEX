#pragma once

#include <elf.h>
#include <functional>
#include <map>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

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

  uint64_t GetEntryPoint() const { return Header.e_entry; }

  using MemoryLayout = std::tuple<uint64_t, uint64_t, uint64_t>;

  MemoryLayout GetLayout() const {
    return std::make_tuple(MinPhysicalMemoryLocation, MaxPhysicalMemoryLocation,
                           PhysicalMemorySize);
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
  std::string &InterpreterLocation() { return DynamicLinker; }

  std::vector<char const*> const *GetNecessaryLibs() const { return &NecessaryLibs; }

  void PrintRelocationTable() const;

  using SymbolGetter = std::function<ELFSymbol*(char const*, uint8_t)>;
  void FixupRelocations(void *ELFBase, uint64_t GuestELFBase, SymbolGetter Getter);

  using SymbolAdder = std::function<void(ELFSymbol*)>;
  void AddSymbols(SymbolAdder Adder);

  void GetInitLocations(uint64_t GuestELFBase, std::vector<uint64_t> *Locations);

  bool HasTLS() const { return TLSHeader != nullptr; }
  uint64_t InitializeThreadSlot(void *ELFBase, std::function<void(void const*, uint64_t)> Writer) const;

private:
  bool LoadELF(std::string const &Filename);
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
  Elf64_Ehdr Header;
  std::vector<Elf64_Shdr*> SectionHeaders;
  std::vector<Elf64_Phdr*> ProgramHeaders;
  std::vector<ELFSymbol> Symbols;
  std::unordered_map<std::string, ELFSymbol *> SymbolMap;
  std::map<uint64_t, ELFSymbol *> SymbolMapByAddress;

  std::vector<char const*> NecessaryLibs;

  uint64_t MinPhysicalMemoryLocation{0};
  uint64_t MaxPhysicalMemoryLocation{0};
  uint64_t PhysicalMemorySize{0};
  Elf64_Phdr* InterpreterHeader{};
  bool DynamicProgram{false};
  std::string DynamicLinker;
  Elf64_Phdr const *TLSHeader{};
};

} // namespace ELFLoader
