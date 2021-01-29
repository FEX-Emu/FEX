#pragma once

#include <FEXCore/Debug/X86Tables.h>
#include <array>
#include <cstdint>
#include <utility>
#include <set>
#include <stack>
#include <vector>

namespace FEXCore::Context {
struct Context;
}

namespace FEXCore::Frontend {
class Decoder final {
public:
  // New Frontend decoding
  struct DecodedBlocks final {
    uint64_t Entry{};
    uint64_t NumInstructions{};
    FEXCore::X86Tables::DecodedInst *DecodedInstructions;
    bool HasInvalidInstruction{};
  };

  Decoder(FEXCore::Context::Context *ctx);
  bool DecodeInstructionsAtEntry(uint8_t const* InstStream, uint64_t PC);

  std::vector<DecodedBlocks> const *GetDecodedBlocks() {
    return &Blocks;
  }

  uint64_t DecodedMinAddress {};
  uint64_t DecodedMaxAddress {~0ULL};
  
private:
  FEXCore::Context::Context *CTX;

  bool DecodeInstruction(uint64_t PC);

  void BranchTargetInMultiblockRange();

  uint8_t ReadByte();
  uint8_t PeekByte(uint8_t Offset);
  uint64_t ReadData(uint8_t Size);
  void SkipBytes(uint8_t Size) { InstructionSize += Size; }
  bool NormalOp(FEXCore::X86Tables::X86InstInfo const *Info, uint16_t Op);
  bool NormalOpHeader(FEXCore::X86Tables::X86InstInfo const *Info, uint16_t Op);

  static constexpr size_t DefaultDecodedBufferSize = 0x10000;
  std::vector<FEXCore::X86Tables::DecodedInst> DecodedBuffer;
  size_t DecodedSize {};

  uint8_t const *InstStream;

  static constexpr size_t MAX_INST_SIZE = 15;
  uint8_t InstructionSize;
  std::array<uint8_t, MAX_INST_SIZE> Instruction;
  FEXCore::X86Tables::DecodedInst *DecodeInst;

  // This is for multiblock data tracking
  bool SymbolAvailable {false};
  uint64_t EntryPoint {};
  uint64_t MaxCondBranchForward {};
  uint64_t MaxCondBranchBackwards {~0ULL};
  uint64_t SymbolMaxAddress {};
  uint64_t SymbolMinAddress {~0ULL};

  std::vector<DecodedBlocks> Blocks;
  std::set<uint64_t> BlocksToDecode;
  std::set<uint64_t> HasBlocks;

  // ModRM rm decoding
  using DecodeModRMPtr = void (FEXCore::Frontend::Decoder::*)(X86Tables::DecodedOperand *Operand, X86Tables::ModRMDecoded ModRM);
  void DecodeModRM_16(X86Tables::DecodedOperand *Operand, X86Tables::ModRMDecoded ModRM);
  void DecodeModRM_64(X86Tables::DecodedOperand *Operand, X86Tables::ModRMDecoded ModRM);

  const std::array<DecodeModRMPtr, 2> DecodeModRMs_Disp {
    &FEXCore::Frontend::Decoder::DecodeModRM_64,
    &FEXCore::Frontend::Decoder::DecodeModRM_16,
  };
};
}
