// SPDX-License-Identifier: MIT
/*
$info$
tags: frontend|x86-tables
$end_info$
*/

#pragma once

#include <FEXCore/Utils/LogManager.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace FEXCore::IR {
class OpDispatchBuilder;
}

namespace FEXCore::X86Tables {
struct X86InstInfo;

namespace DecodeFlags {
constexpr uint32_t FLAG_OPERAND_SIZE  = (1 << 0);
constexpr uint32_t FLAG_ADDRESS_SIZE  = (1 << 1);
constexpr uint32_t FLAG_LOCK          = (1 << 2);
constexpr uint32_t FLAG_LEGACY_PREFIX = (1 << 3);
constexpr uint32_t FLAG_REX_PREFIX    = (1 << 4);
constexpr uint32_t FLAG_VSIB_BYTE     = (1 << 5);
constexpr uint32_t FLAG_OPTION_AVX_W  = (1 << 6);
constexpr uint32_t FLAG_REX_WIDENING  = (1 << 7);
constexpr uint32_t FLAG_REX_XGPR_B    = (1 << 8);
constexpr uint32_t FLAG_REX_XGPR_X    = (1 << 9);
constexpr uint32_t FLAG_REX_XGPR_R    = (1 << 10);
constexpr uint32_t FLAG_NO_PREFIX     = (0b000 << 11);
constexpr uint32_t FLAG_ES_PREFIX     = (0b001 << 11);
constexpr uint32_t FLAG_CS_PREFIX     = (0b010 << 11);
constexpr uint32_t FLAG_SS_PREFIX     = (0b011 << 11);
constexpr uint32_t FLAG_DS_PREFIX     = (0b100 << 11);
constexpr uint32_t FLAG_FS_PREFIX     = (0b101 << 11);
constexpr uint32_t FLAG_GS_PREFIX     = (0b110 << 11);
constexpr uint32_t FLAG_SEGMENTS      = (0b111 << 11);
constexpr uint32_t FLAG_FORCE_TSO     = (1 << 14);
constexpr uint32_t FLAG_DECODED_MODRM = (1 << 15);
constexpr uint32_t FLAG_DECODED_SIB   = (1 << 16);
constexpr uint32_t FLAG_REP_PREFIX    = (1 << 17);
constexpr uint32_t FLAG_REPNE_PREFIX  = (1 << 18);
// Size flags
constexpr uint32_t FLAG_SIZE_DST_OFF = 19;
constexpr uint32_t FLAG_SIZE_SRC_OFF = FLAG_SIZE_DST_OFF + 3;
constexpr uint32_t SIZE_MASK         = 0b111;
constexpr uint32_t SIZE_DEF          = 0b000; // This should be invalid past decoding
constexpr uint32_t SIZE_8BIT         = 0b001;
constexpr uint32_t SIZE_16BIT        = 0b010;
constexpr uint32_t SIZE_32BIT        = 0b011;
constexpr uint32_t SIZE_64BIT        = 0b100;
constexpr uint32_t SIZE_128BIT       = 0b101;
constexpr uint32_t SIZE_256BIT       = 0b110;

constexpr uint32_t FLAG_OPADDR_OFF = (FLAG_SIZE_SRC_OFF + 3);
constexpr uint32_t FLAG_OPADDR_STACKSIZE = 4; // Two level deep stack
constexpr uint32_t FLAG_OPADDR_FLAG_SIZE = 2;
constexpr uint32_t FLAG_OPADDR_MASK = (((1 << FLAG_OPADDR_STACKSIZE) - 1) << FLAG_OPADDR_OFF);

// 00 = NONE
constexpr uint32_t FLAG_OPERAND_SIZE_LAST = 0b01;
constexpr uint32_t FLAG_WIDENING_SIZE_LAST = 0b10;

constexpr uint32_t GetSizeDstFlags(uint32_t Flags) { return (Flags >> FLAG_SIZE_DST_OFF) & SIZE_MASK; }
constexpr uint32_t GetSizeSrcFlags(uint32_t Flags) { return (Flags >> FLAG_SIZE_SRC_OFF) & SIZE_MASK; }

constexpr uint32_t GenSizeDstSize(uint32_t Size) { return Size << FLAG_SIZE_DST_OFF; }
constexpr uint32_t GenSizeSrcSize(uint32_t Size) { return Size << FLAG_SIZE_SRC_OFF; }

constexpr uint32_t GetOpAddr(uint32_t Flags, uint32_t Index) {
  return (((Flags & FLAG_OPADDR_MASK) >> FLAG_OPADDR_OFF) >> (Index * 2)) & ((1 << FLAG_OPADDR_FLAG_SIZE) - 1);
}

inline void PushOpAddr(uint32_t *Flags, uint32_t Flag) {
  uint32_t TmpFlags  = *Flags;
  uint32_t BottomOfStack = ((TmpFlags & FLAG_OPADDR_MASK) >> FLAG_OPADDR_OFF) & ((1 << FLAG_OPADDR_FLAG_SIZE) - 1);

  TmpFlags &= ~(FLAG_OPADDR_MASK);
  TmpFlags |=
    (BottomOfStack << (FLAG_OPADDR_OFF + FLAG_OPADDR_FLAG_SIZE)) |
    (Flag << FLAG_OPADDR_OFF);

  *Flags = TmpFlags;
}

inline void PopOpAddrIf(uint32_t *Flags, uint32_t Flag) {
  uint32_t TmpFlags  = *Flags;
  uint32_t BottomOfStack = ((TmpFlags & FLAG_OPADDR_MASK) >> FLAG_OPADDR_OFF) & ((1 << FLAG_OPADDR_FLAG_SIZE) - 1);

  // Only pop the stack if the bottom flag is the one we care about
  // Necessary for escape prefixes that overlap regular prefixes
  if (BottomOfStack != Flag) {
    return;
  }

  uint32_t TopOfStack = ((TmpFlags & FLAG_OPADDR_MASK) >> (FLAG_OPADDR_OFF + FLAG_OPADDR_FLAG_SIZE)) & ((1 << FLAG_OPADDR_FLAG_SIZE) - 1);

  TmpFlags &= ~(FLAG_OPADDR_MASK);
  TmpFlags |= (TopOfStack << FLAG_OPADDR_OFF);

  *Flags = TmpFlags;
}

}

struct DecodedOperand {
  enum class OpType : uint8_t {
    Nothing,
    GPR,
    GPRDirect,
    GPRIndirect,
    RIPRelative,
    Literal,
    SIB,
  };

