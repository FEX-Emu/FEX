#include "ELFLoader.h"
#include "LogManager.h"
#include <cstring>
#include <elf.h>
#include <fstream>
#include <stdint.h>
#include <vector>

namespace ELFLoader {

ELFContainer::ELFContainer(std::string const &Filename) {
  std::fstream ELFFile;
  size_t FileSize{0};
  ELFFile.open(Filename, std::fstream::in | std::fstream::binary);
  LogMan::Throw::A(ELFFile.is_open(), "Failed to open file");

  ELFFile.seekg(0, ELFFile.end);
  FileSize = ELFFile.tellg();
  ELFFile.seekg(0, ELFFile.beg);

  RawFile.resize(FileSize);

  ELFFile.read(&RawFile.at(0), FileSize);

  ELFFile.close();

  memcpy(&Header, reinterpret_cast<Elf64_Ehdr *>(&RawFile.at(0)),
         sizeof(Elf64_Ehdr));
  LogMan::Throw::A(Header.e_type == ET_EXEC, "ELF wasn't EXEC");
  LogMan::Throw::A(Header.e_phentsize == 56, "PH Entry size wasn't 56");
  LogMan::Throw::A(Header.e_shentsize == 64, "PH Entry size wasn't 64");

  SectionHeaders.resize(Header.e_shnum);
  ProgramHeaders.resize(Header.e_phnum);

  Elf64_Shdr *RawShdrs =
      reinterpret_cast<Elf64_Shdr *>(&RawFile.at(Header.e_shoff));
  Elf64_Phdr *RawPhdrs =
      reinterpret_cast<Elf64_Phdr *>(&RawFile.at(Header.e_phoff));

  for (uint32_t i = 0; i < Header.e_shnum; ++i) {
    memcpy(&SectionHeaders.at(i), &RawShdrs[i], Header.e_shentsize);
  }

  for (uint32_t i = 0; i < Header.e_phnum; ++i) {
    memcpy(&ProgramHeaders.at(i), &RawPhdrs[i], Header.e_phentsize);
  }

  CalculateMemoryLayouts();
  CalculateSymbols();

  // Print Information
  PrintHeader();
  //PrintSectionHeaders();
  //PrintProgramHeaders();
  //PrintSymbolTable();
}

ELFContainer::~ELFContainer() {
  SymbolMapByAddressRange.clear();
  SymbolMapByAddress.clear();
  SymbolMap.clear();
  Symbols.clear();
  ProgramHeaders.clear();
  SectionHeaders.clear();
  RawFile.clear();
}

void ELFContainer::WriteLoadableSections(MemoryWriter Writer) {
  for (uint32_t i = 0; i < ProgramHeaders.size(); ++i) {
    Elf64_Phdr const &hdr = ProgramHeaders.at(i);
    if (hdr.p_type != PT_LOAD)
      continue;
    Writer(&RawFile.at(hdr.p_offset), hdr.p_paddr, hdr.p_filesz);
  }
}

ELFSymbol const *ELFContainer::GetSymbol(std::string const &Name) {
  auto Sym = SymbolMap.find(Name);
  if (Sym == SymbolMap.end())
    return nullptr;
  return Sym->second;
}
ELFSymbol const *ELFContainer::GetSymbol(uint64_t Address) {
  auto Sym = SymbolMapByAddress.find(Address);
  if (Sym == SymbolMapByAddress.end())
    return nullptr;
  return Sym->second;
}
ELFSymbol const *ELFContainer::GetSymbolInRange(RangeType Address) {
  auto Sym = SymbolMapByAddress.lower_bound(Address.first);
  if (Sym == SymbolMapByAddress.end())
    return nullptr;
  return Sym->second;
}

void ELFContainer::CalculateMemoryLayouts() {
  uint64_t MinPhysAddr = ~0ULL;
  uint64_t MaxPhysAddr = 0;
  uint64_t PhysMemSize = 0;
  for (uint32_t i = 0; i < ProgramHeaders.size(); ++i) {
    Elf64_Phdr const &hdr = ProgramHeaders.at(i);
    MinPhysAddr = std::min(MinPhysAddr, hdr.p_paddr);
    MaxPhysAddr = std::max(MaxPhysAddr, hdr.p_paddr + hdr.p_memsz);
  }

  PhysMemSize = MaxPhysAddr - MinPhysAddr;

  MinPhysicalMemoryLocation = MinPhysAddr;
  MaxPhysicalMemoryLocation = MaxPhysAddr;
  PhysicalMemorySize = PhysMemSize;
  LogMan::Msg::D("Min PAddr: 0x%lx", MinPhysAddr);
  LogMan::Msg::D("Max PAddr: 0x%lx", MaxPhysAddr);
  LogMan::Msg::D("Physical Size: 0x%lx", PhysMemSize);
}

void ELFContainer::CalculateSymbols() {
  // Find the symbol table
  Elf64_Shdr const *SymTabHeader{nullptr};
  Elf64_Shdr const *StringTableHeader{nullptr};
  char const *StrTab{nullptr};
  for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
    Elf64_Shdr const *hdr = &SectionHeaders.at(i);
    if (hdr->sh_type == SHT_SYMTAB) {
      SymTabHeader = hdr;
      break;
    }
  }
  if (!SymTabHeader) {
    LogMan::Msg::I("No Symbol table");
    return;
  }

