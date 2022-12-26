#pragma once
#include "Interface/Core/ArchHelpers/CodeEmitter/Emitter.h"

#include <aarch64/cpu-aarch64.h>
#include <aarch64/instructions-aarch64.h>
#include <aarch64/disasm-aarch64.h>

#include <sys/mman.h>

class TestDisassembler : public FEXCore::ARMEmitter::Emitter  {
public:
  TestDisassembler() {
    fp = tmpfile();
    Disasm = std::make_unique<vixl::aarch64::PrintDisassembler>(fp);
    SetBuffer(reinterpret_cast<uint8_t*>(mmap(nullptr, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)), 4096);
    BufferBegin = GetCursorAddress<const vixl::aarch64::Instruction*>();
  }
  ~TestDisassembler() {
    fclose(fp);
  }

  std::string DisassembleSingle() {
    HandleDisasm();
    char Tmp[512];
    uint64_t Addr;
    uint32_t Encoding;
    int Num = fscanf(fp, "0x%lx %x %[^\n]\n", &Addr, &Encoding, Tmp);
    if (Num != 3) {
      return "<Invalid>";
    }
    ResetFP();

    return Tmp;
  }

  uint32_t DisassembleEncoding(size_t Offset = 0) {
    const uint32_t *Values = reinterpret_cast<const uint32_t*>(GetBufferBase());
    SetCursorOffset(0);
    ResetFP();
    return Values[Offset];
  }

  std::string DisassembleString() {
    HandleDisasm();
    std::string Decoded{};
    char Tmp[512];
    uint64_t Addr;
    uint32_t Encoding;
    while (fscanf(fp, "0x%lx %x %[^\n]\n", &Addr, &Encoding, Tmp) == 3) {
      Decoded += std::string_view(Tmp);
      Decoded += "\n";
    }

    ResetFP();
    return Decoded;
  }
private:
  void HandleDisasm() {
    const auto BufferEnd = GetCursorAddress<const vixl::aarch64::Instruction*>();
    Disasm->DisassembleBuffer(BufferBegin, BufferEnd);
    SetCursorOffset(0);
    fseek(fp, 0, SEEK_SET);
  }
  void ResetFP() {
    fseek(fp, 0, SEEK_SET);
  }
  FILE *fp;
  const vixl::aarch64::Instruction* BufferBegin;
  std::unique_ptr<vixl::aarch64::PrintDisassembler> Disasm;
};

#define TEST_SINGLE(emit, expected) \
{ \
  CHECK((emit, DisassembleSingle()) == expected); \
}
