// SPDX-License-Identifier: MIT
#include <catch2/catch_test_macros.hpp>

#include <elf.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

void Masquerade(int fd) {
  Elf64_Ehdr Header {
    .e_ident =
      {
        ELFMAG0,
        ELFMAG1,
        ELFMAG2,
        ELFMAG3,
        ELFCLASS64,
        ELFDATA2LSB,
        EV_CURRENT,
        ELFOSABI_NONE,
      },
    .e_type = ET_NONE,
    .e_machine = EM_X86_64,
    .e_version = 0,
    .e_entry = 0,
    .e_phoff = 0,
    .e_shoff = 0,
    .e_flags = 0,
    .e_ehsize = sizeof(Elf64_Ehdr),
    .e_phentsize = sizeof(Elf64_Phdr),
    .e_phnum = 1,
    .e_shentsize = sizeof(Elf64_Shdr),
    .e_shnum = 0,
    .e_shstrndx = 0,
  };
  REQUIRE(write(fd, &Header, sizeof(Header)) == sizeof(Header));

  // Put a "function" at 0x1000.
  REQUIRE(lseek(fd, 0x1000, SEEK_SET) == 0x1000);

  char ret = 0xc3;
  REQUIRE(write(fd, &ret, 1) == 1);
}

TEST_CASE("Masquerade as an ELF") {
  int fd = memfd_create("Test", MFD_CLOEXEC);
  REQUIRE(fd != -1);

  constexpr int USER_PERMS = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH | S_IXUSR;
  REQUIRE(fchmod(fd, USER_PERMS) != -1);
  REQUIRE(ftruncate(fd, 0x1000 * 2) != -1);

  Masquerade(fd);

  // Map the base first.
  auto ptr = mmap(nullptr, 0x10000, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE, fd, 0);
  REQUIRE(ptr != MAP_FAILED);

  // Now map the second page.
  auto ptr2 = mmap(nullptr, 0x10000, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE, fd, 0x1000);
  REQUIRE(ptr2 != MAP_FAILED);

  // Try and execute the code for fun and profit to make sure it works.
  using func_type = void (*)();
  auto func = reinterpret_cast<func_type>(ptr2);
  func();

  close(fd);
}
