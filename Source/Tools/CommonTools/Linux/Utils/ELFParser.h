// SPDX-License-Identifier: MIT
#pragma once
#include <FEXCore/Core/CodeCache.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>

#include <elf.h>
#include <fcntl.h>
#include <optional>
#include <unistd.h>

#include "Linux/Utils/ELFContainer.h"

/*
  Simpler elf parser, checks for the elf MAGIC COOKIE
  and loads the phdrs
  Also keeps an fd open
*/

struct ELFParser {
  Elf64_Ehdr ehdr;
  fextl::vector<Elf64_Phdr> phdrs;
  std::optional<fextl::vector<Elf64_Shdr>> shdrs;
  ::ELFLoader::ELFContainer::ELFType type {::ELFLoader::ELFContainer::TYPE_NONE};

  fextl::string InterpreterElf;
  int fd {-1};

  bool ReadElf(int NewFD) {
    Closefd();
    static_assert(EI_CLASS == 4);

    fd = NewFD;
    type = ::ELFLoader::ELFContainer::TYPE_NONE;
    shdrs.reset();

    if (fd == -1) {
      // Likely just doesn't exist
      return false;
    }

    // Get file size
    off_t Size = lseek(fd, 0, SEEK_END);

    if (Size < 4) {
      // Likely invalid can't fit header
      return false;
    }

    // Reset to beginning
    if (lseek(fd, 0, SEEK_SET) == -1) {
      return false;
    }

    uint8_t header[5];
    if (pread(fd, header, sizeof(header), 0) == -1) {
      LogMan::Msg::EFmt("Failed to read elf header from '{}'", fd);
      return false;
    }

    if (header[0] != ELFMAG0 || header[1] != ELFMAG1 || header[2] != ELFMAG2 || header[3] != ELFMAG3) {
      LogMan::Msg::EFmt("Elf header from '{}' doesn't match ELF MAGIC", fd);
      return false;
    }

    type = ::ELFLoader::ELFContainer::TYPE_OTHER_ELF;

    if (header[EI_CLASS] == ELFCLASS32) {
      Elf32_Ehdr hdr32;
      if (pread(fd, &hdr32, sizeof(hdr32), 0) == -1) {
        LogMan::Msg::EFmt("Failed to read Ehdr32 from '{}'", fd);
        return false;
      }

      // do the sizes match up as expected?

      // check elf header
      if (hdr32.e_ehsize != sizeof(hdr32)) {
        LogMan::Msg::EFmt("Invalid e_ehsize32 from '{}'", fd);
        return false;
      }

      // check program header
      if (hdr32.e_phentsize != sizeof(Elf32_Phdr)) {
        LogMan::Msg::EFmt("Invalid e_phentsize32 from '{}'", fd);
        return false;
      }

      // Convert to 64 bit header
      for (int i = 0; i < EI_NIDENT; i++) {
        ehdr.e_ident[i] = hdr32.e_ident[i];
      }

#define COPY(name) ehdr.name = hdr32.name
      COPY(e_type);
      COPY(e_machine);
      COPY(e_version);
      COPY(e_entry);
      COPY(e_phoff);
      COPY(e_shoff);
      COPY(e_flags);
      COPY(e_ehsize);
      COPY(e_phentsize);
      COPY(e_phnum);
      COPY(e_shentsize);
      COPY(e_shnum);
      COPY(e_shstrndx);
#undef COPY

      if (ehdr.e_machine != EM_386) {
        LogMan::Msg::EFmt("Invalid e_machine from '{}'", fd);
        return false;
      }

      type = ::ELFLoader::ELFContainer::TYPE_X86_32;
    } else if (header[EI_CLASS] == ELFCLASS64) {
      if (pread(fd, &ehdr, sizeof(ehdr), 0) == -1) {
        LogMan::Msg::EFmt("Failed to read Ehdr64 from '{}'", fd);
        return false;
      }

      // do the sizes match up as expected?

      // check elf header
      if (ehdr.e_ehsize != sizeof(ehdr)) {
        LogMan::Msg::EFmt("Invalid e_ehsize64 from '{}'", fd);
        return false;
      }

      // check program header
      if (ehdr.e_phentsize != sizeof(Elf64_Phdr)) {
        LogMan::Msg::EFmt("Invalid e_phentsize64 from '{}'", fd);
        return false;
      }

      if (ehdr.e_machine != EM_X86_64) {
        LogMan::Msg::EFmt("Invalid e_machine64 from '{}'", fd);
        return false;
      }

      type = ::ELFLoader::ELFContainer::TYPE_X86_64;
    } else {
      // Unexpected elf type
      LogMan::Msg::EFmt("Unexpected elf type from '{}'", fd);
      return false;
    }

    // sanity check program header count
    if (ehdr.e_phnum < 1 || ehdr.e_phnum > 65536 / ehdr.e_phentsize) {
      LogMan::Msg::EFmt("Too many program headers '{}'", fd);
      return false;
    }

    // sanity check program header offset size.
    if (ehdr.e_phoff > Size || (ehdr.e_phentsize * ehdr.e_phnum) > (Size - ehdr.e_phoff)) {
      LogMan::Msg::EFmt("Program headers exceeds size of program");
      return false;
    }

    if (type == ::ELFLoader::ELFContainer::TYPE_X86_32) {
      fextl::vector<Elf32_Phdr> phdrs32(ehdr.e_phnum);

      if (pread(fd, phdrs32.data(), sizeof(Elf32_Phdr) * ehdr.e_phnum, ehdr.e_phoff) == -1) {
        LogMan::Msg::EFmt("Failed to read phdr32 from '{}'", fd);
        return false;
      }

      // Convert to 64 bit program headers
      phdrs.resize(ehdr.e_phnum);

      for (int i = 0; i < ehdr.e_phnum; i++) {
#define COPY(name) phdrs[i].name = phdrs32[i].name

        COPY(p_type);
        COPY(p_offset);
        COPY(p_vaddr);
        COPY(p_paddr);
        COPY(p_filesz);
        COPY(p_memsz);
        COPY(p_flags);
        COPY(p_align);

#undef COPY
      }
    } else {
      phdrs.resize(ehdr.e_phnum);

      if (pread(fd, phdrs.data(), sizeof(Elf64_Phdr) * ehdr.e_phnum, ehdr.e_phoff) == -1) {
        LogMan::Msg::EFmt("Failed to read phdr64 from '{}'", fd);
        return false;
      }
    }

    for (const auto& phdr : phdrs) {
      if (phdr.p_type == PT_INTERP) {
        InterpreterElf.resize(phdr.p_filesz);

        if (pread(fd, InterpreterElf.data(), phdr.p_filesz, phdr.p_offset) == -1) {
          LogMan::Msg::EFmt("Failed to read interpreter from '{}'", fd);
          return false;
        }
      }
    }

    return true;
  }