  bool IsNone() const {
    return Type == OpType::Nothing;
  }
  bool IsGPR() const {
    return Type == OpType::GPR;
  }
  bool IsGPRDirect() const {
    return Type == OpType::GPRDirect;
  }
  bool IsGPRIndirect() const {
    return Type == OpType::GPRIndirect;
  }
  bool IsRIPRelative() const {
    return Type == OpType::RIPRelative;
  }
  bool IsLiteral() const {
    return Type == OpType::Literal;
  }
  bool IsSIB() const {
    return Type == OpType::SIB;
  }
  uint64_t Literal() const {
    LOGMAN_THROW_A_FMT(IsLiteral(), "Precondition: must be a literal");
    if (Data.Literal.SignExtend) {
      return static_cast<int64_t>(static_cast<int32_t>(Data.Literal.Value));
    }
    return Data.Literal.Value;
  }

  union TypeUnion {
    struct GPRType {
      bool HighBits;
      uint8_t GPR;
      auto operator<=>(const GPRType&) const = default;
    } GPR;

    struct {
      int32_t Displacement;
      uint8_t GPR;
    } GPRIndirect;

    struct {
      union {
        int32_t s;
        uint32_t u;
      } Value;
    } RIPLiteral;

    struct LiteralType {
      uint32_t Value;
      uint8_t Size : 7 ;
      bool SignExtend : 1;
      auto operator<=>(const LiteralType&) const = default;
    } Literal;

    struct {
      int32_t Offset;
      uint8_t Scale;
      uint8_t Index; // ~0 invalid
      uint8_t Base; // ~0 invalid
    } SIB;
  };

  TypeUnion Data;
  OpType Type;
};

struct DecodedInst {
  uint64_t PC;

  DecodedOperand Dest;
  DecodedOperand Src[3];

  // Constains the dispatcher handler pointer
  X86InstInfo const* TableInfo;

  uint32_t Flags;
  uint16_t OP;
  uint8_t OPRaw;