  LogMan::Throw::A(SymTabHeader->sh_link < SectionHeaders.size(),
                   "Symbol table string table section is wrong");
  LogMan::Throw::A(SymTabHeader->sh_entsize == sizeof(Elf64_Sym),
                   "Entry size doesn't match symbol entry");

  StringTableHeader = &SectionHeaders.at(SymTabHeader->sh_link);
  StrTab = &RawFile.at(StringTableHeader->sh_offset);

  uint64_t NumSymbols = SymTabHeader->sh_size / SymTabHeader->sh_entsize;
  Symbols.resize(NumSymbols);
  for (uint64_t i = 0; i < NumSymbols; ++i) {
    uint64_t offset = SymTabHeader->sh_offset + i * SymTabHeader->sh_entsize;
    Elf64_Sym const *Symbol =
        reinterpret_cast<Elf64_Sym const *>(&RawFile.at(offset));
    if (ELF64_ST_VISIBILITY(Symbol->st_other) != STV_HIDDEN) {
      ELFSymbol DefinedSymbol{};
      DefinedSymbol.FileOffset = offset;
      DefinedSymbol.Address = Symbol->st_value;
      DefinedSymbol.Size = Symbol->st_size;
      DefinedSymbol.Type = ELF64_ST_TYPE(Symbol->st_info);
      DefinedSymbol.Bind = ELF64_ST_BIND(Symbol->st_info);
      DefinedSymbol.Name = &StrTab[Symbol->st_name];

      Symbols.emplace_back(DefinedSymbol);
      auto Sym = &Symbols.back();
      SymbolMap[DefinedSymbol.Name] = Sym;
      SymbolMapByAddress[DefinedSymbol.Address] = Sym;
      SymbolMapByAddressRange[std::make_pair(DefinedSymbol.Address, DefinedSymbol.Address + Symbol->st_size)] = Sym;
    }
  }
}

void ELFContainer::PrintHeader() const {
  LogMan::Msg::I("Type: %d", Header.e_type);
  LogMan::Msg::I("Machine: %d", Header.e_machine);
  LogMan::Msg::I("Version: %d", Header.e_version);
  LogMan::Msg::I("Entry point: 0x%lx", Header.e_entry);
  LogMan::Msg::I("PH Off: %d", Header.e_phoff);
  LogMan::Msg::I("SH Off: %d", Header.e_shoff);
  LogMan::Msg::I("Flags: %d", Header.e_flags);
  LogMan::Msg::I("EH Size: %d", Header.e_ehsize);
  LogMan::Msg::I("PH Num: %d", Header.e_phnum);
  LogMan::Msg::I("SH Num: %d", Header.e_shnum);
  LogMan::Msg::I("PH Entry Size: %d", Header.e_phentsize);
  LogMan::Msg::I("SH Entry Size: %d", Header.e_shentsize);
  LogMan::Msg::I("SH Str Index: %d", Header.e_shstrndx);
}