  ptrdiff_t FileToVA(off_t FileOffset) const {
    for (const auto& phdr : phdrs) {
      if (phdr.p_offset <= FileOffset && (phdr.p_offset + phdr.p_filesz) > FileOffset) {
        auto SectionFileOffset = FileOffset - phdr.p_offset;

        if (SectionFileOffset < phdr.p_memsz) {
          return SectionFileOffset + phdr.p_vaddr;
        }
      }
    }

    return {};
  }

  off_t VAToFile(ptrdiff_t VAOffset) const {
    for (const auto& phdr : phdrs) {
      if (phdr.p_vaddr <= VAOffset && (phdr.p_vaddr + phdr.p_memsz) > VAOffset) {
        auto SectionVAOffset = VAOffset - phdr.p_vaddr;

        if (SectionVAOffset < phdr.p_filesz) {
          return SectionVAOffset + phdr.p_offset;
        }
      }
    }

    return {};
  }

  bool ReadElf(const fextl::string& file) {
    int NewFD = ::open(file.c_str(), O_RDONLY);

    return ReadElf(NewFD);
  }

  /**
   * Parses relocation sections (SHT_REL/SHT_RELA) and returns a map of
   * offsets to relocations that FEX's JIT must know about.
   */
  fextl::unordered_map<uint32_t, FEXCore::GuestRelocationType> PopulateRelocations() {
    if (fd == -1 || !EnsureSectionHeadersLoaded()) {
      return {};
    }

    fextl::unordered_map<uint32_t, FEXCore::GuestRelocationType> Relocations;
    bool Is32Bit = (type == ::ELFLoader::ELFContainer::TYPE_X86_32);

    for (const auto& shdr : *shdrs) {
      if (shdr.sh_entsize == 0) {
        continue;
      }

      const size_t EntryCount = shdr.sh_size / shdr.sh_entsize;

      if (!Is32Bit) {
        if (shdr.sh_type == SHT_REL) {
          LOGMAN_THROW_A_FMT(false, "Unexpected relocation section type");
        } else if (shdr.sh_type == SHT_RELA) {
          fextl::vector<Elf64_Rela> Entries(EntryCount);
          if (pread(fd, Entries.data(), shdr.sh_size, shdr.sh_offset) == -1) {
            LOGMAN_THROW_A_FMT(false, "Failed to read RELA section");
          }
          for (auto& Entry : Entries) {
            auto RelocType = ClassifyRelocation64(ELF64_R_TYPE(Entry.r_info));
            if (RelocType) {
              Relocations.emplace(static_cast<uint32_t>(Entry.r_offset), *RelocType);
            }
          }
        }
      } else {
        if (shdr.sh_type == SHT_REL) {
          fextl::vector<Elf32_Rel> Entries(EntryCount);
          if (pread(fd, Entries.data(), shdr.sh_size, shdr.sh_offset) == -1) {
            LOGMAN_THROW_A_FMT(false, "Failed to read REL section");
          }
          for (auto& Entry : Entries) {
            auto RelocType = ClassifyRelocation32(ELF32_R_TYPE(Entry.r_info));
            if (RelocType) {
              Relocations.emplace(static_cast<uint32_t>(Entry.r_offset), *RelocType);
            }
          }
        } else if (shdr.sh_type == SHT_RELA) {
          fextl::vector<Elf32_Rela> Entries(EntryCount);
          if (pread(fd, Entries.data(), shdr.sh_size, shdr.sh_offset) == -1) {
            LOGMAN_THROW_A_FMT(false, "Failed to read RELA section");
          }
          for (auto& Entry : Entries) {
            auto RelocType = ClassifyRelocation32(ELF32_R_TYPE(Entry.r_info));
            if (RelocType) {
              Relocations.emplace(static_cast<uint32_t>(Entry.r_offset), *RelocType);
            }
          }
        }
      }
    }

    return Relocations;
  }

