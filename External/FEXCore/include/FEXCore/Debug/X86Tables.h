#pragma once

#include <FEXCore/Core/Context.h>

#include <cstdint>
#include <cstring>
#include <type_traits>

namespace FEXCore::IR {
///< Forward declaration of OpDispatchBuilder
class OpDispatchBuilder;
}

namespace FEXCore::X86Tables {

///< Forward declaration of X86InstInfo
struct X86InstInfo;

namespace DecodeFlags {
constexpr uint32_t FLAG_OPERAND_SIZE  = (1 << 0);
constexpr uint32_t FLAG_ADDRESS_SIZE  = (1 << 1);
constexpr uint32_t FLAG_LOCK          = (1 << 2);
constexpr uint32_t FLAG_LEGACY_PREFIX = (1 << 3);
constexpr uint32_t FLAG_REX_PREFIX    = (1 << 4);
constexpr uint32_t FLAG_MODRM_PRESENT = (1 << 5);
constexpr uint32_t FLAG_SIB_PRESENT   = (1 << 6);
constexpr uint32_t FLAG_REX_WIDENING  = (1 << 7);
constexpr uint32_t FLAG_REX_XGPR_B    = (1 << 8);
constexpr uint32_t FLAG_REX_XGPR_X    = (1 << 9);
constexpr uint32_t FLAG_REX_XGPR_R    = (1 << 10);
constexpr uint32_t FLAG_ES_PREFIX     = (1 << 11);
constexpr uint32_t FLAG_CS_PREFIX     = (1 << 12);
constexpr uint32_t FLAG_SS_PREFIX     = (1 << 13);
constexpr uint32_t FLAG_DS_PREFIX     = (1 << 14);
constexpr uint32_t FLAG_FS_PREFIX     = (1 << 15);
constexpr uint32_t FLAG_GS_PREFIX     = (1 << 16);
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
constexpr uint32_t FLAG_OPADDR_MASK = (0b11 << FLAG_OPADDR_OFF);

constexpr uint32_t FLAG_OPERAND_SIZE_LAST = (0b01 << FLAG_OPADDR_OFF);
constexpr uint32_t FLAG_WIDENING_SIZE_LAST = (0b10 << FLAG_OPADDR_OFF);

inline uint32_t GetSizeDstFlags(uint32_t Flags) { return (Flags >> FLAG_SIZE_DST_OFF) & SIZE_MASK; }
inline uint32_t GetSizeSrcFlags(uint32_t Flags) { return (Flags >> FLAG_SIZE_SRC_OFF) & SIZE_MASK; }

inline uint32_t GenSizeDstSize(uint32_t Size) { return Size << FLAG_SIZE_DST_OFF; }
inline uint32_t GenSizeSrcSize(uint32_t Size) { return Size << FLAG_SIZE_SRC_OFF; }

inline uint32_t GetOpAddr(uint32_t Flags) { return Flags & FLAG_OPADDR_MASK; }
}

union DecodedOperand {
  enum {
    TYPE_NONE,
    TYPE_GPR,
    TYPE_GPR_DIRECT,
    TYPE_GPR_INDIRECT,
    TYPE_RIP_RELATIVE,
    TYPE_LITERAL,
    TYPE_SIB,
  };

  struct {
    uint8_t Type;
  } TypeNone;

  struct {
    uint8_t Type;
    bool HighBits;
    uint8_t GPR;
  } TypeGPR;

  struct {
    uint8_t Type;
    uint8_t GPR;
    int32_t Displacement;
  } TypeGPRIndirect;

  struct {
    uint8_t Type;
    int32_t Literal;
  } TypeRIPLiteral;

  struct {
    uint8_t Type;
    uint8_t Size;
    uint64_t Literal;
  } TypeLiteral;

  struct {
    uint8_t Type;
    uint8_t Index; // ~0 invalid
    uint8_t Base; // ~0 invalid
    uint32_t Scale  : 8;
    int32_t Offset;
  } TypeSIB;
};

struct DecodedInst {
  uint64_t PC;

  uint16_t OP;
  uint32_t Flags;

  uint8_t ModRM;
  uint8_t SIB;
  uint8_t InstSize;
  uint8_t LastEscapePrefix;
  bool DecodedModRM;
  bool DecodedSIB;

  DecodedOperand Dest;
  DecodedOperand Src[2];

  // Constains the dispatcher handler pointer
  X86InstInfo const* TableInfo;
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
  TYPE_XOP_TABLE_PREFIX,
  TYPE_INST,
  TYPE_X87 = TYPE_INST,
  TYPE_INVALID,
  TYPE_COPY_OTHER,

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

  // Just to make grepping easier
  TYPE_3DNOW_TABLE = TYPE_INVALID,
  TYPE_3DNOW_INST = TYPE_INVALID,

