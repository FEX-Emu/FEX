// SPDX-License-Identifier: MIT
#include <catch2/catch_all.hpp>
#include <Common/FileMappingBaseAddress.h>

#include <FEXCore/fextl/vector.h>

namespace {

struct Mapping {
  uint64_t Addr;
  uint64_t Size;
  uint64_t FileOffset;
  int Flags; // PF_*
};

} // anonymous namespace

TEST_CASE("libm") {
  uint64_t BaseAddr = 0x123400000;

  fextl::vector<Elf64_Phdr> Headers = {
    {.p_type = PT_LOAD, .p_offset = 0x00000, .p_vaddr = 0x00000, .p_paddr = 0x00000, .p_filesz = 0x7bdd5, .p_memsz = 0x7bdd5},
    {.p_type = PT_LOAD, .p_offset = 0x7c000, .p_vaddr = 0x7c000, .p_paddr = 0x7c000, .p_filesz = 0x6f3a8, .p_memsz = 0x6f3a8},
    {.p_type = PT_LOAD, .p_offset = 0xebbd0, .p_vaddr = 0xecbd0, .p_paddr = 0xecbd0, .p_filesz = 0x434, .p_memsz = 0x440},
  };

  fextl::vector<Mapping> Mappings = {
    {.Addr = BaseAddr, .Size = 0x7c000, .FileOffset = 0x00000},
    {.Addr = BaseAddr + 0x7c000, .Size = 0x70000, .FileOffset = 0x7c000},
    {.Addr = BaseAddr + 0xec000, .Size = 0x2000, .FileOffset = 0xeb000},
  };

  for (auto& Mapping : Mappings) {
    INFO("Mapping to 0x" << std::hex << Mapping.Addr << "-0x" << Mapping.Addr + Mapping.Size << " from file offset 0x" << Mapping.FileOffset);
    auto DeducedBase = FEXCore::InferMappingBaseAddress(Headers, Mapping.Addr, Mapping.Size, Mapping.FileOffset, Mapping.Flags);
    CHECK(DeducedBase == fextl::vector<uint64_t> {BaseAddr});
  }
}

// E.g. libX11-xcb
TEST_CASE("Access flags are checked") {
  uint64_t BaseAddr = 0x123400000;

  fextl::vector<Elf64_Phdr> Headers = {
    {.p_type = PT_LOAD, .p_flags = PF_R | PF_X, .p_offset = 0x0000, .p_vaddr = 0x0000, .p_paddr = 0x0000, .p_filesz = 0x00040d, .p_memsz = 0x00040d},
    {.p_type = PT_LOAD, .p_flags = PF_R, .p_offset = 0x1000, .p_vaddr = 0x1000, .p_paddr = 0x1000, .p_filesz = 0x00036c, .p_memsz = 0x00036c},
    {.p_type = PT_LOAD, .p_flags = PF_W, .p_offset = 0x1dc8, .p_vaddr = 0x2dc8, .p_paddr = 0x2dc8, .p_filesz = 0x000238, .p_memsz = 0x000240},
  };

  fextl::vector<Mapping> Mappings = {
    {.Addr = BaseAddr + 0x1000, .Size = 0x1000, .FileOffset = 0x1000, .Flags = PF_R},
    {.Addr = BaseAddr + 0x2000, .Size = 0x1000, .FileOffset = 0x1000, .Flags = PF_W},
  };

  for (auto& Mapping : Mappings) {
    INFO("Mapping to 0x" << std::hex << Mapping.Addr << "-0x" << Mapping.Addr + Mapping.Size << " from file offset 0x" << Mapping.FileOffset);
    auto DeducedBase = FEXCore::InferMappingBaseAddress(Headers, Mapping.Addr, Mapping.Size, Mapping.FileOffset, Mapping.Flags);
    CHECK(DeducedBase == fextl::vector<uint64_t> {BaseAddr});
  }
}