  uint8_t ModRM;
  uint8_t SIB;
  uint8_t InstSize;
};

union ModRMDecoded {
  uint8_t Hex{};
  struct {
    uint8_t rm : 3;
    uint8_t reg : 3;
    uint8_t mod : 2;
  };
};

union SIBDecoded {
  uint8_t Hex{};
  struct {
    uint8_t base : 3;
    uint8_t index : 3;
    uint8_t scale : 2;
  };
};

enum InstType {
  TYPE_UNKNOWN,
  TYPE_LEGACY_PREFIX,
  TYPE_PREFIX,
  TYPE_REX_PREFIX,
  TYPE_SECONDARY_TABLE_PREFIX,
  TYPE_X87_TABLE_PREFIX,
  TYPE_VEX_TABLE_PREFIX,
  TYPE_INST,
  TYPE_X87 = TYPE_INST,
  TYPE_INVALID,
  TYPE_COPY_OTHER,
  // Changes `X86InstInfo::OpcodeDispatcher` member to use the `Indirect` version.
  // Points to a 2 member array of X86InstInfo to choose instruction description based on executing bitness.
  TYPE_ARCH_DISPATCHER,

  // Must be in order
  // Groups 1, 1a, 2, 3, 4, 5, 11 are for the primary op table
  // Groups 6, 7, 8, 9, 10, 12, 13, 14, 15, 16, 17, p are for the secondary op table
  TYPE_GROUP_1,
  TYPE_GROUP_1A,
  TYPE_GROUP_2,
  TYPE_GROUP_3,
  TYPE_GROUP_4,
  TYPE_GROUP_5,
  TYPE_GROUP_11,

  // Must be in order
  // Groups 6-p Are for the secondary op table
  TYPE_GROUP_6,
  TYPE_GROUP_7,
  TYPE_GROUP_8,
  TYPE_GROUP_9,
  TYPE_GROUP_10,
  TYPE_GROUP_12,
  TYPE_GROUP_13,
  TYPE_GROUP_14,
  TYPE_GROUP_15,
  TYPE_GROUP_16,
  TYPE_GROUP_17,
  TYPE_GROUP_P,

  // The secondary op extension table allows further extensions
  // Group 7 allows additional extensions to this table
  TYPE_SECOND_GROUP_MODRM,

  TYPE_VEX_GROUP_12,
  TYPE_VEX_GROUP_13,
  TYPE_VEX_GROUP_14,
  TYPE_VEX_GROUP_15,
  TYPE_VEX_GROUP_17,

  TYPE_GROUP_EVEX,

  // Exists in the table but isn't decoded correctly
  TYPE_UNDEC = TYPE_INVALID,
  TYPE_MMX = TYPE_INVALID,
  TYPE_PRIV = TYPE_INVALID,
  TYPE_0F38_TABLE = TYPE_INVALID,
  TYPE_0F3A_TABLE = TYPE_INVALID,
  TYPE_3DNOW_TABLE = TYPE_INVALID,
};

namespace InstFlags {

using InstFlagType = uint64_t;

constexpr InstFlagType FLAGS_NONE                  = 0;
// The secondary Opcode Map uses prefix bytes to overlay more instruction
// But some instructions need to ignore this overlay and consume these prefixes.
constexpr InstFlagType FLAGS_NO_OVERLAY            = (1ULL << 0);
// Some instructions partially ignore overlay
// Ignore OpSize (0x66) in this case
constexpr InstFlagType FLAGS_NO_OVERLAY66          = (1ULL << 1);
constexpr InstFlagType FLAGS_DEBUG_MEM_ACCESS      = (1ULL << 2);
// Only SEXT if the instruction is operating in 64bit operand size
constexpr InstFlagType FLAGS_SRC_SEXT64BIT         = (1ULL << 3);
constexpr InstFlagType FLAGS_BLOCK_END             = (1ULL << 4);
constexpr InstFlagType FLAGS_SETS_RIP              = (1ULL << 5);

constexpr InstFlagType FLAGS_DISPLACE_SIZE_MUL_2   = (1ULL << 6);
constexpr InstFlagType FLAGS_DISPLACE_SIZE_DIV_2   = (1ULL << 7);
constexpr InstFlagType FLAGS_SRC_SEXT              = (1ULL << 8);
constexpr InstFlagType FLAGS_MEM_OFFSET            = (1ULL << 9);

// Enables XMM based subflags
// Current reserved range for this SF is [10, 15]
constexpr InstFlagType FLAGS_XMM_FLAGS             = (1ULL << 10);

// X87 flags aliased to XMM flags selection
// Allows X87 instruction table that is abusing the flag for 64BIT selection to work
constexpr InstFlagType FLAGS_X87_FLAGS             = (1ULL << 10);

