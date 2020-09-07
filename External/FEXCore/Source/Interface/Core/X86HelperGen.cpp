#include "Interface/Core/X86HelperGen.h"

#include <cstring>
#include <stdlib.h>
#include <vector>

namespace FEXCore {
X86GeneratedCode::X86GeneratedCode() {
  // Allocate a page for our emulated guest
  CodePtr = malloc(0x1000);

  SignalReturn = reinterpret_cast<uint64_t>(CodePtr);
  CallbackReturn = reinterpret_cast<uint64_t>(CodePtr) + 2;

  const std::vector<uint8_t> SignalReturnCode = {
    0x0F, 0x36, // SIGRET FEX instruction
    0x0F, 0x37, // CALLBACKRET FEX Instruction
  };

  memcpy(CodePtr, &SignalReturnCode.at(0), SignalReturnCode.size());
}

X86GeneratedCode::~X86GeneratedCode() {
  free(CodePtr);
}

}