void ELFContainer::PrintSectionHeaders() const {
  LogMan::Throw::A(Header.e_shstrndx < SectionHeaders.size(),
                   "String index section is wrong index!");
  Elf64_Shdr const &StrHeader = SectionHeaders.at(Header.e_shstrndx);
  char const *SHStrings = &RawFile.at(StrHeader.sh_offset);
  for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
    Elf64_Shdr const &hdr = SectionHeaders.at(i);
    LogMan::Msg::I("Index: %d", i);
    LogMan::Msg::I("Name:       %s", &SHStrings[hdr.sh_name]);
    LogMan::Msg::I("Type:       %d", hdr.sh_type);
    LogMan::Msg::I("Flags:      %d", hdr.sh_flags);
    LogMan::Msg::I("Addr:       0x%lx", hdr.sh_addr);
    LogMan::Msg::I("Offset:     0x%lx", hdr.sh_offset);
    LogMan::Msg::I("Size:       %d", hdr.sh_size);
    LogMan::Msg::I("Link:       %d", hdr.sh_link);
    LogMan::Msg::I("Info:       %d", hdr.sh_info);
    LogMan::Msg::I("AddrAlign:  %d", hdr.sh_addralign);
    LogMan::Msg::I("Entry Size: %d", hdr.sh_entsize);
  }
}

void ELFContainer::PrintProgramHeaders() const {
  LogMan::Throw::A(Header.e_shstrndx < SectionHeaders.size(),
                   "String index section is wrong index!");
  Elf64_Shdr const &StrHeader = SectionHeaders.at(Header.e_shstrndx);
  char const *SHStrings = &RawFile.at(StrHeader.sh_offset);
  for (uint32_t i = 0; i < ProgramHeaders.size(); ++i) {
    Elf64_Phdr const &hdr = ProgramHeaders.at(i);
    LogMan::Msg::I("Type:    %d", hdr.p_type);
    LogMan::Msg::I("Flags:   %d", hdr.p_flags);
    LogMan::Msg::I("Offset:  %d", hdr.p_offset);
    LogMan::Msg::I("VAddr:   0x%lx", hdr.p_vaddr);
    LogMan::Msg::I("PAddr:   0x%lx", hdr.p_paddr);
    LogMan::Msg::I("FSize:   %d", hdr.p_filesz);
    LogMan::Msg::I("MemSize: %d", hdr.p_memsz);
    LogMan::Msg::I("Align:   %d", hdr.p_align);
  }
}

void ELFContainer::PrintSymbolTable() const {
  // Find the symbol table
  Elf64_Shdr const *SymTabHeader{nullptr};
  Elf64_Shdr const *StringTableHeader{nullptr};
  char const *StrTab{nullptr};
  for (uint32_t i = 0; i < SectionHeaders.size(); ++i) {
    Elf64_Shdr const *hdr = &SectionHeaders.at(i);
    if (hdr->sh_type == SHT_SYMTAB) {
      SymTabHeader = hdr;
      break;
    }
  }
  if (!SymTabHeader) {
    LogMan::Msg::I("No Symbol table");
    return;
  }

  LogMan::Throw::A(SymTabHeader->sh_link < SectionHeaders.size(),
                   "Symbol table string table section is wrong");
  LogMan::Throw::A(SymTabHeader->sh_entsize == sizeof(Elf64_Sym),
                   "Entry size doesn't match symbol entry");

  StringTableHeader = &SectionHeaders.at(SymTabHeader->sh_link);
  StrTab = &RawFile.at(StringTableHeader->sh_offset);

  uint64_t NumSymbols = SymTabHeader->sh_size / SymTabHeader->sh_entsize;
  for (uint64_t i = 0; i < NumSymbols; ++i) {
    uint64_t offset = SymTabHeader->sh_offset + i * SymTabHeader->sh_entsize;
    Elf64_Sym const *Symbol =
        reinterpret_cast<Elf64_Sym const *>(&RawFile.at(offset));
    std::ostringstream Str{};
    Str << i << " : " << std::hex << Symbol->st_value << std::dec << " "
        << Symbol->st_size << " " << uint32_t(Symbol->st_info) << " "
        << uint32_t(Symbol->st_other) << " " << Symbol->st_shndx << " "
        << &StrTab[Symbol->st_name];
    LogMan::Msg::I("%s", Str.str().c_str());
  }
}

} // namespace ELFLoader