  // Non-XMM subflags
  constexpr InstFlagType FLAGS_SF_DST_RAX               = (1ULL << 11);
  constexpr InstFlagType FLAGS_SF_DST_RDX               = (1ULL << 12);
  constexpr InstFlagType FLAGS_SF_SRC_RAX               = (1ULL << 13);
  constexpr InstFlagType FLAGS_SF_SRC_RCX               = (1ULL << 14);
  constexpr InstFlagType FLAGS_SF_REX_IN_BYTE           = (1ULL << 15);

  // XMM subflags
  constexpr InstFlagType FLAGS_SF_UNUSED             = (1ULL << 11); // No assigned behavior yet
  constexpr InstFlagType FLAGS_SF_DST_GPR            = (1ULL << 12);
  constexpr InstFlagType FLAGS_SF_SRC_GPR            = (1ULL << 13);
  constexpr InstFlagType FLAGS_SF_MMX_DST            = (1ULL << 14);
  constexpr InstFlagType FLAGS_SF_MMX_SRC            = (1ULL << 15);
  constexpr InstFlagType FLAGS_SF_MMX                = FLAGS_SF_MMX_DST | FLAGS_SF_MMX_SRC;

// Enables MODRM specific subflags
// Current reserved range for this SF is [14, 17]
constexpr InstFlagType FLAGS_MODRM                 = (1ULL << 16);

  // With ModRM SF flag enabled
  // Direction of ModRM. Dst ^ Src
  // Set means destination is rm bits
  // Unset means src is rm bits
  constexpr InstFlagType FLAGS_SF_MOD_DST            = (1ULL << 17);

  // If the instruction is restricted to mem or reg only
  // 0b00 = Regular ModRM support
  // 0b01 = Memory accesses only
  // 0b10 = Register accesses only
  // 0b11 = <Reserved>
  constexpr InstFlagType FLAGS_SF_MOD_MEM_ONLY       = (1ULL << 18);
  constexpr InstFlagType FLAGS_SF_MOD_REG_ONLY       = (1ULL << 19);

