// SPDX-License-Identifier: MIT
#pragma once

#include "Interface/Core/X86Tables/X86Tables.h"

#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/Utils/Telemetry.h>
#include <FEXCore/fextl/set.h>
#include <FEXCore/fextl/vector.h>

#include <array>
#include <cstdint>
#include <stddef.h>

namespace FEXCore::Context {
class ContextImpl;
}

namespace FEXCore::Frontend {
class Decoder final {
public:
  // New Frontend decoding
  struct DecodedBlocks final {
    uint64_t Entry {};
    uint64_t Size {};
    uint64_t NumInstructions {};
    FEXCore::X86Tables::DecodedInst* DecodedInstructions;
    bool HasInvalidInstruction {};
    bool IsEntryPoint {};
  };

  struct DecodedBlockInformation final {
    uint64_t TotalInstructionCount;
    fextl::vector<DecodedBlocks> Blocks;
  };

  Decoder(FEXCore::Context::ContextImpl* ctx);
  void DecodeInstructionsAtEntry(const uint8_t* InstStream, uint64_t PC, uint64_t MaxInst,
                                 std::function<void(const fextl::vector<uint64_t>& BlockEntryPoints, uint64_t Start, uint64_t Length)> AddContainedCodePage);

  const DecodedBlockInformation* GetDecodedBlockInfo() const {
    return &BlockInfo;
  }

  uint64_t DecodedMinAddress {};
  uint64_t DecodedMaxAddress {~0ULL};

  void SetSectionMaxAddress(uint64_t v) {
    SectionMaxAddress = v;
  }
  void SetExternalBranches(fextl::set<uint64_t>* v) {
    ExternalBranches = v;
  }

  void DelayedDisownBuffer() {
    PoolObject.DelayedDisownBuffer();
  }

private:
  // To pass any information from instruction prefixes
  // down into the actual instruction handling machinery.
  struct DecodedHeader {
    uint8_t vvvv; // Encoded operand in a VEX prefix.
    bool w;       // VEX.W bit.
    bool L;       // VEX.L bit (if set then 256 bit operation, if unset then scalar or 128-bit operation)
  };

  FEXCore::Context::ContextImpl* CTX;
  const FEXCore::HLE::SyscallOSABI OSABI {};

  bool DecodeInstruction(uint64_t PC);

  void BranchTargetInMultiblockRange();
  bool InstCanContinue() const;

  void AddBranchTarget(uint64_t Target);

  uint8_t ReadByte();
  uint8_t PeekByte(uint8_t Offset) const;
  uint64_t ReadData(uint8_t Size);
  void SkipBytes(uint8_t Size) {
    InstructionSize += Size;
  }

  bool NormalOp(const FEXCore::X86Tables::X86InstInfo* Info, uint16_t Op, DecodedHeader Options = {});
  bool NormalOpHeader(const FEXCore::X86Tables::X86InstInfo* Info, uint16_t Op);

  static constexpr size_t DefaultDecodedBufferSize = 0x10000;
  FEXCore::X86Tables::DecodedInst* DecodedBuffer {};
  Utils::PoolBufferWithTimedRetirement<FEXCore::X86Tables::DecodedInst*, 5000, 500> PoolObject;
  size_t DecodedSize {};

  const uint8_t* InstStream {};

  static constexpr size_t MAX_INST_SIZE = 15;
  uint8_t InstructionSize {};
  std::array<uint8_t, MAX_INST_SIZE> Instruction;
  FEXCore::X86Tables::DecodedInst* DecodeInst;

  // This is for multiblock data tracking
  bool SymbolAvailable {false};
  uint64_t EntryPoint {};
  uint64_t MaxCondBranchForward {};
  uint64_t MaxCondBranchBackwards {~0ULL};
  uint64_t SymbolMaxAddress {};
  uint64_t SymbolMinAddress {~0ULL};
  uint64_t SectionMaxAddress {~0ULL};
  uint64_t NextBlockStartAddress {~0ULL};

  DecodedBlockInformation BlockInfo;
  fextl::set<uint64_t> CurrentBlockTargets;
  fextl::set<uint64_t> BlocksToDecode;
  fextl::set<uint64_t> VisitedBlocks;
  fextl::set<uint64_t>* ExternalBranches {nullptr};
  fextl::set<uint64_t> BlockEntryPoints;

  // ModRM rm decoding
  using DecodeModRMPtr = void (FEXCore::Frontend::Decoder::*)(X86Tables::DecodedOperand* Operand, X86Tables::ModRMDecoded ModRM);
  void DecodeModRM_16(X86Tables::DecodedOperand* Operand, X86Tables::ModRMDecoded ModRM);
  void DecodeModRM_64(X86Tables::DecodedOperand* Operand, X86Tables::ModRMDecoded ModRM);

  static constexpr std::array<DecodeModRMPtr, 2> DecodeModRMs_Disp {
    &FEXCore::Frontend::Decoder::DecodeModRM_64,
    &FEXCore::Frontend::Decoder::DecodeModRM_16,
  };

  const uint8_t* AdjustAddrForSpecialRegion(const uint8_t* _InstStream, uint64_t EntryPoint, uint64_t RIP);
};
} // namespace FEXCore::Frontend
