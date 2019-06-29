#pragma once

#include <FEXCore/Debug/X86Tables.h>
#include <cstdint>
#include <utility>
#include <set>
#include <vector>

namespace FEXCore::Context {
struct Context;
}

namespace FEXCore::Frontend {
class Decoder {
public:
  Decoder(FEXCore::Context::Context *ctx);
  bool DecodeInstructionsInBlock(uint8_t const* InstStream, uint64_t PC);

  std::pair<std::vector<FEXCore::X86Tables::DecodedInst>*, size_t> const GetDecodedInsts() {
    return std::make_pair(&DecodedBuffer, DecodedSize);
  }
  std::set<uint64_t> JumpTargets;

private:
  FEXCore::Context::Context *CTX;

  bool DecodeInstruction(uint8_t const *InstStream, uint64_t PC);

  bool BlockEndCanContinuePast(FEXCore::X86Tables::DecodedInst const &Inst);
  bool BranchTargetInMultiblockRange(FEXCore::X86Tables::DecodedInst const &Inst);
  static constexpr size_t DefaultDecodedBufferSize = 0x10000;
  std::vector<FEXCore::X86Tables::DecodedInst> DecodedBuffer;
  size_t DecodedSize {};

  // This is for multiblock data tracking
  bool SymbolAvailable {false};
  uint64_t EntryPoint {};
  uint64_t MaxCondBranchForward {};
  uint64_t MaxCondBranchBackwards {~0ULL};
  uint64_t SymbolMaxAddress {};
  uint64_t SymbolMinAddress {~0ULL};

};
}
