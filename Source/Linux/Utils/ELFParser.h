#pragma once
#include <vector>
#include <string>
#include <elf.h>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>

#include "Linux/Utils/ELFContainer.h"

/*
  Simpler elf parser, checks for the elf MAGIC COOKIE
  and loads the phdrs
  Also keeps an fd open
*/

struct ELFParser {
  Elf64_Ehdr ehdr;
  std::vector<Elf64_Phdr> phdrs;
  ::ELFLoader::ELFContainer::ELFType type {::ELFLoader::ELFContainer::TYPE_NONE};

  std::string InterpreterElf;
  int fd {-1};

  bool ReadElf(int NewFD) {
    Closefd();
    static_assert(EI_CLASS == 4);

    fd = NewFD;
    type = ::ELFLoader::ELFContainer::TYPE_NONE;

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
    lseek(fd, 0, SEEK_SET);

    uint8_t header[5];
    if (pread(fd, header, sizeof(header), 0) == -1) {
      LogMan::Msg::E("Failed to read elf header from '%d'", fd);
      return false;
    }

    if (header[0] != ELFMAG0 || header[1] != ELFMAG1 || header[2] != ELFMAG2 || header[3] != ELFMAG3) {
      LogMan::Msg::E("Elf header from '%d' doesn't match ELF MAGIC", fd);
      return false;
    }

    type = ::ELFLoader::ELFContainer::TYPE_OTHER_ELF;

    if (header[EI_CLASS] == ELFCLASS32) {
      Elf32_Ehdr hdr32;
      if (pread(fd, &hdr32, sizeof(hdr32), 0) == -1) {
        LogMan::Msg::E("Failed to read Ehdr32 from '%d'", fd);
        return false;
      }

      // do the sizes match up as expected?

      // check elf header
      if (hdr32.e_ehsize != sizeof(hdr32)) {
        LogMan::Msg::E("Invalid e_ehsize32 from '%d'", fd);
        return false;
      }

      // check program header
      if (hdr32.e_phentsize != sizeof(Elf32_Phdr)) {
        LogMan::Msg::E("Invalid e_phentsize32 from '%d'", fd);
        return false;
      }

      // Convert to 64 bit header
      for (int i = 0; i < EI_NIDENT; i++)
        ehdr.e_ident[i] = hdr32.e_ident[i];

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
        LogMan::Msg::E("Invalid e_machine from '%d'", fd);
        return false;
      }

      type = ::ELFLoader::ELFContainer::TYPE_X86_32;
    } else if (header[EI_CLASS] == ELFCLASS64) {
      if (pread(fd, &ehdr, sizeof(ehdr), 0) == -1) {
        LogMan::Msg::E("Failed to read Ehdr64 from '%d'", fd);
        return false;
      }

      // do the sizes match up as expected?

      // check elf header
      if (ehdr.e_ehsize != sizeof(ehdr)) {
        LogMan::Msg::E("Invalid e_ehsize64 from '%d'", fd);
        return false;
      }

      // check program header
      if (ehdr.e_phentsize != sizeof(Elf64_Phdr)) {
        LogMan::Msg::E("Invalid e_phentsize64 from '%d'", fd);
        return false;
      }

      if (ehdr.e_machine != EM_X86_64) {
        LogMan::Msg::E("Invalid e_machine64 from '%d'", fd);
        return false;
      }

      type = ::ELFLoader::ELFContainer::TYPE_X86_64;
    } else {
      // Unexpected elf type
      LogMan::Msg::E("Unexpected elf type from '%d'", fd);
      return false;
    }

    // sanity check program header count
    if (ehdr.e_phnum < 1 || ehdr.e_phnum > 65536 / ehdr.e_phentsize) {
      LogMan::Msg::E("Too many program headers '%d'", fd);
      return false;
    }

    if (type == ::ELFLoader::ELFContainer::TYPE_X86_32) {
      Elf32_Phdr phdrs32[ehdr.e_phnum];

      if (pread(fd, phdrs32, sizeof(Elf32_Phdr) * ehdr.e_phnum, ehdr.e_phoff) == -1) {
        LogMan::Msg::E("Failed to read phdr32 from '%d'", fd);
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

      if (pread(fd, &phdrs[0], sizeof(Elf64_Phdr) * ehdr.e_phnum, ehdr.e_phoff) == -1) {
        LogMan::Msg::E("Failed to read phdr64 from '%d'", fd);
        return false;
      }
    }

    for (auto phdr : phdrs) {
      if (phdr.p_type == PT_INTERP) {
        InterpreterElf.resize(phdr.p_filesz);

        if (pread(fd, &InterpreterElf[0], phdr.p_filesz, phdr.p_offset) == -1) {
          LogMan::Msg::E("Failed to read interpreter from '%d'", fd);
          return false;
        }
      }
    }

    return true;
  }

  bool ReadElf(const std::string &file) {
    int NewFD = ::open(file.c_str(), O_RDONLY);

    return ReadElf(NewFD);
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
};
