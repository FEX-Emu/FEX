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
  char const *Name;
};

class ELFContainer {
public:
  ELFContainer(std::string const &Filename);
  ~ELFContainer();

  uint64_t GetEntryPoint() const { return Header.e_entry; }

  using MemoryLayout = std::tuple<uint64_t, uint64_t, uint64_t>;

  MemoryLayout GetLayout() const {
    return std::make_tuple(MinPhysicalMemoryLocation, MaxPhysicalMemoryLocation,
                           PhysicalMemorySize);
  }

  // Data, Physical, Size
  using MemoryWriter = std::function<void(void *, uint64_t, uint64_t)>;
  void WriteLoadableSections(MemoryWriter Writer);

  ELFSymbol const *GetSymbol(char const *Name);
  ELFSymbol const *GetSymbol(uint64_t Address);

  using RangeType = std::pair<uint64_t, uint64_t>;
  ELFSymbol const *GetSymbolInRange(RangeType Address);

private:
  void CalculateMemoryLayouts();
  void CalculateSymbols();

  // Information functions
  void PrintHeader() const;
  void PrintSectionHeaders() const;
  void PrintProgramHeaders() const;
  void PrintSymbolTable() const;

  std::vector<char> RawFile;
  Elf64_Ehdr Header;
  std::vector<Elf64_Shdr*> SectionHeaders;
  std::vector<Elf64_Phdr*> ProgramHeaders;
  std::vector<ELFSymbol> Symbols;
  std::unordered_map<char const*, ELFSymbol *> SymbolMap;
  std::map<uint64_t, ELFSymbol *> SymbolMapByAddress;

  uint64_t MinPhysicalMemoryLocation{0};
  uint64_t MaxPhysicalMemoryLocation{0};
  uint64_t PhysicalMemorySize{0};
  bool IsStatic{true};
};

} // namespace ELFLoader