// Program headers that don't generate memory mappings can't be used to infer base addresses
TEST_CASE("Non-mapping program headers are ignored") {
  uint64_t BaseAddr = 0x123400000;

  fextl::vector<Elf64_Phdr> Headers = {
    {.p_type = PT_LOAD, .p_offset = 0x00000, .p_vaddr = 0x0000, .p_paddr = 0x00000, .p_filesz = 0x1000, .p_memsz = 0x1000},
    {.p_type = PT_INTERP, .p_offset = 0x10000, .p_vaddr = 0xa000, .p_paddr = 0xa0000, .p_filesz = 0x1000, .p_memsz = 0x1000},
    {.p_type = PT_LOAD, .p_offset = 0x10000, .p_vaddr = 0x1000, .p_paddr = 0x10000, .p_filesz = 0x1000, .p_memsz = 0x1000},
  };

  fextl::vector<Mapping> Mappings = {
    {.Addr = BaseAddr + 0x1000, .Size = 0x1000, .FileOffset = 0x10000},
  };

  for (auto& Mapping : Mappings) {
    INFO("Mapping to 0x" << std::hex << Mapping.Addr << "-0x" << Mapping.Addr + Mapping.Size << " from file offset 0x" << Mapping.FileOffset);
    auto DeducedBase = FEXCore::InferMappingBaseAddress(Headers, Mapping.Addr, Mapping.Size, Mapping.FileOffset, Mapping.Flags);
    CHECK(DeducedBase == fextl::vector<uint64_t> {BaseAddr});
  }
}

// Some binaries have (e.g. glxtest) end up with two RW mappings at the end that trigger two mmap parameters that only differ in their
// virtual address. In such cases, multiple base addresses could be valid
TEST_CASE("Duplicate data page") {
  uint64_t BaseAddr = 0x123400000;

  fextl::vector<Elf64_Phdr> Headers = {
    {.p_type = PT_LOAD, .p_flags = PF_R, .p_offset = 0x00000, .p_vaddr = 0x0000, .p_paddr = 0x00000, .p_filesz = 0x1000, .p_memsz = 0x1000},
    {.p_type = PT_LOAD, .p_flags = PF_R | PF_X, .p_offset = 0x3910, .p_vaddr = 0x4910, .p_paddr = 0x4910, .p_filesz = 0x29d0, .p_memsz = 0x29d0},
    {.p_type = PT_LOAD, .p_flags = PF_R | PF_W, .p_offset = 0x62e0, .p_vaddr = 0x82e0, .p_paddr = 0x82e0, .p_filesz = 0x6e0, .p_memsz = 0xd20},
    {.p_type = PT_LOAD, .p_flags = PF_R | PF_W, .p_offset = 0x69c0, .p_vaddr = 0x99c0, .p_paddr = 0x99c0, .p_filesz = 0x018, .p_memsz = 0x068},
  };

  fextl::vector<Mapping> Mappings = {
    {.Addr = BaseAddr, .Size = 0x4000, .FileOffset = 0x0000, .Flags = PF_R},
    {.Addr = BaseAddr + 0x4000, .Size = 0x4000, .FileOffset = 0x3000, .Flags = PF_R | PF_X},
    {.Addr = BaseAddr + 0x8000, .Size = 0x1000, .FileOffset = 0x6000, .Flags = PF_R | PF_W},
    {.Addr = BaseAddr + 0x9000, .Size = 0x1000, .FileOffset = 0x6000, .Flags = PF_R | PF_W},
  };

  for (auto& Mapping : Mappings) {
    INFO("Mapping to 0x" << std::hex << Mapping.Addr << "-0x" << Mapping.Addr + Mapping.Size << " from file offset 0x" << Mapping.FileOffset);
    auto DeducedBase = FEXCore::InferMappingBaseAddress(Headers, Mapping.Addr, Mapping.Size, Mapping.FileOffset, Mapping.Flags);
    if (Mapping.Addr < BaseAddr + 0x8000) {
      CHECK(DeducedBase == fextl::vector<uint64_t> {BaseAddr});
    } else {
      CHECK(DeducedBase == fextl::vector<uint64_t> {Mapping.Addr - 0x8000, Mapping.Addr - 0x9000});
    }
  }
}