  constexpr InstFlagType FLAGS_SF_MOD_ZERO_REG       = (1ULL << 20);

// x87
constexpr InstFlagType FLAGS_POP                  = (1ULL << 21);

// Whether or not the instruction has a VEX prefix for the dest, first, or second source.
constexpr InstFlagType FLAGS_VEX_SRC_MASK         = (0b11ULL << 22);
constexpr InstFlagType FLAGS_VEX_NO_OPERAND       = (0b00ULL << 22);
constexpr InstFlagType FLAGS_VEX_DST              = (0b01ULL << 22);
constexpr InstFlagType FLAGS_VEX_1ST_SRC          = (0b10ULL << 22);
constexpr InstFlagType FLAGS_VEX_2ND_SRC          = (0b11ULL << 22);
// Whether or not the instruction has a VSIB byte
constexpr InstFlagType FLAGS_VEX_VSIB             = (1ULL << 24);
constexpr InstFlagType FLAGS_VEX_L_IGNORE         = (1ULL << 25);
constexpr InstFlagType FLAGS_VEX_L_0             = (1ULL << 26);
constexpr InstFlagType FLAGS_VEX_L_1             = (1ULL << 27);

constexpr InstFlagType FLAGS_REX_W_0             = (1ULL << 28);
constexpr InstFlagType FLAGS_REX_W_1             = (1ULL << 29);

constexpr InstFlagType FLAGS_CALL             = (1ULL << 30);

constexpr InstFlagType FLAGS_SIZE_DST_OFF = 58;
constexpr InstFlagType FLAGS_SIZE_SRC_OFF = FLAGS_SIZE_DST_OFF + 3;

constexpr InstFlagType SIZE_MASK     = 0b111;
constexpr InstFlagType SIZE_DEF      = 0b000;
constexpr InstFlagType SIZE_8BIT     = 0b001;
constexpr InstFlagType SIZE_16BIT    = 0b010;
constexpr InstFlagType SIZE_32BIT    = 0b011;
constexpr InstFlagType SIZE_64BIT    = 0b100;
constexpr InstFlagType SIZE_128BIT   = 0b101;
constexpr InstFlagType SIZE_256BIT   = 0b110;
constexpr InstFlagType SIZE_64BITDEF = 0b111; // Default mode is 64bit instead of typical 32bit

#ifndef _WIN32
  constexpr uint32_t DEFAULT_SYSCALL_FLAGS = FLAGS_NO_OVERLAY;
#else
  // Syscall ends a block on WIN32 because the instruction can update the CPU's RIP.
  constexpr uint32_t DEFAULT_SYSCALL_FLAGS = FLAGS_NO_OVERLAY | FLAGS_BLOCK_END;
#endif

constexpr InstFlagType GetSizeDstFlags(InstFlagType Flags) { return (Flags >> FLAGS_SIZE_DST_OFF) & SIZE_MASK; }
constexpr InstFlagType GetSizeSrcFlags(InstFlagType Flags) { return (Flags >> FLAGS_SIZE_SRC_OFF) & SIZE_MASK; }

constexpr InstFlagType GenFlagsDstSize(InstFlagType Size) { return Size << FLAGS_SIZE_DST_OFF; }
constexpr InstFlagType GenFlagsSrcSize(InstFlagType Size) { return Size << FLAGS_SIZE_SRC_OFF; }
constexpr InstFlagType GenFlagsSameSize(InstFlagType Size) { return (Size << FLAGS_SIZE_DST_OFF) | (Size << FLAGS_SIZE_SRC_OFF); }
constexpr InstFlagType GenFlagsSizes(InstFlagType Dest, InstFlagType Src) { return (Dest << FLAGS_SIZE_DST_OFF) | (Src << FLAGS_SIZE_SRC_OFF); }

// If it has an xmm subflag
#define HAS_XMM_SUBFLAG(x, flag) (((x) & (FEXCore::X86Tables::InstFlags::FLAGS_XMM_FLAGS | (flag))) == (FEXCore::X86Tables::InstFlags::FLAGS_XMM_FLAGS | (flag)))

// If it has non-xmm subflag
#define HAS_NON_XMM_SUBFLAG(x, flag) (((x) & (FEXCore::X86Tables::InstFlags::FLAGS_XMM_FLAGS | (flag))) == (flag))
}

constexpr uint8_t OpToIndex(uint8_t Op) {
  switch (Op) {
  // Group 1
  case 0x80: return 0;
  case 0x81: return 1;
  case 0x82: return 2;
  case 0x83: return 3;
  // Group 2
  case 0xC0: return 0;
  case 0xC1: return 1;
  case 0xD0: return 2;
  case 0xD1: return 3;
  case 0xD2: return 4;
  case 0xD3: return 5;
  // Group 3
  case 0xF6: return 0;
  case 0xF7: return 1;
  // Group 4
  case 0xFE: return 0;
  // Group 5
  case 0xFF: return 0;
  // Group 11
  case 0xC6: return 0;
  case 0xC7: return 1;
  }
  return 0;
}

using DecodedOp = DecodedInst const*;
using OpDispatchPtr = void (IR::OpDispatchBuilder::*)(DecodedOp);

union OpDispatchPtrWrapper {
  OpDispatchPtr OpDispatch;
  const struct X86InstInfo *Indirect;
};

struct X86InstInfo {
  char const *Name;
  InstType Type;
  InstFlags::InstFlagType Flags; ///< Must be larger than InstFlags enum
  uint8_t MoreBytes;
  OpDispatchPtrWrapper OpcodeDispatcher;