  /**
   * Returns underlying 32-bit relocation entries.
   * SHT_REL entries are implicitly converted to Elf32_Rela.
   */
  fextl::vector<Elf32_Rela> ReadRawRelocations32() {
    if (fd == -1 || type != ::ELFLoader::ELFContainer::TYPE_X86_32 || !EnsureSectionHeadersLoaded()) {
      return {};
    }

    // Load dynamic symbol table (find SHT_DYNSYM section)
    fextl::vector<Elf32_Sym> DynSyms;
    auto DynsymHeader = std::ranges::find_if(*shdrs, [](auto& shdr) { return shdr.sh_type == SHT_DYNSYM; });
    if (DynsymHeader != shdrs->end()) {
      size_t SymCount = DynsymHeader->sh_size / sizeof(Elf32_Sym);
      DynSyms.resize(SymCount);
      if (pread(fd, DynSyms.data(), DynsymHeader->sh_size, DynsymHeader->sh_offset) == -1) {
        LOGMAN_MSG_A_FMT("Could not load DYNSYM section");
      }
    }

    fextl::vector<Elf32_Rela> Result;
    for (const auto& shdr : *shdrs) {
      if (shdr.sh_entsize == 0) {
        continue;
      }

      const size_t EntryCount = shdr.sh_size / shdr.sh_entsize;

      if (shdr.sh_type == SHT_REL) {
        fextl::vector<Elf32_Rel> Entries(EntryCount);
        if (pread(fd, Entries.data(), shdr.sh_size, shdr.sh_offset) == -1) {
          LOGMAN_MSG_A_FMT("Could not load REL section");
        }
        for (auto& Entry : Entries) {
          auto Sym = ELF32_R_SYM(Entry.r_info);
          int32_t Addend = (Sym < DynSyms.size()) ? static_cast<int32_t>(DynSyms[Sym].st_value) : 0;
          Result.push_back(Elf32_Rela {Entry.r_offset, Entry.r_info, Addend});
        }
      } else if (shdr.sh_type == SHT_RELA) {
        fextl::vector<Elf32_Rela> Entries(EntryCount);
        if (pread(fd, Entries.data(), shdr.sh_size, shdr.sh_offset) == -1) {
          LOGMAN_MSG_A_FMT("Could not load RELA section");
        }
        Result.insert(Result.end(), Entries.begin(), Entries.end());
      }
    }

    return Result;
  }