  // Exists in the table but isn't decoded correctly
  TYPE_UNDEC = TYPE_INVALID,
  TYPE_MMX = TYPE_INVALID,
  TYPE_PRIV = TYPE_INVALID,
  TYPE_0F38_TABLE = TYPE_INVALID,
  TYPE_0F3A_TABLE = TYPE_INVALID,
};

namespace InstFlags {
constexpr uint32_t FLAGS_NONE                  = 0;
constexpr uint32_t FLAGS_DEBUG                 = (1 << 1);
constexpr uint32_t FLAGS_DEBUG_MEM_ACCESS      = (1 << 2);
constexpr uint32_t FLAGS_SUPPORTS_REP          = (1 << 3);
constexpr uint32_t FLAGS_BLOCK_END             = (1 << 4);
constexpr uint32_t FLAGS_SETS_RIP              = (1 << 5);

constexpr uint32_t FLAGS_DISPLACE_SIZE_MUL_2   = (1 << 6);
constexpr uint32_t FLAGS_DISPLACE_SIZE_DIV_2   = (1 << 7);
constexpr uint32_t FLAGS_SRC_SEXT              = (1 << 8);
constexpr uint32_t FLAGS_MEM_OFFSET            = (1 << 9);

// Enables XMM based subflags
// Current reserved range for this SF is [10, 15]
constexpr uint32_t FLAGS_XMM_FLAGS             = (1 << 10);

  // Non-XMM subflags
  constexpr uint32_t FLAGS_SF_DST_RAX               = (1 << 11);
  constexpr uint32_t FLAGS_SF_DST_RDX               = (1 << 12);
  constexpr uint32_t FLAGS_SF_SRC_RAX               = (1 << 13);
  constexpr uint32_t FLAGS_SF_SRC_RCX               = (1 << 14);
  constexpr uint32_t FLAGS_SF_REX_IN_BYTE           = (1 << 15);

  // XMM subflags
  constexpr uint32_t FLAGS_SF_HIGH_XMM_REG       = (1 << 11);
  constexpr uint32_t FLAGS_SF_DST_GPR            = (1 << 12);
  constexpr uint32_t FLAGS_SF_SRC_GPR            = (1 << 13);
  constexpr uint32_t FLAGS_SF_MMX                = (3 << 14); // MMX_DST | MMX_SRC
  constexpr uint32_t FLAGS_SF_MMX_DST            = (1 << 14);
  constexpr uint32_t FLAGS_SF_MMX_SRC            = (1 << 15);

// Enables MODRM specific subflags
// Current reserved range for this SF is [14, 17]
constexpr uint32_t FLAGS_MODRM                 = (1 << 16);

  // With ModRM SF flag enabled
  // Direction of ModRM. Dst ^ Src
  // Set means destination is rm bits
  // Unset means src is rm bits
  constexpr uint32_t FLAGS_SF_MOD_DST            = (1 << 17);

  // If the instruction is restricted to mem or reg only
  // 0b00 = Regular ModRM support
  // 0b01 = Memory accesses only
  // 0b10 = Register accesses only
  // 0b11 = <Reserved>
  constexpr uint32_t FLAGS_SF_MOD_MEM_ONLY       = (1 << 18);
  constexpr uint32_t FLAGS_SF_MOD_REG_ONLY       = (1 << 19);

// The secondary Opcode Map uses prefix bytes to overlay more instruction
// But some instructions need to ignore this overlay and consume these prefixes.
constexpr uint32_t FLAGS_NO_OVERLAY           = (1 << 20);
// Some instructions partially ignore overlay
// Ignore OpSize (0x66) in this case
constexpr uint32_t FLAGS_NO_OVERLAY66         = (1 << 21);

// x87
constexpr uint32_t FLAGS_POP                  = (1 << 22);

// Only SEXT if the instruction is operating in 64bit operand size
constexpr uint32_t FLAGS_SRC_SEXT64BIT        = (1 << 23);

constexpr uint32_t FLAGS_SIZE_DST_OFF = 26;
constexpr uint32_t FLAGS_SIZE_SRC_OFF = FLAGS_SIZE_DST_OFF + 3;

constexpr uint32_t SIZE_MASK     = 0b111;
constexpr uint32_t SIZE_DEF      = 0b000;
constexpr uint32_t SIZE_8BIT     = 0b001;
constexpr uint32_t SIZE_16BIT    = 0b010;
constexpr uint32_t SIZE_32BIT    = 0b011;
constexpr uint32_t SIZE_64BIT    = 0b100;
constexpr uint32_t SIZE_128BIT   = 0b101;
constexpr uint32_t SIZE_256BIT   = 0b110;
constexpr uint32_t SIZE_64BITDEF = 0b111; // Default mode is 64bit instead of typical 32bit

inline uint32_t GetSizeDstFlags(uint32_t Flags) { return (Flags >> FLAGS_SIZE_DST_OFF) & SIZE_MASK; }
inline uint32_t GetSizeSrcFlags(uint32_t Flags) { return (Flags >> FLAGS_SIZE_SRC_OFF) & SIZE_MASK; }

inline uint32_t GenFlagsDstSize(uint32_t Size) { return Size << FLAGS_SIZE_DST_OFF; }
inline uint32_t GenFlagsSrcSize(uint32_t Size) { return Size << FLAGS_SIZE_SRC_OFF; }
inline uint32_t GenFlagsSameSize(uint32_t Size) {return (Size << FLAGS_SIZE_DST_OFF) | (Size << FLAGS_SIZE_SRC_OFF); }
inline uint32_t GenFlagsSizes(uint32_t Dest, uint32_t Src) {return (Dest << FLAGS_SIZE_DST_OFF) | (Src << FLAGS_SIZE_SRC_OFF); }


// If it has an xmm subflag
#define HAS_XMM_SUBFLAG(x, flag) (((x) & (FEXCore::X86Tables::InstFlags::FLAGS_XMM_FLAGS | (flag))) == (FEXCore::X86Tables::InstFlags::FLAGS_XMM_FLAGS | (flag)))

// If it has non-xmm subflag
#define HAS_NON_XMM_SUBFLAG(x, flag) (((x) & (FEXCore::X86Tables::InstFlags::FLAGS_XMM_FLAGS | (flag))) == (flag))
}

auto OpToIndex = [](uint8_t Op) constexpr -> uint8_t {
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
};

using DecodedOp = DecodedInst const*;
using OpDispatchPtr = void (IR::OpDispatchBuilder::*)(DecodedOp);

#ifndef NDEBUG
namespace X86InstDebugInfo {
constexpr uint64_t FLAGS_MEM_ALIGN_4    = (1 << 0);
constexpr uint64_t FLAGS_MEM_ALIGN_8    = (1 << 1);
constexpr uint64_t FLAGS_MEM_ALIGN_16   = (1 << 2);
constexpr uint64_t FLAGS_MEM_ALIGN_SIZE = (1 << 3); // If instruction size changes depending on prefixes
constexpr uint64_t FLAGS_MEM_ACCESS     = (1 << 4);
constexpr uint64_t FLAGS_DEBUG          = (1 << 5);
constexpr uint64_t FLAGS_DIVIDE         = (1 << 6);


struct Flags {
  uint64_t DebugFlags;
};
void InstallDebugInfo();
}

#endif

struct X86InstInfo {
  char const *Name;
  InstType Type;
  uint32_t Flags; ///< Must be larger than InstFlags enum
  uint8_t MoreBytes;
  OpDispatchPtr OpcodeDispatcher;
#ifndef NDEBUG
  X86InstDebugInfo::Flags DebugInfo;
  uint32_t NumUnitTestsGenerated;
#endif

