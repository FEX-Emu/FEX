#pragma once
#include <memory>

namespace FEX::Debugger {

class Disassembler {
public:
  virtual ~Disassembler() {}
  virtual std::string Disassemble(uint8_t *Code, uint32_t CodeSize, uint32_t MaxInst, uint64_t StartingPC, uint32_t *InstructionCount) = 0;
};

std::unique_ptr<Disassembler> CreateHostDisassembler();
std::unique_ptr<Disassembler> CreateGuestDisassembler();

}
