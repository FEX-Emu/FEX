#pragma once
#include <vector>
#include <string>
#include <elf.h>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>

#include "ELFContainer.h"

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

  bool ReadElf(const std::string &file) {
    Closefd();
    static_assert(EI_CLASS == 4);

    type = ::ELFLoader::ELFContainer::TYPE_NONE;

    std::ifstream elf(file);

    fd = ::open(file.c_str(), O_RDONLY);
    
    if (fd == -1)
      return false;

    if (!elf.good())
      return false;

    uint8_t header[5];
    elf.read((char*)header, sizeof(header));

    if (!elf.good())
      return false;

    if (header[0] != ELFMAG0 || header[1] != ELFMAG1 || header[2] != ELFMAG2 || header[3] != ELFMAG3) {
      return false;
    }

    type = ::ELFLoader::ELFContainer::TYPE_OTHER_ELF;

    // go to the beggining of the file
    elf.seekg(0);

    if (header[EI_CLASS] == ELFCLASS32) {
      Elf32_Ehdr hdr32;
      elf.read((char*)&hdr32, sizeof(hdr32));

      if (!elf.good())
        return false;

      // do the sizes match up as expected?

      // check elf header
      if (hdr32.e_ehsize != sizeof(hdr32))
        return false;

      // check program header
      if (hdr32.e_phentsize != sizeof(Elf32_Phdr))
        return false;
      
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

      if (ehdr.e_machine != EM_386)
        return false;

      type = ::ELFLoader::ELFContainer::TYPE_X86_32;

      if (ehdr.e_phnum < 1 || ehdr.e_phnum > 65536 / sizeof(Elf32_Phdr))
        return false;
    } else if (header[EI_CLASS] == ELFCLASS64) {
      elf.read((char*)&ehdr, sizeof(ehdr));
      if (!elf.good())
        return false;


      // do the sizes match up as expected?

      // check elf header
      if (ehdr.e_ehsize != sizeof(ehdr))
        return false;

      // check program header
      if (ehdr.e_phentsize != sizeof(Elf64_Phdr))
        return false;

      if (ehdr.e_machine != EM_X86_64)
        return false;

      type = ::ELFLoader::ELFContainer::TYPE_X86_64;

    } else {
      // Unexpected elf type
      return false;
    }

    // seek to the program header offset
    elf.seekg(ehdr.e_phoff);

    // sanity check program header count
    if (ehdr.e_phnum < 1 || ehdr.e_phnum > 65536 / ehdr.e_phentsize)
      return false;

    if (type == ::ELFLoader::ELFContainer::TYPE_X86_32) {
      Elf32_Phdr phdrs32[ehdr.e_phnum];
      elf.read((char*)phdrs32, sizeof(Elf32_Phdr) * ehdr.e_phnum);
      
      if (!elf.good())
        return false;
      
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

      elf.read((char*)&phdrs[0], sizeof(Elf64_Phdr) * ehdr.e_phnum);
      
      if (!elf.good())
        return false;
    }
    
    for (auto phdr : phdrs) {
      if (phdr.p_type == PT_INTERP) {
        elf.seekg(phdr.p_offset);
        InterpreterElf.resize(phdr.p_filesz);

        elf.read(&InterpreterElf[0], phdr.p_filesz);

        if (!elf.good())
          return false;
      }
    }

    return true;
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