  bool operator==(const X86InstInfo &b) const {
    if (strcmp(Name, b.Name) != 0 ||
        Type != b.Type ||
        Flags != b.Flags ||
        MoreBytes != b.MoreBytes)
      return false;

    // We don't care if the opcode dispatcher differs
    return true;
  }
};

static_assert(std::is_pod<X86InstInfo>::value, "Pod?");

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
// 3 prefixes | 8 bit opcode
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

// XOP
// group (2 bits for now) | vex.pp (2 bits) | opcode (8bit)
constexpr size_t MAX_XOP_TABLE_SIZE = (1 << 13);

// XOP group ops
// group select (2 bits for now) | modrm opcode (3 bits)
constexpr size_t MAX_XOP_GROUP_TABLE_SIZE = (1 << 6);

constexpr size_t MAX_EVEX_TABLE_SIZE = 256;

extern X86InstInfo BaseOps[MAX_PRIMARY_TABLE_SIZE];
extern X86InstInfo SecondBaseOps[MAX_SECOND_TABLE_SIZE];
extern X86InstInfo RepModOps[MAX_REP_MOD_TABLE_SIZE];
extern X86InstInfo RepNEModOps[MAX_REPNE_MOD_TABLE_SIZE];
extern X86InstInfo OpSizeModOps[MAX_OPSIZE_MOD_TABLE_SIZE];
extern X86InstInfo PrimaryInstGroupOps[MAX_INST_GROUP_TABLE_SIZE];
extern X86InstInfo SecondInstGroupOps[MAX_INST_SECOND_GROUP_TABLE_SIZE];
extern X86InstInfo SecondModRMTableOps[MAX_SECOND_MODRM_TABLE_SIZE];
extern X86InstInfo X87Ops[MAX_X87_TABLE_SIZE];
extern X86InstInfo DDDNowOps[MAX_3DNOW_TABLE_SIZE];
extern X86InstInfo H0F38TableOps[MAX_0F_38_TABLE_SIZE];
extern X86InstInfo H0F3ATableOps[MAX_0F_3A_TABLE_SIZE];

// VEX
extern X86InstInfo VEXTableOps[MAX_VEX_TABLE_SIZE];
extern X86InstInfo VEXTableGroupOps[MAX_VEX_GROUP_TABLE_SIZE];

// XOP
extern X86InstInfo XOPTableOps[MAX_XOP_TABLE_SIZE];
extern X86InstInfo XOPTableGroupOps[MAX_XOP_GROUP_TABLE_SIZE];

// EVEX
extern X86InstInfo EVEXTableOps[MAX_EVEX_TABLE_SIZE];

void InitializeInfoTables(Context::OperatingMode Mode);
}