  void Closefd() {
    if (fd != -1) {
      close(fd);
      fd = -1;
    }
  }

  ~ELFParser() {
    Closefd();
  }

private:
  /// Returns true if loading section headers succeeded
  bool EnsureSectionHeadersLoaded() {
    if (shdrs.has_value()) {
      return !shdrs->empty();
    }

    if (fd == -1 || ehdr.e_shoff == 0 || ehdr.e_shnum == 0) {
      shdrs.emplace();
      return false;
    }

    if (type == ::ELFLoader::ELFContainer::TYPE_X86_64) {
      shdrs.emplace(ehdr.e_shnum);
      if (pread(fd, shdrs->data(), sizeof(Elf64_Shdr) * ehdr.e_shnum, ehdr.e_shoff) == -1) {
        shdrs->clear();
        return false;
      }
    } else {
      fextl::vector<Elf32_Shdr> shdrs32(ehdr.e_shnum);
      if (pread(fd, shdrs32.data(), sizeof(Elf32_Shdr) * ehdr.e_shnum, ehdr.e_shoff) == -1) {
        shdrs.emplace();
        return false;
      }

      shdrs.emplace(ehdr.e_shnum);
      for (int i = 0; i < ehdr.e_shnum; i++) {
#define COPY(name) (*shdrs)[i].name = shdrs32[i].name
        COPY(sh_name);
        COPY(sh_type);
        COPY(sh_flags);
        COPY(sh_addr);
        COPY(sh_offset);
        COPY(sh_size);
        COPY(sh_link);
        COPY(sh_info);
        COPY(sh_addralign);
        COPY(sh_entsize);
#undef COPY
      }
    }

    return !shdrs->empty();
  }

  static std::optional<FEXCore::GuestRelocationType> ClassifyRelocation32(uint32_t Type) {
    if (Type == R_386_RELATIVE || Type == R_386_32) {
      return FEXCore::GuestRelocationType::Rel32;
    }
    return std::nullopt;
  }

  static std::optional<FEXCore::GuestRelocationType> ClassifyRelocation64(uint32_t Type) {
    if (Type == R_X86_64_RELATIVE || Type == R_X86_64_64) {
      return FEXCore::GuestRelocationType::Rel64;
    } else if (Type == R_X86_64_32) {
      return FEXCore::GuestRelocationType::Rel32;
    }
    return std::nullopt;
  }
};
