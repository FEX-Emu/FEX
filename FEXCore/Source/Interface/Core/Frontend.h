// SPDX-License-Identifier: MIT
#pragma once

#include "Interface/Core/X86Tables/X86Tables.h"
#include "Interface/IR/IR.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CodeCache.h>
#include <FEXCore/Utils/ThreadPoolAllocator.h>
#include <FEXCore/fextl/set.h>
#include <FEXCore/fextl/vector.h>
#include <FEXCore/fextl/unordered_map.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace FEXCore::Context {
class ContextImpl;
}
namespace FEXCore::HLE {
enum class SyscallOSABI;
}

namespace FEXCore::Frontend {
class Decoder final {
public:
  enum class DecodedBlockStatus {
    SUCCESS,
    INVALID_INST,
    NOEXEC_INST,
    PARTIAL_DECODE_INST,
    BAD_RELOCATION,
  };

  // New Frontend decoding
  struct DecodedBlocks final {
    uint64_t Entry {};
    uint64_t Size {};
    uint64_t NumInstructions {};
    FEXCore::X86Tables::DecodedInst* DecodedInstructions;
    DecodedBlockStatus BlockStatus;
    bool IsEntryPoint {};
    bool ForceFullSMCDetection {};
  };

  struct DecodedBlockInformation final {
    uint64_t TotalInstructionCount;
    bool Is64BitMode {};
    fextl::vector<DecodedBlocks> Blocks;
    fextl::set<uint64_t> EntryPoints;
    fextl::set<uint64_t> CodePages; // Start addresses of all pages touching the block
  };

  Decoder(FEXCore::Core::InternalThreadState* Thread);
  void DecodeInstructionsAtEntry(FEXCore::Core::InternalThreadState* Thread, const uint8_t* InstStream, uint64_t PC, uint64_t MaxInst);

  const DecodedBlockInformation* GetDecodedBlockInfo() const {
    return &BlockInfo;
  }

  uint64_t DecodedMinAddress {};
  uint64_t DecodedMaxAddress {~0ULL};

  void SetExternalBranches(fextl::set<uint64_t>* v) {
    ExternalBranches = v;
  }

  void DelayedDisownBuffer() {
    PoolObject.DelayedDisownBuffer();
  }

  void ResetExecutableRangeCache() {
    ExecutableRangeBase = ExecutableRangeEnd = 0;
  }

private:
  // To pass any information from instruction prefixes
  // down into the actual instruction handling machinery.
  struct DecodedHeader {
    uint8_t vvvv; // Encoded operand in a VEX prefix.
    bool w;       // VEX.W bit.
    bool L;       // VEX.L bit (if set then 256 bit operation, if unset then scalar or 128-bit operation)
  };

  FEXCore::Core::InternalThreadState* Thread;
  FEXCore::Context::ContextImpl* CTX;
  const FEXCore::HLE::SyscallOSABI OSABI {};

  FEX_CONFIG_OPT(EnableCodeCacheValidation, ENABLECODECACHEVALIDATION);

  bool DecodeInstructionImpl(uint64_t PC);
  DecodedBlockStatus DecodeInstruction(uint64_t PC);

  void BranchTargetInMultiblockRange();
  bool IsBranchMonoTailcall(uint64_t NumInstructions) const;
  bool InstCanContinue() const;

  void AddBranchTarget(uint64_t Target);

  bool CheckRangeExecutable(uint64_t Address, uint64_t Size);

  uint8_t ReadByte();
  std::optional<uint8_t> PeekByte(uint8_t Offset);
  std::pair<uint64_t, bool> ReadData(uint8_t Size);

  void SkipBytes(uint8_t Size) {
    InstructionSize += Size;
  }

  bool NormalOp(const FEXCore::X86Tables::X86InstInfo* Info, uint16_t Op, DecodedHeader Options = {});
  bool NormalOpHeader(const FEXCore::X86Tables::X86InstInfo* Info, uint16_t Op);

  static constexpr size_t DefaultDecodedBufferSize = 0x10000;
  FEXCore::X86Tables::DecodedInst* DecodedBuffer {};
  Utils::PoolBufferWithTimedRetirement<FEXCore::X86Tables::DecodedInst*, 5000, 500> PoolObject;
  size_t DecodedSize {};

  uint64_t ExecutableRangeBase {};
  uint64_t ExecutableRangeEnd {};
  bool ExecutableRangeWritable {};
  bool HitNonExecutableRange {};
  bool HitBadRelocation {};

  const uint8_t* InstStream {};
  IR::OpSize GetGPROpSize() const {
    return BlockInfo.Is64BitMode ? IR::OpSize::i64Bit : IR::OpSize::i32Bit;
  }

  static constexpr size_t MAX_INST_SIZE = 15;
  uint8_t InstructionSize {};
  std::array<uint8_t, MAX_INST_SIZE> Instruction;
  uint8_t LastEscapePrefix {};
  FEXCore::X86Tables::DecodedInst* DecodeInst;

  // This is for multiblock data tracking
  uint64_t EntryPoint {};
  uint64_t MaxCondBranchForward {};
  uint64_t MaxCondBranchBackwards {~0ULL};
  uint64_t SectionMaxAddress {~0ULL};
  uint64_t SectionMinAddress {};
  uint64_t NextBlockStartAddress {~0ULL};

  DecodedBlockInformation BlockInfo;
  fextl::set<uint64_t> CurrentBlockTargets;
  fextl::set<uint64_t> BlocksToDecode;
  fextl::set<uint64_t> VisitedBlocks;
  fextl::set<uint64_t>* ExternalBranches {nullptr};

  fextl::unordered_map<uint32_t, GuestRelocationType>* Relocations {nullptr};

  // ModRM rm decoding
  using DecodeModRMPtr = void (FEXCore::Frontend::Decoder::*)(X86Tables::DecodedOperand* Operand, X86Tables::ModRMDecoded ModRM);
  void DecodeModRM_16(X86Tables::DecodedOperand* Operand, X86Tables::ModRMDecoded ModRM);
  void DecodeModRM_64(X86Tables::DecodedOperand* Operand, X86Tables::ModRMDecoded ModRM);

  static constexpr std::array<DecodeModRMPtr, 2> DecodeModRMs_Disp {
    &FEXCore::Frontend::Decoder::DecodeModRM_64,
    &FEXCore::Frontend::Decoder::DecodeModRM_16,
  };

  const std::array<X86Tables::X86InstInfo, X86Tables::MAX_X87_TABLE_SIZE>* X87Table;

  const std::array<X86Tables::X86InstInfo, X86Tables::MAX_VEX_TABLE_SIZE>* VEXTable {};
  const std::array<X86Tables::X86InstInfo, X86Tables::MAX_VEX_GROUP_TABLE_SIZE>* VEXTableGroup {};

  const uint8_t* AdjustAddrForSpecialRegion(const uint8_t* _InstStream, uint64_t EntryPoint, uint64_t RIP);
};
} // namespace FEXCore::Frontend