  bool operator==(const X86InstInfo &b) const {
    if (strcmp(Name, b.Name) != 0 ||
        Type != b.Type ||
        Flags != b.Flags ||
        MoreBytes != b.MoreBytes)
      return false;

    // We don't care if the opcode dispatcher differs
    return true;
  }
  bool operator!=(const X86InstInfo &b) const {
    return !operator==(b);
  }
};

static_assert(std::is_trivially_copyable_v<X86InstInfo>);

constexpr size_t MAX_PRIMARY_TABLE_SIZE = 256;
constexpr size_t MAX_SECOND_TABLE_SIZE = 256;
constexpr size_t MAX_REP_MOD_TABLE_SIZE = 256;
constexpr size_t MAX_REPNE_MOD_TABLE_SIZE = 256;
constexpr size_t MAX_OPSIZE_MOD_TABLE_SIZE = 256;
// 6 (groups) | 6 (max indexes) | 8 ops = 0b111'111'111 = 9 bits
constexpr size_t MAX_INST_GROUP_TABLE_SIZE = 512;
// 12 (groups) | 3(max indexes) | 8 ops = 0b1111'11'111 = 9 bits
constexpr size_t MAX_INST_SECOND_GROUP_TABLE_SIZE = 512;
constexpr size_t MAX_X87_TABLE_SIZE = 1 << 11;
constexpr size_t MAX_SECOND_MODRM_TABLE_SIZE = 32;
// (3 bit prefixes) | 8 bit opcode
constexpr size_t MAX_0F_38_TABLE_SIZE = (1 << 11);
// 1 REX | 1 prefixes | 8 bit opcode
constexpr size_t MAX_0F_3A_TABLE_SIZE = (1 << 11);
constexpr size_t MAX_3DNOW_TABLE_SIZE = 256;
// VEX
// map_select(2 bits for now) | vex.pp (2 bits) | opcode (8bit)
constexpr size_t MAX_VEX_TABLE_SIZE = (1 << 13);
// VEX group ops
// group select (3 bits for now) | ModRM opcode (3 bits)
constexpr size_t MAX_VEX_GROUP_TABLE_SIZE = (1 << 7);

extern const std::array<X86InstInfo, MAX_PRIMARY_TABLE_SIZE> BaseOps;
extern const std::array<X86InstInfo, MAX_SECOND_TABLE_SIZE> SecondBaseOps;
extern const std::array<X86InstInfo, MAX_REP_MOD_TABLE_SIZE> RepModOps;
extern const std::array<X86InstInfo, MAX_REPNE_MOD_TABLE_SIZE> RepNEModOps;
extern const std::array<X86InstInfo, MAX_OPSIZE_MOD_TABLE_SIZE> OpSizeModOps;

extern const std::array<X86InstInfo, MAX_INST_GROUP_TABLE_SIZE> PrimaryInstGroupOps;
extern const std::array<X86InstInfo, MAX_INST_SECOND_GROUP_TABLE_SIZE> SecondInstGroupOps;
extern const std::array<X86InstInfo, MAX_SECOND_MODRM_TABLE_SIZE> SecondModRMTableOps;
extern const std::array<X86InstInfo, MAX_X87_TABLE_SIZE> X87F80Ops;
extern const std::array<X86InstInfo, MAX_X87_TABLE_SIZE> X87F64Ops;
extern const std::array<X86InstInfo, MAX_3DNOW_TABLE_SIZE> DDDNowOps;
extern const std::array<X86InstInfo, MAX_0F_38_TABLE_SIZE> H0F38TableOps;
extern const std::array<X86InstInfo, MAX_0F_3A_TABLE_SIZE> H0F3ATableOps;

// VEX
extern const std::array<X86InstInfo, MAX_VEX_TABLE_SIZE> VEXTableOps;
extern const std::array<X86InstInfo, MAX_VEX_GROUP_TABLE_SIZE> VEXTableGroupOps;

extern const std::array<X86InstInfo, MAX_VEX_TABLE_SIZE> VEXTableOps_AVX128;
extern const std::array<X86InstInfo, MAX_VEX_GROUP_TABLE_SIZE> VEXTableGroupOps_AVX128;

template <typename OpcodeType>
struct X86TablesInfoStruct {
  OpcodeType first;
  uint8_t second;
  X86InstInfo Info;
};
using U8U8InfoStruct = X86TablesInfoStruct<uint8_t>;
using U16U8InfoStruct = X86TablesInfoStruct<uint16_t>;

template<typename OpcodeType>
constexpr static inline void GenerateTable(X86InstInfo *FinalTable, X86TablesInfoStruct<OpcodeType> const *LocalTable, size_t TableSize) {
  for (size_t j = 0; j < TableSize; ++j) {
    X86TablesInfoStruct<OpcodeType> const &Op = LocalTable[j];
    auto OpNum = Op.first;
    X86InstInfo const &Info = Op.Info;
    for (uint32_t i = 0; i < Op.second; ++i) {
      if (FinalTable[OpNum + i].Type != TYPE_UNKNOWN) {
        LOGMAN_MSG_A_FMT("Duplicate Entry {}->{}", FinalTable[OpNum + i].Name, Info.Name);
      }
      if (FinalTable[OpNum + i].OpcodeDispatcher.OpDispatch) {
        LOGMAN_MSG_A_FMT("Already installed an OpcodeDispatcher for 0x{:x}", OpNum + i);
      }
      FinalTable[OpNum + i] = Info;
    }
  }
};

template<typename OpcodeType>
constexpr static inline void GenerateTableWithCopy(X86InstInfo *FinalTable, X86TablesInfoStruct<OpcodeType> const *LocalTable, size_t TableSize, const X86InstInfo *OtherLocal) {
  for (size_t j = 0; j < TableSize; ++j) {
    X86TablesInfoStruct<OpcodeType> const &Op = LocalTable[j];
    auto OpNum = Op.first;
    X86InstInfo const &Info = Op.Info;
    for (uint32_t i = 0; i < Op.second; ++i) {
      if (FinalTable[OpNum + i].Type != TYPE_UNKNOWN) {
        LOGMAN_MSG_A_FMT("Duplicate Entry {}->{}", FinalTable[OpNum + i].Name, Info.Name);
      }
      if (Info.Type == TYPE_COPY_OTHER) {
        FinalTable[OpNum + i] = OtherLocal[OpNum + i];
      }
      else {
        FinalTable[OpNum + i] = Info;
      }
    }
  }
};

template<typename OpcodeType>
static inline void LateInitCopyTable(X86InstInfo *FinalTable, X86TablesInfoStruct<OpcodeType> const *OtherLocal, size_t OtherTableSize) {
  for (size_t j = 0; j < OtherTableSize; ++j) {
    X86TablesInfoStruct<OpcodeType> const &OtherOp = OtherLocal[j];
    auto OtherOpNum = OtherOp.first;
    X86InstInfo const &OtherInfo = OtherOp.Info;
    for (uint32_t i = 0; i < OtherOp.second; ++i) {
      X86InstInfo &FinalOp = FinalTable[OtherOpNum + i];
      if (FinalOp.Type == TYPE_COPY_OTHER) {
        FinalOp = OtherInfo;
      }
    }
  }
}

template<typename OpcodeType>
constexpr static inline void GenerateX87Table(X86InstInfo *FinalTable, X86TablesInfoStruct<OpcodeType> const *LocalTable, size_t TableSize) {
  for (size_t j = 0; j < TableSize; ++j) {
    X86TablesInfoStruct<OpcodeType> const &Op = LocalTable[j];
    auto OpNum = Op.first;
    X86InstInfo const &Info = Op.Info;
    for (uint32_t i = 0; i < Op.second; ++i) {
      if (FinalTable[OpNum + i].Type != TYPE_UNKNOWN) {
        LOGMAN_MSG_A_FMT("Duplicate Entry {}->{}", FinalTable[OpNum + i].Name, Info.Name);
      }
      if ((OpNum & 0b11'000'000) == 0b11'000'000) {
        // If the mod field is 0b11 then it is a regular op
        FinalTable[OpNum + i] = Info;
      }
      else {
        // If the mod field is !0b11 then this instruction is duplicated through the whole mod [0b00, 0b10] range
        // and the modrm.rm space because that is used part of the instruction encoding
        if ((OpNum & 0b11'000'000) != 0) {
          ERROR_AND_DIE_FMT("Only support mod field of zero in this path");
        }
        for (uint16_t mod = 0b00'000'000; mod < 0b11'000'000; mod += 0b01'000'000) {
          for (uint16_t rm = 0b000; rm < 0b1'000; ++rm) {
            FinalTable[(OpNum | mod | rm) + i] = Info;
          }
        }
      }
    }
  }
};

}

template <>
struct fmt::formatter<FEXCore::X86Tables::DecodedOperand::OpType> : formatter<uint32_t> {
  template <typename FormatContext>
  auto format(FEXCore::X86Tables::DecodedOperand::OpType type, FormatContext& ctx) const {
    return fmt::formatter<uint32_t>::format(static_cast<uint32_t>(type), ctx);
  }
};
