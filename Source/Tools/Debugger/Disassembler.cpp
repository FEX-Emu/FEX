#include "Disassembler.h"
#include <llvm-c/Disassembler.h>
#include <llvm-c/Target.h>
#include <memory>
#include <sstream>
#include <iomanip>

namespace FEX::Debugger {

class LLVMDisassembler final : public Disassembler {
public:
  ~LLVMDisassembler() override {
    LLVMDisasmDispose(LLVMContext);
  }
  explicit LLVMDisassembler(const char *Arch);
  std::string Disassemble(uint8_t *Code, uint32_t CodeSize, uint32_t MaxInst, uint64_t StartingPC, uint32_t *InstructionCount) override;

private:
  LLVMDisasmContextRef LLVMContext;

};

LLVMDisassembler::LLVMDisassembler(const char *Arch) {
  LLVMInitializeAllTargetInfos();
  LLVMInitializeAllTargetMCs();
  LLVMInitializeAllDisassemblers();

  LLVMContext = LLVMCreateDisasmCPU(Arch, "", nullptr, 0, nullptr, nullptr);

  if (!LLVMContext)
    return;

  LLVMSetDisasmOptions(LLVMContext, LLVMDisassembler_Option_AsmPrinterVariant |
                                           LLVMDisassembler_Option_PrintLatency);
}

std::string LLVMDisassembler::Disassemble(uint8_t *Code, uint32_t CodeSize, uint32_t MaxInst, uint64_t StartingPC, uint32_t *InstructionCount) {
  std::ostringstream Output;

  uint8_t *CurrentRIPAddr = Code;
  uint8_t *EndRIPAddr = Code + CodeSize;
  uint64_t CurrentRIP = StartingPC;
  uint32_t NumberOfInstructions = 0;

  while (CurrentRIPAddr <= EndRIPAddr) {
    char OutputText[128];
    size_t InstSize = LLVMDisasmInstruction(LLVMContext,
        CurrentRIPAddr,
        static_cast<uint64_t>(EndRIPAddr - CurrentRIPAddr),
        CurrentRIP,
        OutputText,
        128);

    Output << "0x" << std::hex << CurrentRIP << ": ";
    if (!InstSize) {
      Output <<  "<Invalid Inst>" << std::endl;
      break;
    }
    else {
      // Print instruction hex encodings if we want
      if (!true) {
        for (size_t i = 0; i < InstSize; ++i) {
          Output << std::setw(2) << std::setfill('0') << static_cast<uint32_t>(CurrentRIPAddr[i]);
          if ((i + 1) == InstSize) {
            Output << ": ";
          }
          else {
            Output << " ";
          }
        }
      }
      Output << OutputText << std::endl;
    }

    CurrentRIP += InstSize;
    CurrentRIPAddr += InstSize;
    NumberOfInstructions++;
    if (NumberOfInstructions >= MaxInst) {
      break;
    }
  }

  *InstructionCount = NumberOfInstructions;
  return Output.str();
}

std::unique_ptr<Disassembler> CreateHostDisassembler() {
  return std::make_unique<LLVMDisassembler>("x86_64-none-unknown");
}
std::unique_ptr<Disassembler> CreateGuestDisassembler() {
  return std::make_unique<LLVMDisassembler>("x86_64-none-unknown");
}

}
