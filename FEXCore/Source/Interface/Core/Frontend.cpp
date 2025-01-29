// SPDX-License-Identifier: MIT
/*
$info$
tags: frontend|x86-meta-blocks
desc: Extracts instruction & block meta info, frontend multiblock logic
$end_info$
*/

#include "Interface/Context/Context.h"
#include "Interface/Core/Frontend.h"
#include "Interface/Core/X86Tables/X86Tables.h"

#include <array>
#include <algorithm>
#include <cstring>
#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/Profiler.h>
#include <FEXCore/Utils/Telemetry.h>
#include <FEXCore/Utils/TypeDefines.h>
#include <FEXCore/fextl/set.h>

namespace FEXCore::Frontend {
#include "Interface/Core/VSyscall/VSyscall.inc"

using namespace FEXCore::X86Tables;

static uint32_t MapModRMToReg(uint8_t REX, uint8_t bits, bool HighBits, bool HasREX, bool HasXMM, bool HasMM, uint8_t InvalidOffset = 16) {
  using GPRArray = std::array<uint32_t, 16>;

  static constexpr GPRArray GPR8BitHighIndexes = {
    // Classical ordering?
    FEXCore::X86State::REG_RAX, FEXCore::X86State::REG_RCX, FEXCore::X86State::REG_RDX, FEXCore::X86State::REG_RBX,
    FEXCore::X86State::REG_RAX, FEXCore::X86State::REG_RCX, FEXCore::X86State::REG_RDX, FEXCore::X86State::REG_RBX,
    FEXCore::X86State::REG_R8,  FEXCore::X86State::REG_R9,  FEXCore::X86State::REG_R10, FEXCore::X86State::REG_R11,
    FEXCore::X86State::REG_R12, FEXCore::X86State::REG_R13, FEXCore::X86State::REG_R14, FEXCore::X86State::REG_R15,
  };

  uint8_t Offset = (REX << 3) | bits;

  if (Offset == InvalidOffset) {
    return FEXCore::X86State::REG_INVALID;
  }

  if (HasXMM) {
    return FEXCore::X86State::REG_XMM_0 + Offset;
  } else if (HasMM) {
    return FEXCore::X86State::REG_MM_0 + Offset;
  } else if (!(HighBits && !HasREX)) {
    return FEXCore::X86State::REG_RAX + Offset;
  }

  return GPR8BitHighIndexes[Offset];
}

static uint32_t MapVEXToReg(uint8_t vvvv, bool HasXMM) {
  if (HasXMM) {
    return FEXCore::X86State::REG_XMM_0 + vvvv;
  } else {
    return FEXCore::X86State::REG_RAX + vvvv;
  }
}

Decoder::Decoder(FEXCore::Context::ContextImpl* ctx)
  : CTX {ctx}
  , OSABI {ctx->SyscallHandler ? ctx->SyscallHandler->GetOSABI() : FEXCore::HLE::SyscallOSABI::OS_UNKNOWN}
  , PoolObject {ctx->FrontendAllocator, sizeof(FEXCore::X86Tables::DecodedInst) * DefaultDecodedBufferSize} {}

Decoder::~Decoder() {
  PoolObject.UnclaimBuffer();
}

uint8_t Decoder::ReadByte() {
  uint8_t Byte = InstStream[InstructionSize];
  LOGMAN_THROW_A_FMT(InstructionSize < MAX_INST_SIZE, "Max instruction size exceeded!");
  Instruction[InstructionSize] = Byte;
  InstructionSize++;
  return Byte;
}

uint8_t Decoder::PeekByte(uint8_t Offset) const {
  uint8_t Byte = InstStream[InstructionSize + Offset];
  return Byte;
}

uint64_t Decoder::ReadData(uint8_t Size) {
  LOGMAN_THROW_A_FMT(Size != 0 && Size <= sizeof(uint64_t), "Unknown data size to read");

  uint64_t Res = 0;
  std::memcpy(&Res, &InstStream[InstructionSize], Size);

#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
  for (size_t i = 0; i < Size; ++i) {
    ReadByte();
  }
#else
  SkipBytes(Size);
#endif

  return Res;
}

void Decoder::DecodeModRM_16(X86Tables::DecodedOperand* Operand, X86Tables::ModRMDecoded ModRM) {
  // 16bit modrm behaves similar to SIB but encoded directly in modrm
  // mod != 0b11 case
  // RM    | Result
  // ===============
  // 0b000 | [BX + SI]
  // 0b001 | [BX + DI]
  // 0b010 | [BP + SI]
  // 0b011 | [BP + DI]
  // 0b100 | [SI]
  // 0b101 | [DI]
  // 0b110 | {[BP], disp16}
  // 0b111 | [BX]
  // if mod = 0b00
  //    0b110 = disp16
  // if mod = 0b01
  //    All encodings gain 8bit displacement
  //    0b110 = [BP] + disp8
  // if mod = 0b10
  //    All encodings gain 16bit displacement
  //    0b110 = [BP] + disp16
  uint32_t Literal {};
  uint8_t DisplacementSize {};
  if ((ModRM.mod == 0 && ModRM.rm == 0b110) || ModRM.mod == 0b10) {
    DisplacementSize = 2;
  } else if (ModRM.mod == 0b01) {
    DisplacementSize = 1;
  }
  if (DisplacementSize) {
    Literal = ReadData(DisplacementSize);
    if (DisplacementSize == 1) {
      Literal = static_cast<int8_t>(Literal);
    }
  }

  Operand->Type = DecodedOperand::OpType::SIB;
  Operand->Data.SIB.Scale = 1;
  Operand->Data.SIB.Offset = Literal;

  // Only called when ModRM.mod != 0b11
  struct Encodings {
    uint8_t Base;
    uint8_t Index;
  };
  constexpr static std::array<Encodings, 24> Lookup = {{
    // Mod = 0b00
    {FEXCore::X86State::REG_RBX, FEXCore::X86State::REG_RSI},
    {FEXCore::X86State::REG_RBX, FEXCore::X86State::REG_RDI},
    {FEXCore::X86State::REG_RBP, FEXCore::X86State::REG_RSI},
    {FEXCore::X86State::REG_RBP, FEXCore::X86State::REG_RDI},
    {FEXCore::X86State::REG_RSI, FEXCore::X86State::REG_INVALID},
    {FEXCore::X86State::REG_RDI, FEXCore::X86State::REG_INVALID},
    {FEXCore::X86State::REG_INVALID, FEXCore::X86State::REG_INVALID},
    {FEXCore::X86State::REG_RBX, FEXCore::X86State::REG_INVALID},
    // Mod = 0b01
    {FEXCore::X86State::REG_RBX, FEXCore::X86State::REG_RSI},
    {FEXCore::X86State::REG_RBX, FEXCore::X86State::REG_RDI},
    {FEXCore::X86State::REG_RBP, FEXCore::X86State::REG_RSI},
    {FEXCore::X86State::REG_RBP, FEXCore::X86State::REG_RDI},
    {FEXCore::X86State::REG_RSI, FEXCore::X86State::REG_INVALID},
    {FEXCore::X86State::REG_RDI, FEXCore::X86State::REG_INVALID},
    {FEXCore::X86State::REG_RBP, FEXCore::X86State::REG_INVALID},
    {FEXCore::X86State::REG_RBX, FEXCore::X86State::REG_INVALID},
    // Mod = 0b10
    {FEXCore::X86State::REG_RBX, FEXCore::X86State::REG_RSI},
    {FEXCore::X86State::REG_RBX, FEXCore::X86State::REG_RDI},
    {FEXCore::X86State::REG_RBP, FEXCore::X86State::REG_RSI},
    {FEXCore::X86State::REG_RBP, FEXCore::X86State::REG_RDI},
    {FEXCore::X86State::REG_RSI, FEXCore::X86State::REG_INVALID},
    {FEXCore::X86State::REG_RDI, FEXCore::X86State::REG_INVALID},
    {FEXCore::X86State::REG_RBP, FEXCore::X86State::REG_INVALID},
    {FEXCore::X86State::REG_RBX, FEXCore::X86State::REG_INVALID},
  }};

  uint8_t LookupIndex = ModRM.mod << 3 | ModRM.rm;
  auto it = Lookup[LookupIndex];
  Operand->Data.SIB.Base = it.Base;
  Operand->Data.SIB.Index = it.Index;
}

void Decoder::DecodeModRM_64(X86Tables::DecodedOperand* Operand, X86Tables::ModRMDecoded ModRM) {
  uint8_t Displacement {};
  // Do we have an offset?
  if (ModRM.mod == 0b01) {
    Displacement = 1;
  } else if (ModRM.mod == 0b10) {
    Displacement = 4;
  } else if (ModRM.mod == 0 && ModRM.rm == 0b101) {
    Displacement = 4;
  }

  // Calculate SIB
  bool HasSIB = ((ModRM.mod != 0b11) && (ModRM.rm == 0b100));

  if (HasSIB) {
    FEXCore::X86Tables::SIBDecoded SIB;
    if (DecodeInst->DecodedSIB) {
      SIB.Hex = DecodeInst->SIB;
    } else {
      // Haven't yet grabbed SIB, pull it now
      DecodeInst->SIB = ReadByte();
      SIB.Hex = DecodeInst->SIB;
      DecodeInst->DecodedSIB = true;
    }

    // If the SIB base is 0b101, aka BP or R13 then we have a 32bit displacement
    if (ModRM.mod == 0b00 && ModRM.rm == 0b100 && SIB.base == 0b101) {
      Displacement = 4;
    }

    // SIB
    Operand->Type = DecodedOperand::OpType::SIB;
    Operand->Data.SIB.Scale = 1 << SIB.scale;

    // The invalid encoding types are described at Table 1-12. "promoted nsigned is always non-zero"
    {
      // If we have a VSIB byte (as opposed to SIB), then the index register is a vector.
      // DecodeInst->TableInfo may be null in the case of 3DNow! ModRM decoding.
      const bool IsIndexVector = DecodeInst->TableInfo && (DecodeInst->TableInfo->Flags & InstFlags::FLAGS_VEX_VSIB) != 0;
      uint8_t InvalidSIBIndex = 0b100; ///< SIB Index where there is no register encoding.
      if (IsIndexVector) {
        DecodeInst->Flags |= X86Tables::DecodeFlags::FLAG_VSIB_BYTE;
        InvalidSIBIndex = ~0; ///< No Invalid SIB Index with Index Vectors.
      }

      const uint8_t IndexREX = (DecodeInst->Flags & DecodeFlags::FLAG_REX_XGPR_X) != 0 ? 1 : 0;
      const uint8_t BaseREX = (DecodeInst->Flags & DecodeFlags::FLAG_REX_XGPR_B) != 0 ? 1 : 0;

      Operand->Data.SIB.Index = MapModRMToReg(IndexREX, SIB.index, false, false, IsIndexVector, false, InvalidSIBIndex);
      Operand->Data.SIB.Base = MapModRMToReg(BaseREX, SIB.base, false, false, false, false, ModRM.mod == 0 ? 0b101 : 16);
    }

    LOGMAN_THROW_A_FMT(Displacement <= 4, "Number of bytes should be <= 4 for literal src");

    if (Displacement) {
      uint64_t Literal = ReadData(Displacement);
      if (Displacement == 1) {
        Literal = static_cast<int8_t>(Literal);
      }
      Operand->Data.SIB.Offset = Literal;
    }
  } else if (ModRM.mod == 0) {
    // Explained in Table 1-14. "Operand Addressing Using ModRM and SIB Bytes"
    if (ModRM.rm == 0b101) {
      // 32bit Displacement
      const uint32_t Literal = ReadData(4);

      Operand->Type = DecodedOperand::OpType::RIPRelative;
      Operand->Data.RIPLiteral.Value.u = Literal;
    } else {
      // Register-direct addressing
      Operand->Type = DecodedOperand::OpType::GPRDirect;
      Operand->Data.GPR.GPR = MapModRMToReg(DecodeInst->Flags & DecodeFlags::FLAG_REX_XGPR_B ? 1 : 0, ModRM.rm, false, false, false, false);
    }
  } else {
    uint8_t DisplacementSize = ModRM.mod == 1 ? 1 : 4;
    uint32_t Literal = ReadData(DisplacementSize);
    if (DisplacementSize == 1) {
      Literal = static_cast<int8_t>(Literal);
    }

    Operand->Type = DecodedOperand::OpType::GPRIndirect;
    Operand->Data.GPRIndirect.GPR = MapModRMToReg(DecodeInst->Flags & DecodeFlags::FLAG_REX_XGPR_B ? 1 : 0, ModRM.rm, false, false, false, false);
    Operand->Data.GPRIndirect.Displacement = Literal;
  }
}

bool Decoder::NormalOp(const FEXCore::X86Tables::X86InstInfo* Info, uint16_t Op, DecodedHeader Options) {
  DecodeInst->OP = Op;
  DecodeInst->TableInfo = Info;

  if (Info->Type == FEXCore::X86Tables::TYPE_UNKNOWN) {
    return false;
  }

  if (Info->Type == FEXCore::X86Tables::TYPE_INVALID) {
    return false;
  }

  LOGMAN_THROW_A_FMT(!(Info->Type >= FEXCore::X86Tables::TYPE_GROUP_1 && Info->Type <= FEXCore::X86Tables::TYPE_GROUP_P), "Group Ops "
                                                                                                                          "should have "
                                                                                                                          "been decoded "
                                                                                                                          "before this!");

  uint8_t DestSize {};
  const bool HasWideningDisplacement =
    (FEXCore::X86Tables::DecodeFlags::GetOpAddr(DecodeInst->Flags, 0) & FEXCore::X86Tables::DecodeFlags::FLAG_WIDENING_SIZE_LAST) != 0 ||
    (Options.w && CTX->Config.Is64BitMode);
  const bool HasNarrowingDisplacement =
    (FEXCore::X86Tables::DecodeFlags::GetOpAddr(DecodeInst->Flags, 0) & FEXCore::X86Tables::DecodeFlags::FLAG_OPERAND_SIZE_LAST) != 0;

  const bool HasXMMFlags = (Info->Flags & InstFlags::FLAGS_XMM_FLAGS) != 0;
  bool HasXMMSrc =
    HasXMMFlags && !HAS_XMM_SUBFLAG(Info->Flags, InstFlags::FLAGS_SF_SRC_GPR) && !HAS_XMM_SUBFLAG(Info->Flags, InstFlags::FLAGS_SF_MMX_SRC);
  bool HasXMMDst =
    HasXMMFlags && !HAS_XMM_SUBFLAG(Info->Flags, InstFlags::FLAGS_SF_DST_GPR) && !HAS_XMM_SUBFLAG(Info->Flags, InstFlags::FLAGS_SF_MMX_DST);
  bool HasMMSrc =
    HasXMMFlags && !HAS_XMM_SUBFLAG(Info->Flags, InstFlags::FLAGS_SF_SRC_GPR) && HAS_XMM_SUBFLAG(Info->Flags, InstFlags::FLAGS_SF_MMX_SRC);
  bool HasMMDst =
    HasXMMFlags && !HAS_XMM_SUBFLAG(Info->Flags, InstFlags::FLAGS_SF_DST_GPR) && HAS_XMM_SUBFLAG(Info->Flags, InstFlags::FLAGS_SF_MMX_DST);

  // Is ModRM present via explicit instruction encoded or REX?
  const bool HasMODRM = !!(Info->Flags & FEXCore::X86Tables::InstFlags::FLAGS_MODRM);

  const bool HasREX = !!(DecodeInst->Flags & DecodeFlags::FLAG_REX_PREFIX);
  const bool Has16BitAddressing = !CTX->Config.Is64BitMode && DecodeInst->Flags & DecodeFlags::FLAG_ADDRESS_SIZE;

  // This is used for ModRM register modification
  // For both modrm.reg and modrm.rm(when mod == 0b11) when value is >= 0b100
  // then it changes from expected registers to the high 8bits of the lower registers
  // Bit annoying to support
  // In the case of no modrm (REX in byte situation) then it is unaffected
  bool Is8BitSrc {};
  bool Is8BitDest {};

  // If we require ModRM and haven't decoded it yet, do it now
  // Some instructions have to read modrm upfront, others do it later
  if (HasMODRM && !DecodeInst->DecodedModRM) {
    DecodeInst->ModRM = ReadByte();
    DecodeInst->DecodedModRM = true;
  }

  // New instruction size decoding
  {
    // Decode destinations first
    const auto DstSizeFlag = FEXCore::X86Tables::InstFlags::GetSizeDstFlags(Info->Flags);
    const auto SrcSizeFlag = FEXCore::X86Tables::InstFlags::GetSizeSrcFlags(Info->Flags);

    if (DstSizeFlag == FEXCore::X86Tables::InstFlags::SIZE_8BIT) {
      DecodeInst->Flags |= DecodeFlags::GenSizeDstSize(DecodeFlags::SIZE_8BIT);
      DestSize = 1;
      Is8BitDest = true;
    } else if (DstSizeFlag == FEXCore::X86Tables::InstFlags::SIZE_16BIT) {
      DecodeInst->Flags |= DecodeFlags::GenSizeDstSize(DecodeFlags::SIZE_16BIT);
      DestSize = 2;
    } else if (DstSizeFlag == FEXCore::X86Tables::InstFlags::SIZE_128BIT) {
      if (Options.L) {
        DecodeInst->Flags |= DecodeFlags::GenSizeDstSize(DecodeFlags::SIZE_256BIT);
        DestSize = 32;
      } else {
        DecodeInst->Flags |= DecodeFlags::GenSizeDstSize(DecodeFlags::SIZE_128BIT);
        DestSize = 16;
      }
    } else if (DstSizeFlag == FEXCore::X86Tables::InstFlags::SIZE_256BIT) {
      DecodeInst->Flags |= DecodeFlags::GenSizeDstSize(DecodeFlags::SIZE_256BIT);
      DestSize = 32;
    } else if (HasNarrowingDisplacement &&
               (DstSizeFlag == FEXCore::X86Tables::InstFlags::SIZE_DEF || DstSizeFlag == FEXCore::X86Tables::InstFlags::SIZE_64BITDEF)) {
      // See table 1-2. Operand-Size Overrides for this decoding
      // If the default operating mode is 32bit and we have the operand size flag then the operating size drops to 16bit
      DecodeInst->Flags |= DecodeFlags::GenSizeDstSize(DecodeFlags::SIZE_16BIT);
      DestSize = 2;
    } else if ((HasXMMDst || HasMMDst || CTX->Config.Is64BitMode) &&
               (HasWideningDisplacement || DstSizeFlag == FEXCore::X86Tables::InstFlags::SIZE_64BIT ||
                DstSizeFlag == FEXCore::X86Tables::InstFlags::SIZE_64BITDEF)) {
      DecodeInst->Flags |= DecodeFlags::GenSizeDstSize(DecodeFlags::SIZE_64BIT);
      DestSize = 8;
    } else {
      DecodeInst->Flags |= DecodeFlags::GenSizeDstSize(DecodeFlags::SIZE_32BIT);
      DestSize = 4;
    }

    // Decode sources
    if (SrcSizeFlag == FEXCore::X86Tables::InstFlags::SIZE_8BIT) {
      DecodeInst->Flags |= DecodeFlags::GenSizeSrcSize(DecodeFlags::SIZE_8BIT);
      Is8BitSrc = true;
    } else if (SrcSizeFlag == FEXCore::X86Tables::InstFlags::SIZE_16BIT) {
      DecodeInst->Flags |= DecodeFlags::GenSizeSrcSize(DecodeFlags::SIZE_16BIT);
    } else if (SrcSizeFlag == FEXCore::X86Tables::InstFlags::SIZE_128BIT) {
      if (Options.L) {
        DecodeInst->Flags |= DecodeFlags::GenSizeSrcSize(DecodeFlags::SIZE_256BIT);
      } else {
        DecodeInst->Flags |= DecodeFlags::GenSizeSrcSize(DecodeFlags::SIZE_128BIT);
      }
    } else if (SrcSizeFlag == FEXCore::X86Tables::InstFlags::SIZE_256BIT) {
      DecodeInst->Flags |= DecodeFlags::GenSizeSrcSize(DecodeFlags::SIZE_256BIT);
    } else if (HasNarrowingDisplacement &&
               (SrcSizeFlag == FEXCore::X86Tables::InstFlags::SIZE_DEF || SrcSizeFlag == FEXCore::X86Tables::InstFlags::SIZE_64BITDEF)) {
      // See table 1-2. Operand-Size Overrides for this decoding
      // If the default operating mode is 32bit and we have the operand size flag then the operating size drops to 16bit
      DecodeInst->Flags |= DecodeFlags::GenSizeSrcSize(DecodeFlags::SIZE_16BIT);
    } else if ((HasXMMSrc || HasMMSrc || CTX->Config.Is64BitMode) &&
               (HasWideningDisplacement || SrcSizeFlag == FEXCore::X86Tables::InstFlags::SIZE_64BIT ||
                SrcSizeFlag == FEXCore::X86Tables::InstFlags::SIZE_64BITDEF)) {
      DecodeInst->Flags |= DecodeFlags::GenSizeSrcSize(DecodeFlags::SIZE_64BIT);
    } else {
      DecodeInst->Flags |= DecodeFlags::GenSizeSrcSize(DecodeFlags::SIZE_32BIT);
    }
  }

  auto* CurrentDest = &DecodeInst->Dest;

  if (HAS_NON_XMM_SUBFLAG(Info->Flags, FEXCore::X86Tables::InstFlags::FLAGS_SF_DST_RAX) ||
      HAS_NON_XMM_SUBFLAG(Info->Flags, FEXCore::X86Tables::InstFlags::FLAGS_SF_DST_RDX)) {
    // Some instructions hardcode their destination as RAX
    CurrentDest->Type = DecodedOperand::OpType::GPR;
    CurrentDest->Data.GPR.HighBits = false;
    CurrentDest->Data.GPR.GPR =
      HAS_NON_XMM_SUBFLAG(Info->Flags, FEXCore::X86Tables::InstFlags::FLAGS_SF_DST_RAX) ? FEXCore::X86State::REG_RAX : FEXCore::X86State::REG_RDX;
    CurrentDest = &DecodeInst->Src[0];
  } else if (HAS_NON_XMM_SUBFLAG(Info->Flags, FEXCore::X86Tables::InstFlags::FLAGS_SF_REX_IN_BYTE)) {
    LOGMAN_THROW_A_FMT(!HasMODRM, "This instruction shouldn't have ModRM!");

    // If the REX is in the byte that means the lower nibble of the OP contains the destination GPR
    // This also means that the destination is always a GPR on these ones
    // ADDITIONALLY:
    // If there is a REX prefix then that allows extended GPR usage
    CurrentDest->Type = DecodedOperand::OpType::GPR;
    DecodeInst->Dest.Data.GPR.HighBits = (Is8BitDest && !HasREX && (Op & 0b111) >= 0b100);
    CurrentDest->Data.GPR.GPR =
      MapModRMToReg(DecodeInst->Flags & DecodeFlags::FLAG_REX_XGPR_B ? 1 : 0, Op & 0b111, Is8BitDest, HasREX, false, false);

    if (CurrentDest->Data.GPR.GPR == FEXCore::X86State::REG_INVALID) {
      return false;
    }
  }

  uint8_t Bytes = Info->MoreBytes;

  if ((Info->Flags & FEXCore::X86Tables::InstFlags::FLAGS_DISPLACE_SIZE_MUL_2) && HasWideningDisplacement) {
    Bytes <<= 1;
  }
  if ((Info->Flags & FEXCore::X86Tables::InstFlags::FLAGS_DISPLACE_SIZE_DIV_2) && HasNarrowingDisplacement) {
    Bytes >>= 1;
  }

  if ((Info->Flags & FEXCore::X86Tables::InstFlags::FLAGS_MEM_OFFSET) && (DecodeInst->Flags & DecodeFlags::FLAG_ADDRESS_SIZE)) {
    // If we have a memory offset and have the address size override then divide it just like narrowing displacement
    Bytes >>= 1;
  }

  auto ModRMOperand = [&](FEXCore::X86Tables::DecodedOperand& GPR, FEXCore::X86Tables::DecodedOperand& NonGPR, bool HasXMMGPR,
                          bool HasXMMNonGPR, bool HasMMGPR, bool HasMMNonGPR, bool GPR8Bit, bool NonGPR8Bit) {
    FEXCore::X86Tables::ModRMDecoded ModRM;
    ModRM.Hex = DecodeInst->ModRM;

    // Decode the GPR source first
    GPR.Type = DecodedOperand::OpType::GPR;
    GPR.Data.GPR.HighBits = (GPR8Bit && ModRM.reg >= 0b100 && !HasREX);
    GPR.Data.GPR.GPR = MapModRMToReg(DecodeInst->Flags & DecodeFlags::FLAG_REX_XGPR_R ? 1 : 0, ModRM.reg, GPR8Bit, HasREX, HasXMMGPR, HasMMGPR);

    if (GPR.Data.GPR.GPR == FEXCore::X86State::REG_INVALID) {
      return false;
    }

    // ModRM.mod == 0b11 == Register
    // ModRM.Mod != 0b11 == Register-direct addressing
    if (ModRM.mod == 0b11) {
      NonGPR.Type = DecodedOperand::OpType::GPR;
      NonGPR.Data.GPR.HighBits = (NonGPR8Bit && ModRM.rm >= 0b100 && !HasREX);
      NonGPR.Data.GPR.GPR =
        MapModRMToReg(DecodeInst->Flags & DecodeFlags::FLAG_REX_XGPR_B ? 1 : 0, ModRM.rm, NonGPR8Bit, HasREX, HasXMMNonGPR, HasMMNonGPR);
      if (NonGPR.Data.GPR.GPR == FEXCore::X86State::REG_INVALID) {
        return false;
      }
    } else {
      // Only decode if we haven't pre-decoded
      if (NonGPR.IsNone()) {
        auto Disp = DecodeModRMs_Disp[Has16BitAddressing];
        (this->*Disp)(&NonGPR, ModRM);
      }
    }

    return true;
  };

  size_t CurrentSrc = 0;

  if ((Info->Flags & FEXCore::X86Tables::InstFlags::FLAGS_VEX_1ST_SRC) != 0) {
    DecodeInst->Src[CurrentSrc].Type = DecodedOperand::OpType::GPR;
    DecodeInst->Src[CurrentSrc].Data.GPR.HighBits = false;

    // If we have XMM flags at all, then SRC 1 cannot be a GPR. The only case where
    // this is possible is with BMI1 and BMI2 instructions (which are all GPR-based
    // and don't use XMM flags)
    DecodeInst->Src[CurrentSrc].Data.GPR.GPR = MapVEXToReg(Options.vvvv, HasXMMFlags);

    ++CurrentSrc;
  }

  if (Info->Flags & FEXCore::X86Tables::InstFlags::FLAGS_MODRM) {
    if (Info->Flags & FEXCore::X86Tables::InstFlags::FLAGS_SF_MOD_DST) {
      if (!ModRMOperand(DecodeInst->Src[CurrentSrc], DecodeInst->Dest, HasXMMSrc, HasXMMDst, HasMMSrc, HasMMDst, Is8BitSrc, Is8BitDest)) {
        return false;
      }
    } else {
      if (!ModRMOperand(DecodeInst->Dest, DecodeInst->Src[CurrentSrc], HasXMMDst, HasXMMSrc, HasMMDst, HasMMSrc, Is8BitDest, Is8BitSrc)) {
        return false;
      }
    }
    ++CurrentSrc;
  }

  if ((Info->Flags & FEXCore::X86Tables::InstFlags::FLAGS_VEX_2ND_SRC) != 0) {
    DecodeInst->Src[CurrentSrc].Type = DecodedOperand::OpType::GPR;
    DecodeInst->Src[CurrentSrc].Data.GPR.HighBits = false;
    DecodeInst->Src[CurrentSrc].Data.GPR.GPR = MapVEXToReg(Options.vvvv, HasXMMSrc);
    ++CurrentSrc;
  }

  if (HAS_NON_XMM_SUBFLAG(Info->Flags, FEXCore::X86Tables::InstFlags::FLAGS_SF_SRC_RAX)) {
    DecodeInst->Src[CurrentSrc].Type = DecodedOperand::OpType::GPR;
    DecodeInst->Src[CurrentSrc].Data.GPR.HighBits = false;
    DecodeInst->Src[CurrentSrc].Data.GPR.GPR = FEXCore::X86State::REG_RAX;
    ++CurrentSrc;
  } else if (HAS_NON_XMM_SUBFLAG(Info->Flags, FEXCore::X86Tables::InstFlags::FLAGS_SF_SRC_RCX)) {
    DecodeInst->Src[CurrentSrc].Type = DecodedOperand::OpType::GPR;
    DecodeInst->Src[CurrentSrc].Data.GPR.HighBits = false;
    DecodeInst->Src[CurrentSrc].Data.GPR.GPR = FEXCore::X86State::REG_RCX;
    ++CurrentSrc;
  }

  if ((Info->Flags & FEXCore::X86Tables::InstFlags::FLAGS_VEX_DST) != 0) {
    CurrentDest->Type = DecodedOperand::OpType::GPR;
    CurrentDest->Data.GPR.HighBits = false;
    CurrentDest->Data.GPR.GPR = MapVEXToReg(Options.vvvv, HasXMMDst);
  }

  if (Bytes != 0) {
    LOGMAN_THROW_A_FMT(Bytes <= 8, "Number of bytes should be <= 8 for literal src");

    DecodeInst->Src[CurrentSrc].Data.Literal.Size = Bytes;

    uint64_t Literal = ReadData(Bytes);

    if ((Info->Flags & FEXCore::X86Tables::InstFlags::FLAGS_SRC_SEXT) || (DecodeFlags::GetSizeDstFlags(DecodeInst->Flags) == DecodeFlags::SIZE_64BIT &&
                                                                          Info->Flags & FEXCore::X86Tables::InstFlags::FLAGS_SRC_SEXT64BIT)) {
      if (Bytes == 1) {
        Literal = static_cast<int8_t>(Literal);
      } else if (Bytes == 2) {
        Literal = static_cast<int16_t>(Literal);
      } else {
        Literal = static_cast<int32_t>(Literal);
      }
      DecodeInst->Src[CurrentSrc].Data.Literal.Size = DestSize;
    }

    Bytes = 0;
    DecodeInst->Src[CurrentSrc].Type = DecodedOperand::OpType::Literal;
    DecodeInst->Src[CurrentSrc].Data.Literal.Value = Literal;
  }

  LOGMAN_THROW_A_FMT(Bytes == 0, "Inst at 0x{:x}: 0x{:04x} '{}' Had an instruction of size {} with {} remaining", DecodeInst->PC,
                     DecodeInst->OP, DecodeInst->TableInfo->Name ?: "UND", InstructionSize, Bytes);
  DecodeInst->InstSize = InstructionSize;
  return true;
}

bool Decoder::NormalOpHeader(const FEXCore::X86Tables::X86InstInfo* Info, uint16_t Op) {
  DecodeInst->OP = Op;
  DecodeInst->TableInfo = Info;

  if (Info->Type == FEXCore::X86Tables::TYPE_UNKNOWN) {
    return false;
  }

  if (Info->Type == FEXCore::X86Tables::TYPE_INVALID) {
    return false;
  }

  LOGMAN_THROW_A_FMT(Info->Type != FEXCore::X86Tables::TYPE_REX_PREFIX, "REX PREFIX should have been decoded before this!");

  // A normal instruction is the most likely.
  if (Info->Type == FEXCore::X86Tables::TYPE_INST) [[likely]] {
    return NormalOp(Info, Op);
  } else if (Info->Type >= FEXCore::X86Tables::TYPE_GROUP_1 && Info->Type <= FEXCore::X86Tables::TYPE_GROUP_11) {
    uint8_t ModRMByte = ReadByte();
    DecodeInst->ModRM = ModRMByte;
    DecodeInst->DecodedModRM = true;

    FEXCore::X86Tables::ModRMDecoded ModRM;
    ModRM.Hex = DecodeInst->ModRM;

#define OPD(group, prefix, Reg) (((group - FEXCore::X86Tables::TYPE_GROUP_1) << 6) | (prefix) << 3 | (Reg))
    Op = OPD(Info->Type, Info->MoreBytes, ModRM.reg);
    return NormalOp(&PrimaryInstGroupOps[Op], Op);
#undef OPD
  } else if (Info->Type >= FEXCore::X86Tables::TYPE_GROUP_6 && Info->Type <= FEXCore::X86Tables::TYPE_GROUP_P) {
#define OPD(group, prefix, Reg) (((group - FEXCore::X86Tables::TYPE_GROUP_6) << 5) | (prefix) << 3 | (Reg))
    constexpr uint16_t PF_NONE = 0;
    constexpr uint16_t PF_F3 = 1;
    constexpr uint16_t PF_66 = 2;
    constexpr uint16_t PF_F2 = 3;

    uint16_t PrefixType = PF_NONE;
    if (DecodeInst->LastEscapePrefix == 0xF3) {
      PrefixType = PF_F3;
    } else if (DecodeInst->LastEscapePrefix == 0xF2) {
      PrefixType = PF_F2;
    } else if (DecodeInst->LastEscapePrefix == 0x66) {
      PrefixType = PF_66;
    }

    // We have ModRM
    uint8_t ModRMByte = ReadByte();
    DecodeInst->ModRM = ModRMByte;
    DecodeInst->DecodedModRM = true;

    FEXCore::X86Tables::ModRMDecoded ModRM;
    ModRM.Hex = DecodeInst->ModRM;

    uint16_t LocalOp = OPD(Info->Type, PrefixType, ModRM.reg);
    FEXCore::X86Tables::X86InstInfo* LocalInfo = &SecondInstGroupOps[LocalOp];
#undef OPD
    if (LocalInfo->Type == FEXCore::X86Tables::TYPE_SECOND_GROUP_MODRM) {
      // Everything in this group is privileged instructions aside from XGETBV
      constexpr std::array<uint8_t, 8> RegToField = {
        255, 0, 1, 2, 255, 255, 255, 3,
      };
      uint8_t Field = RegToField[ModRM.reg];
      LOGMAN_THROW_A_FMT(Field != 255, "Invalid field selected!");

      LocalOp = (Field << 3) | ModRM.rm;
      return NormalOp(&SecondModRMTableOps[LocalOp], LocalOp);
    } else {
      return NormalOp(&SecondInstGroupOps[LocalOp], LocalOp);
    }
  } else if (Info->Type == FEXCore::X86Tables::TYPE_X87_TABLE_PREFIX) {
    // We have ModRM
    uint8_t ModRMByte = ReadByte();
    DecodeInst->ModRM = ModRMByte;
    DecodeInst->DecodedModRM = true;

    uint16_t X87Op = ((Op - 0xD8) << 8) | ModRMByte;
    return NormalOp(&X87Ops[X87Op], X87Op);
  } else if (Info->Type == FEXCore::X86Tables::TYPE_VEX_TABLE_PREFIX) {
    uint16_t map_select = 1;
    uint16_t pp = 0;
    const uint8_t Byte1 = ReadByte();
    DecodedHeader options {};

    if ((Byte1 & 0b10000000) == 0) {
      LOGMAN_THROW_A_FMT(CTX->Config.Is64BitMode, "VEX.R shouldn't be 0 in 32-bit mode!");
      DecodeInst->Flags |= DecodeFlags::FLAG_REX_XGPR_R;
    }

    if (Op == 0xC5) { // Two byte VEX
      pp = Byte1 & 0b11;
      options.vvvv = 15 - ((Byte1 & 0b01111000) >> 3);
      options.L = (Byte1 & 0b100) != 0;
    } else { // 0xC4 = Three byte VEX
      const uint8_t Byte2 = ReadByte();
      pp = Byte2 & 0b11;
      map_select = Byte1 & 0b11111;
      options.vvvv = 15 - ((Byte2 & 0b01111000) >> 3);
      options.w = (Byte2 & 0b10000000) != 0;
      options.L = (Byte2 & 0b100) != 0;
      if ((Byte1 & 0b01000000) == 0) {
        LOGMAN_THROW_A_FMT(CTX->Config.Is64BitMode, "VEX.X shouldn't be 0 in 32-bit mode!");
        DecodeInst->Flags |= DecodeFlags::FLAG_REX_XGPR_X;
      }
      if (CTX->Config.Is64BitMode && (Byte1 & 0b00100000) == 0) {
        DecodeInst->Flags |= DecodeFlags::FLAG_REX_XGPR_B;
      }
      if (options.w) {
        DecodeInst->Flags |= DecodeFlags::FLAG_OPTION_AVX_W;
      }
      if (!(map_select >= 1 && map_select <= 3)) {
        LogMan::Msg::EFmt("We don't understand a map_select of: {}", map_select);
        return false;
      }
    }

    uint16_t VEXOp = ReadByte();
#define OPD(map_select, pp, opcode) (((map_select - 1) << 10) | (pp << 8) | (opcode))
    Op = OPD(map_select, pp, VEXOp);
#undef OPD

    FEXCore::X86Tables::X86InstInfo* LocalInfo = &VEXTableOps[Op];

    if (LocalInfo->Type >= FEXCore::X86Tables::TYPE_VEX_GROUP_12 && LocalInfo->Type <= FEXCore::X86Tables::TYPE_VEX_GROUP_17) {
      // We have ModRM
      uint8_t ModRMByte = ReadByte();
      DecodeInst->ModRM = ModRMByte;
      DecodeInst->DecodedModRM = true;

      FEXCore::X86Tables::ModRMDecoded ModRM;
      ModRM.Hex = DecodeInst->ModRM;

#define OPD(group, pp, opcode) (((group - TYPE_VEX_GROUP_12) << 4) | (pp << 3) | (opcode))
      Op = OPD(LocalInfo->Type, pp, ModRM.reg);
#undef OPD
      return NormalOp(&VEXTableGroupOps[Op], Op, options);
    } else {
      return NormalOp(LocalInfo, Op, options);
    }
  } else if (Info->Type == FEXCore::X86Tables::TYPE_GROUP_EVEX) {
    FEXCORE_TELEMETRY_SET(EVEXOpTelem, 1);
    // EVEX unsupported
    return false;
  }

  LOGMAN_MSG_A_FMT("Invalid instruction decoding type");
  FEX_UNREACHABLE;
}

bool Decoder::DecodeInstruction(uint64_t PC) {
  InstructionSize = 0;
  Instruction.fill(0);

  DecodeInst = &DecodedBuffer[DecodedSize];
  memset(DecodeInst, 0, sizeof(DecodedInst));
  DecodeInst->PC = PC;

  for (;;) {
    if (InstructionSize >= MAX_INST_SIZE) {
      return false;
    }
    uint8_t Op = ReadByte();
    switch (Op) {
    case 0x0F: { // Escape Op
      uint8_t EscapeOp = ReadByte();
      switch (EscapeOp) {
      case 0x0F:
        [[unlikely]] { // 3DNow!
          // 3DNow! Instruction Encoding: 0F 0F [ModRM] [SIB] [Displacement] [Opcode]
          // Decode ModRM
          uint8_t ModRMByte = ReadByte();
          DecodeInst->ModRM = ModRMByte;
          DecodeInst->DecodedModRM = true;

          FEXCore::X86Tables::ModRMDecoded ModRM;
          ModRM.Hex = DecodeInst->ModRM;

          const bool Has16BitAddressing = !CTX->Config.Is64BitMode && DecodeInst->Flags & DecodeFlags::FLAG_ADDRESS_SIZE;

          // All 3DNow! instructions have the second argument as the rm handler
          // We need to decode it upfront to get the displacement out of the way
          if (ModRM.mod != 0b11) {
            auto Disp = DecodeModRMs_Disp[Has16BitAddressing];
            (this->*Disp)(&DecodeInst->Src[0], ModRM);
          }

          // Take a peek at the op just past the displacement
          uint8_t LocalOp = ReadByte();
          return NormalOpHeader(&FEXCore::X86Tables::DDDNowOps[LocalOp], LocalOp);
          break;
        }
      case 0x38: { // F38 Table!
        constexpr uint16_t PF_38_NONE = 0;
        constexpr uint16_t PF_38_66 = (1U << 0);
        constexpr uint16_t PF_38_F2 = (1U << 1);
        constexpr uint16_t PF_38_F3 = (1U << 2);

        uint16_t Prefix = PF_38_NONE;
        if (DecodeInst->Flags & DecodeFlags::FLAG_OPERAND_SIZE) {
          Prefix |= PF_38_66;
        }
        if (DecodeInst->Flags & DecodeFlags::FLAG_REPNE_PREFIX) {
          Prefix |= PF_38_F2;
        }
        if (DecodeInst->Flags & DecodeFlags::FLAG_REP_PREFIX) {
          Prefix |= PF_38_F3;
        }

        uint16_t LocalOp = (Prefix << 8) | ReadByte();

        bool NoOverlay66 = (FEXCore::X86Tables::H0F38TableOps[LocalOp].Flags & InstFlags::FLAGS_NO_OVERLAY66) != 0;
        if (DecodeInst->LastEscapePrefix == 0x66 && NoOverlay66) { // Operand Size
          // Remove prefix so it doesn't effect calculations.
          // This is only an escape prefix rather than modifier now
          DecodeInst->Flags &= ~DecodeFlags::FLAG_OPERAND_SIZE;
          DecodeFlags::PopOpAddrIf(&DecodeInst->Flags, DecodeFlags::FLAG_OPERAND_SIZE_LAST);
        }

        return NormalOpHeader(&FEXCore::X86Tables::H0F38TableOps[LocalOp], LocalOp);
        break;
      }
      case 0x3A: { // F3A Table!
        constexpr uint16_t PF_3A_NONE = 0;
        constexpr uint16_t PF_3A_66 = (1 << 0);
        constexpr uint16_t PF_3A_REX = (1 << 1);

        uint16_t Prefix = PF_3A_NONE;
        if (DecodeInst->LastEscapePrefix == 0x66) { // Operand Size
          Prefix = PF_3A_66;
        }

        if (DecodeInst->Flags & DecodeFlags::FLAG_REX_WIDENING) {
          Prefix |= PF_3A_REX;
        }

        uint16_t LocalOp = (Prefix << 8) | ReadByte();
        return NormalOpHeader(&FEXCore::X86Tables::H0F3ATableOps[LocalOp], LocalOp);
        break;
      }
      default:
        [[likely]] { // Two byte table!
          // x86-64 abuses three legacy prefixes to extend the table encodings
          // 0x66 - Operand Size prefix
          // 0xF2 - REPNE prefix
          // 0xF3 - REP prefix
          // If any of these three prefixes are used then it falls down the subtable
          // Additionally: If you hit repeat of differnt prefixes then only the LAST one before this one works for subtable selection

          bool NoOverlay = (FEXCore::X86Tables::SecondBaseOps[EscapeOp].Flags & InstFlags::FLAGS_NO_OVERLAY) != 0;
          bool NoOverlay66 = (FEXCore::X86Tables::SecondBaseOps[EscapeOp].Flags & InstFlags::FLAGS_NO_OVERLAY66) != 0;

          if (NoOverlay) { // This section of the table ignores prefix extention
            return NormalOpHeader(&FEXCore::X86Tables::SecondBaseOps[EscapeOp], EscapeOp);
          } else if (DecodeInst->LastEscapePrefix == 0xF3) { // REP
            // Remove prefix so it doesn't effect calculations.
            // This is only an escape prefix rather tan modifier now
            DecodeInst->Flags &= ~DecodeFlags::FLAG_REP_PREFIX;
            return NormalOpHeader(&FEXCore::X86Tables::RepModOps[EscapeOp], EscapeOp);
          } else if (DecodeInst->LastEscapePrefix == 0xF2) { // REPNE
            // Remove prefix so it doesn't effect calculations.
            // This is only an escape prefix rather tan modifier now
            DecodeInst->Flags &= ~DecodeFlags::FLAG_REPNE_PREFIX;
            return NormalOpHeader(&FEXCore::X86Tables::RepNEModOps[EscapeOp], EscapeOp);
          } else if (DecodeInst->LastEscapePrefix == 0x66 && !NoOverlay66) { // Operand Size
            // Remove prefix so it doesn't effect calculations.
            // This is only an escape prefix rather tan modifier now
            DecodeInst->Flags &= ~DecodeFlags::FLAG_OPERAND_SIZE;
            DecodeFlags::PopOpAddrIf(&DecodeInst->Flags, DecodeFlags::FLAG_OPERAND_SIZE_LAST);
            return NormalOpHeader(&FEXCore::X86Tables::OpSizeModOps[EscapeOp], EscapeOp);
          } else {
            return NormalOpHeader(&FEXCore::X86Tables::SecondBaseOps[EscapeOp], EscapeOp);
          }
          break;
        }
      }
      break;
    }
    case 0x66: // Operand Size prefix
      DecodeInst->Flags |= DecodeFlags::FLAG_OPERAND_SIZE;
      DecodeInst->LastEscapePrefix = Op;
      DecodeFlags::PushOpAddr(&DecodeInst->Flags, DecodeFlags::FLAG_OPERAND_SIZE_LAST);
      break;
    case 0x67: // Address Size override prefix
      DecodeInst->Flags |= DecodeFlags::FLAG_ADDRESS_SIZE;
      break;
    case 0x26: // ES legacy prefix
      if (!CTX->Config.Is64BitMode) {
        DecodeInst->Flags |= DecodeFlags::FLAG_ES_PREFIX;
      }
      break;
    case 0x2E: // CS legacy prefix
      if (!CTX->Config.Is64BitMode) {
        DecodeInst->Flags |= DecodeFlags::FLAG_CS_PREFIX;
      }
      break;
    case 0x36: // SS legacy prefix
      if (!CTX->Config.Is64BitMode) {
        DecodeInst->Flags |= DecodeFlags::FLAG_SS_PREFIX;
      }
      break;
    case 0x3E: // DS legacy prefix
      // Annoyingly GCC generates NOP ops with these prefixes
      // Just ignore them for now
      // eg. 66 2e 0f 1f 84 00 00 00 00 00 nop    WORD PTR cs:[rax+rax*1+0x0]
      if (!CTX->Config.Is64BitMode) {
        DecodeInst->Flags |= DecodeFlags::FLAG_DS_PREFIX;
      }
      break;
      break;
    case 0xF0: // LOCK prefix
      DecodeInst->Flags |= DecodeFlags::FLAG_LOCK;
      break;
    case 0xF2: // REPNE prefix
      DecodeInst->Flags |= DecodeFlags::FLAG_REPNE_PREFIX;
      DecodeInst->LastEscapePrefix = Op;
      break;
    case 0xF3: // REP prefix
      DecodeInst->Flags |= DecodeFlags::FLAG_REP_PREFIX;
      DecodeInst->LastEscapePrefix = Op;
      break;
    case 0x64: // FS prefix
      DecodeInst->Flags |= DecodeFlags::FLAG_FS_PREFIX;
      break;
    case 0x65: // GS prefix
      DecodeInst->Flags |= DecodeFlags::FLAG_GS_PREFIX;
      break;
    default:
      [[likely]] { // Default base table
        auto Info = &FEXCore::X86Tables::BaseOps[Op];

        if (Info->Type == FEXCore::X86Tables::TYPE_REX_PREFIX) {
          LOGMAN_THROW_A_FMT(CTX->Config.Is64BitMode, "Got REX prefix in 32bit mode");
          DecodeInst->Flags |= DecodeFlags::FLAG_REX_PREFIX;

          // Widening displacement
          if (Op & 0b1000) {
            DecodeInst->Flags |= DecodeFlags::FLAG_REX_WIDENING;
            DecodeFlags::PushOpAddr(&DecodeInst->Flags, DecodeFlags::FLAG_WIDENING_SIZE_LAST);
          }

          // XGPR_B bit set
          if (Op & 0b0001) {
            DecodeInst->Flags |= DecodeFlags::FLAG_REX_XGPR_B;
          }

          // XGPR_X bit set
          if (Op & 0b0010) {
            DecodeInst->Flags |= DecodeFlags::FLAG_REX_XGPR_X;
          }

          // XGPR_R bit set
          if (Op & 0b0100) {
            DecodeInst->Flags |= DecodeFlags::FLAG_REX_XGPR_R;
          }
        } else {
          return NormalOpHeader(Info, Op);
        }

        break;
      }
    }
  }

  if (DecodeInst->Dest.IsGPR()) {
    LOGMAN_THROW_A_FMT(DecodeInst->Dest.Data.GPR.GPR != FEXCore::X86State::REG_INVALID, "Destination GPR was invalid");
  }

  return true;
}

void Decoder::BranchTargetInMultiblockRange() {
  if (!CTX->Config.Multiblock) {
    return;
  }

  // If the RIP setting is conditional AND within our symbol range then it can be considered for multiblock
  uint64_t TargetRIP = 0;
  const auto GPRSize = CTX->GetGPROpSize();
  bool Conditional = true;
  const auto InstEnd = DecodeInst->PC + DecodeInst->InstSize;

  switch (DecodeInst->OP) {
  case 0x70 ... 0x7F:   // Conditional JUMP
  case 0x80 ... 0x8F: { // More conditional
    // Source is a literal
    // auto RIPOffset = LoadSource(Op, Op->Src[0], Op->Flags);
    // auto RIPTargetConst = _Constant(Op->PC + Op->InstSize);
    // Target offset is PC + InstSize + Literal
    TargetRIP = InstEnd + DecodeInst->Src[0].Literal();
    break;
  }
  case 0xE9:
  case 0xEB: // Both are unconditional JMP instructions
    TargetRIP = InstEnd + DecodeInst->Src[0].Literal();
    Conditional = false;
    break;
  case 0xE8: // Call - Immediate target, We don't want to inline calls
    if (ExternalBranches) {
      ExternalBranches->insert(InstEnd);
    }
    [[fallthrough]];
  case 0xC2: // RET imm
  case 0xC3: // RET
  default: return; break;
  }

  if (GPRSize == IR::OpSize::i32Bit) {
    // If we are running a 32bit guest then wrap around addresses that go above 32bit
    TargetRIP &= 0xFFFFFFFFU;
  }

  // If the target RIP is x86 code within the symbol ranges then we are golden
  // Forbid cross-page branches to both avoid massive (range-wise) code blocks in highly fragmented code and trying to decode unmapped branch targets
  bool ValidMultiblockMember =
    TargetRIP >= SymbolMinAddress && TargetRIP < std::min(FEXCore::AlignUp(InstEnd, FEXCore::Utils::FEX_PAGE_SIZE), SymbolMaxAddress);

#ifdef _M_ARM_64EC
  ValidMultiblockMember = ValidMultiblockMember && !RtlIsEcCode(TargetRIP);
#endif

  if (ValidMultiblockMember) {
    // Update our conditional branch ranges before we return
    if (Conditional) {
      MaxCondBranchForward = std::max(MaxCondBranchForward, TargetRIP);
      MaxCondBranchBackwards = std::min(MaxCondBranchBackwards, TargetRIP);

      // If we are conditional then a target can be the instruction past the conditional instruction
      if (!HasBlocks.contains(InstEnd)) {
        CurrentBlockTargets.insert(InstEnd);
      }
    }

    if (!HasBlocks.contains(TargetRIP)) {
      CurrentBlockTargets.insert(TargetRIP);
    }
  } else {
    if (ExternalBranches) {
      ExternalBranches->insert(TargetRIP);
    }
  }
}

bool Decoder::BranchTargetCanContinue(bool FinalInstruction) const {
  if (FinalInstruction) {
    return false;
  }

  uint64_t TargetRIP = 0;
  const auto GPRSize = CTX->GetGPROpSize();

  if (DecodeInst->OP == 0xE8) { // Call - immediate target
    const uint64_t NextRIP = DecodeInst->PC + DecodeInst->InstSize;
    TargetRIP = DecodeInst->PC + DecodeInst->InstSize + DecodeInst->Src[0].Literal();

    if (GPRSize == IR::OpSize::i32Bit) {
      // If we are running a 32bit guest then wrap around addresses that go above 32bit
      TargetRIP &= 0xFFFFFFFFU;
    }

    if (TargetRIP == NextRIP) {
      // Optimize the case that the instruction is jumping just after itself.
      // This is a GOT calculation which we can optimize out.
      // Optimization occurs inside of the OpDispatcher implementation
      return true;
    }
  }

  return false;
}

const uint8_t* Decoder::AdjustAddrForSpecialRegion(const uint8_t* _InstStream, uint64_t EntryPoint, uint64_t RIP) {
  constexpr uint64_t VSyscall_Base = 0xFFFF'FFFF'FF60'0000ULL;
  constexpr uint64_t VSyscall_End = VSyscall_Base + 0x1000;

  if (OSABI == FEXCore::HLE::SyscallOSABI::OS_LINUX64 && RIP >= VSyscall_Base && RIP < VSyscall_End) {
    // VSyscall
    // This doesn't exist on AArch64 and on x86_64 hosts this is emulated with faults to a region mapped with --xp permissions
    // Offset     0: vgettimeofday
    // Offset 0x400: vtime
    // Offset 0x800: vgetcpu
    uint64_t Offset = RIP - VSyscall_Base;
    return VSyscallData + Offset;
  }

  return _InstStream - EntryPoint + RIP;
}

void Decoder::DecodeInstructionsAtEntry(const uint8_t* _InstStream, uint64_t PC, uint64_t MaxInst,
                                        std::function<void(uint64_t BlockEntry, uint64_t Start, uint64_t Length)> AddContainedCodePage) {
  FEXCORE_PROFILE_SCOPED("DecodeInstructions");
  BlockInfo.TotalInstructionCount = 0;
  BlockInfo.Blocks.clear();
  BlocksToDecode.clear();
  HasBlocks.clear();
  // Reset internal state management
  DecodedSize = 0;
  MaxCondBranchForward = 0;
  MaxCondBranchBackwards = ~0ULL;
  DecodedBuffer = PoolObject.ReownOrClaimBuffer();

  // XXX: Load symbol data
  SymbolAvailable = false;
  EntryPoint = PC;
  InstStream = _InstStream;

  uint64_t TotalInstructions {};

  // If we don't have symbols available then we become a bit optimistic about multiblock ranges
  if (!SymbolAvailable) {
    // If we don't have a symbol available then assume all branches are valid for multiblock
    SymbolMaxAddress = SectionMaxAddress;
    SymbolMinAddress = EntryPoint;
  }

  DecodedMinAddress = EntryPoint;
  DecodedMaxAddress = EntryPoint;

  // Entry is a jump target
  BlocksToDecode.emplace(PC);

  uint64_t CurrentCodePage = PC & FEXCore::Utils::FEX_PAGE_MASK;

  fextl::set<uint64_t> CodePages = {CurrentCodePage};

  AddContainedCodePage(PC, CurrentCodePage, FEXCore::Utils::FEX_PAGE_SIZE);

  if (MaxInst == 0) {
    MaxInst = CTX->Config.MaxInstPerBlock;
  }

  bool EntryBlock {true};
  bool FinalInstruction {false};

  while (!FinalInstruction && !BlocksToDecode.empty()) {
    auto BlockDecodeIt = BlocksToDecode.begin();
    uint64_t RIPToDecode = *BlockDecodeIt;
    BlockInfo.Blocks.emplace_back();
    DecodedBlocks& CurrentBlockDecoding = BlockInfo.Blocks.back();

    CurrentBlockDecoding.Entry = RIPToDecode;

    uint64_t PCOffset = 0;
    uint64_t BlockNumberOfInstructions {};
    uint64_t BlockStartOffset = DecodedSize;

    // Do a bit of pointer math to figure out where we are in code
    InstStream = AdjustAddrForSpecialRegion(_InstStream, EntryPoint, RIPToDecode);

    while (1) {
      // MAX_INST_SIZE assumes worst case
      auto OpAddress = RIPToDecode + PCOffset;
      auto OpMaxAddress = OpAddress + MAX_INST_SIZE;

      auto OpMinPage = OpAddress & FEXCore::Utils::FEX_PAGE_MASK;
      auto OpMaxPage = OpMaxAddress & FEXCore::Utils::FEX_PAGE_MASK;

      if (!EntryBlock && OpMinPage == OpMaxPage && PeekByte(0) == 0 && PeekByte(1) == 0) [[unlikely]] {
        // End the multiblock early if we hit 2 consecutive null bytes (add [rax], al) in the same page with the
        // assumption we are most likely trying to explore garbage code.
        break;
      }

      if (OpMinPage != CurrentCodePage) {
        CurrentCodePage = OpMinPage;
        CodePages.insert(CurrentCodePage);
      }

      if (OpMaxPage != CurrentCodePage) {
        CurrentCodePage = OpMaxPage;
        CodePages.insert(CurrentCodePage);
      }

      bool ErrorDuringDecoding = !DecodeInstruction(OpAddress);

      if (ErrorDuringDecoding) [[unlikely]] {
        // Put an invalid instruction in the stream so the core can raise SIGILL if hit
        CurrentBlockDecoding.HasInvalidInstruction = true;
        // Error while decoding instruction. We don't know the table or instruction size
        DecodeInst->TableInfo = nullptr;
        DecodeInst->InstSize = 0;
      }

      if (!ErrorDuringDecoding) {
        // If there wasn't an error during decoding but we have no dispatcher for the instruction then claim invalid instruction.
        auto TableInfo = DecodedBuffer[BlockStartOffset + BlockNumberOfInstructions].TableInfo;
        if (!TableInfo || !TableInfo->OpcodeDispatcher) {
          CurrentBlockDecoding.HasInvalidInstruction = true;
        }
      }

      DecodedMinAddress = std::min(DecodedMinAddress, OpAddress);
      DecodedMaxAddress = std::max(DecodedMaxAddress, OpAddress + DecodeInst->InstSize);
      ++TotalInstructions;
      ++BlockNumberOfInstructions;
      ++DecodedSize;

      // Can not continue this block at all on invalid instruction
      if (CurrentBlockDecoding.HasInvalidInstruction) [[unlikely]] {
        if (!EntryBlock) {
          // In multiblock configurations, we can early terminate any non-entrypoint blocks with the expectation that this won't get hit.
          // Improves compile-times.
          // Just need to undo additions that this block decoding has caused.
          TotalInstructions -= CurrentBlockDecoding.NumInstructions;
          DecodedSize = BlockStartOffset;
          BlockNumberOfInstructions = 0;
          InstStream -= PCOffset;
          CurrentBlockTargets.clear();
        }
        break;
      }

      bool CanContinue = false;
      if (!(DecodeInst->TableInfo->Flags & (FEXCore::X86Tables::InstFlags::FLAGS_BLOCK_END | FEXCore::X86Tables::InstFlags::FLAGS_SETS_RIP))) {
        // If this isn't a block ender then we can keep going regardless
        CanContinue = true;
      }

      FinalInstruction = DecodedSize >= MaxInst || DecodedSize >= DefaultDecodedBufferSize || TotalInstructions >= MaxInst;

      if (DecodeInst->TableInfo->Flags & FEXCore::X86Tables::InstFlags::FLAGS_SETS_RIP) {
        // If we have multiblock enabled
        // If the branch target is within our multiblock range then we can keep going on
        // We don't want to short circuit this since we want to calculate our ranges still
        BranchTargetInMultiblockRange();

        // Bypass branches if we can continue through them in some cases.
        CanContinue |= BranchTargetCanContinue(FinalInstruction);
      }

      if (FinalInstruction || !CanContinue) {
        break;
      }

      PCOffset += DecodeInst->InstSize;
      InstStream += DecodeInst->InstSize;
    }

    BlocksToDecode.merge(CurrentBlockTargets);
    CurrentBlockTargets.clear();

    BlocksToDecode.erase(BlockDecodeIt);
    HasBlocks.emplace(RIPToDecode);

    // Copy over only the number of instructions we decoded
    CurrentBlockDecoding.NumInstructions = BlockNumberOfInstructions;
    CurrentBlockDecoding.DecodedInstructions = &DecodedBuffer[BlockStartOffset];
    BlockInfo.TotalInstructionCount += BlockNumberOfInstructions;

    EntryBlock = false;
  }

  for (auto CodePage : CodePages) {
    AddContainedCodePage(PC, CodePage, FEXCore::Utils::FEX_PAGE_SIZE);
  }

  // sort for better branching
  std::sort(BlockInfo.Blocks.begin(), BlockInfo.Blocks.end(),
            [](const FEXCore::Frontend::Decoder::DecodedBlocks& a, const FEXCore::Frontend::Decoder::DecodedBlocks& b) {
    return a.Entry < b.Entry;
  });
}

} // namespace FEXCore::Frontend
