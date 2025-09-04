// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/EnumUtils.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/MathUtils.h>
#include <FEXCore/fextl/vector.h>

#include <FEXHeaderUtils/BitUtils.h>
#include <CodeEmitter/Buffer.h>
#include <CodeEmitter/Registers.h>

#include <array>
#include <bit>
#include <cstdint>
#include <utility>
#include <type_traits>

/*
 * Welcome to FEX-Emu's custom AArch64 emitter.
 * This was written specifically to avoid the performance cost of the vixl emitter.
 *
 * There are some specific design constraints in this design to target a couple features:
 *   - High performance
 *   - Low CPU cache performance hit
 *   - Significantly reduced code footprint
 *   - Low number of branches
 *
 * These requirements are mostly achieved by removing a bunch of developer conveniences
 * that vixl provides. The developer needs to take a lot of care to not shoot themselves in the foot.
 *
 * Misc design decisions:
 * - Registers are encoded as basic uint32_t enums.
 *   - Converting between different registers is zero-cost.
 *   - Passing around as arguments are as cheap as registers
 *     - Contrast to vixl where every register requires living on the stack.
 *   - Registers can get encoded in to instructions with a simple `BFM` instruction.
 *
 * - Instructions are very simply emitted, allowing direct inlining most of the time.
 *   - These are simple enough that multiple back-to-back instructions get optimized to 128-bit load-store operations.
 *     - Contrast to vixl where pretty much no instruction emitter gets inlined.
 *
 * - Instruction emitters are /mostly/ unsized. Most instructions take a size argument first, which gets encoded
 *   directly in to the instruction.
 *   - Contrast to vixl where the register arguments are how the instructions determine operating size.
 *   - Size argument allows FEX to use `CSEL` to select a size at runtime, instead of branching.
 *   - Some instructions are explicitly sized based on register type. Read comments in the respective `inl` files to
 *     see why.
 *     Some scalar/vector operations are an example of this.
 *
 * - Almost zero helper functions.
 *   - Primary exception to this rule is load-store operations. These will use a helper to make
 *     it easier to select the correct load-store instruction. Mostly because these are a nightmare selecting
 *     the right instruction.
 */
namespace ARMEmitter {
/*
 * This `Size` enum is used for most ALU operations.
 * These follow the AArch64 encoding style in most cases.
 */
enum class Size : uint32_t {
  i32Bit = 0,
  i64Bit,
};

// This allows us to get the `Size` enum in bits.
[[nodiscard]]
constexpr size_t RegSizeInBits(Size size) {
  return size_t {32} << FEXCore::ToUnderlying(size);
}

/* This `SubRegSize` enum is used for most ASIMD operations.
 * These follow the AArch64 encoding style in most cases.
 */
enum class SubRegSize : uint32_t {
  i8Bit = 0b00,
  i16Bit = 0b01,
  i32Bit = 0b10,
  i64Bit = 0b11,
  i128Bit = 0b100,
};

// This allows us to get the `SubRegSize` in bits.
[[nodiscard]]
constexpr size_t SubRegSizeInBits(SubRegSize size) {
  return size_t {8} << FEXCore::ToUnderlying(size);
}

// Many floating point operations constrain their element sizes to the
// main three float sizes half, single, and double precision. This just
// combines all the checks together for brevity.
[[nodiscard]]
constexpr bool IsStandardFloatSize(SubRegSize size) {
  return size == SubRegSize::i16Bit || size == SubRegSize::i32Bit || size == SubRegSize::i64Bit;
}

/* This `ScalarRegSize` enum is used for most scalar float
 * operations.
 *
 * This is specifically duplicated from `SubRegSize` to have strongly
 * typed functions.
 *
 * `ScalarRegSize` specifically doesn't have `i128Bit` because scalar operations
 * can't operate at 128-bit.
 */
enum class ScalarRegSize : uint32_t {
  i8Bit = 0b00,
  i16Bit = 0b01,
  i32Bit = 0b10,
  i64Bit = 0b11,
};

// This allows us to get the `ScalarRegSize` in bits.
[[nodiscard]]
constexpr size_t ScalarRegSizeInBits(ScalarRegSize size) {
  return size_t {8} << FEXCore::ToUnderlying(size);
}

/* This `VectorRegSizePair` union allows us to have an overlapping type
 * to select a scalar operation or a vector depending on which operation
 * we pass in.
 * Useful in FEX's vector operations that behave as scalar or vector
 * depending on various factors. But since the operation will have the sa,e
 * element size, we want to choose the operation more easily
 */
union VectorRegSizePair {
  ScalarRegSize Scalar;
  SubRegSize Vector;
};

// This allows us to create a `VectorRegSizePair` union.
[[nodiscard]]
constexpr VectorRegSizePair ToVectorSizePair(SubRegSize size) {
  return VectorRegSizePair {.Vector = size};
}
[[nodiscard]]
constexpr VectorRegSizePair ToVectorSizePair(ScalarRegSize size) {
  return VectorRegSizePair {.Scalar = size};
}

// This `ShiftType` enum is used for ALU shift-register encoded instructions.
enum class ShiftType : uint32_t {
  LSL = 0,
  LSR,
  ASR,
  ROR,
};

// This `ExtendedType` enum is used for ALU extended-register encoded instructions.
enum class ExtendedType : uint32_t {
  UXTB = 0b000,
  UXTH = 0b001,
  UXTW = 0b010,
  UXTX = 0b011,
  SXTB = 0b100,
  SXTH = 0b101,
  SXTW = 0b110,
  SXTX = 0b111,
  LSL_32 = UXTW,
  LSL_64 = UXTX,
};

// This `Condition` enum is used for various conditional instructions.
enum class Condition : uint32_t {
  // Meaning:   Int                    - Float
  CC_EQ = 0, // Equal                  - Equal
  CC_NE,     // Not Eq                 - Not Eq or unordered
  CC_CS,     // Carry set              - Greater than, equal, or unordered
  CC_CC,     // Carry clear            - Less than
  CC_MI,     // Minus/Negative         - Less than
  CC_PL,     // Plus, positive or zero - GT, equal, or unordered
  CC_VS,     // Overflow               - Unordered
  CC_VC,     // No Overflow            - Ordered
  CC_HI,     // Unsigned higher        - GT, or unordered
  CC_LS,     // Unsigned lower or same - LT or EQ
  CC_GE,     // Signed GT or EQ        - GT or EQ
  CC_LT,     // Signed LT              - LT or Unordered
  CC_GT,     // Signed GT              - GT
  CC_LE,     // Signed LT or EQ        - LT, EQ, or Unordered
  CC_AL,     // Always                 - Always
  CC_NV,     // Always                 - Always

  // Aliases
  CC_HS = CC_CS,
  CC_LO = CC_CC,
};

/*
 * This `StatusFlags` enum is used for conditional compare encoded instructions.
 * These directly encode to the `nzcv` flags.
 */
enum class StatusFlags : uint32_t {
  None = 0,
  Flag_V = 0b0001,
  Flag_C = 0b0010,
  Flag_Z = 0b0100,
  Flag_N = 0b1000,

  Flag_NZCV = Flag_N | Flag_Z | Flag_C | Flag_V,
};


/*
 * This `IndexType` enum is used for load-store instructions.
 * Not all load-store instructions use this, so the user needs to be careful.
 */
enum class IndexType {
  POST,
  OFFSET,
  PRE,

  UNPRIVILEGED,
};

// Used with adr and scalar + vector load/store variants to denote
// a modifier operation.
enum class SVEModType : uint8_t {
  MOD_UXTW,
  MOD_SXTW,
  MOD_LSL,
  MOD_NONE,
};

/* This `SVEMemOperand` class is used for the helper SVE load-store instructions.
 * Load-store instructions are quite expressive, so having a helper that handles these differences is worth it.
 */
class SVEMemOperand final {
public:
  enum class Type {
    ScalarPlusScalar,
    ScalarPlusImm,
    ScalarPlusVector,
    VectorPlusImm,
  };

  SVEMemOperand(XRegister rn, XRegister rm = XReg::zr)
    : rn {rn}
    , MemType {Type::ScalarPlusScalar}
    , MetaType {.ScalarScalarType {
        .rm = rm,
      }} {}
  SVEMemOperand(XRegister rn, int32_t imm = 0)
    : rn {rn}
    , MemType {Type::ScalarPlusImm}
    , MetaType {.ScalarImmType {
        .Imm = imm,
      }} {}
  SVEMemOperand(XRegister rn, ZRegister zm, SVEModType mod = SVEModType::MOD_NONE, uint8_t scale = 0)
    : rn {rn}
    , MemType {Type::ScalarPlusVector}
    , MetaType {.ScalarVectorType {
        .zm = zm,
        .mod = mod,
        .scale = scale,
      }} {}
  SVEMemOperand(ZRegister zn, uint32_t imm)
    : rn {Register {zn.Idx()}}
    , MemType {Type::VectorPlusImm}
    , MetaType {.VectorImmType {
        .Imm = imm,
      }} {}

  [[nodiscard]]
  bool IsScalarPlusScalar() const {
    return MemType == Type::ScalarPlusScalar;
  }
  [[nodiscard]]
  bool IsScalarPlusImm() const {
    return MemType == Type::ScalarPlusImm;
  }
  [[nodiscard]]
  bool IsScalarPlusVector() const {
    return MemType == Type::ScalarPlusVector;
  }
  [[nodiscard]]
  bool IsVectorPlusImm() const {
    return MemType == Type::VectorPlusImm;
  }

  union Data {
    struct {
      Register rm;
    } ScalarScalarType;

    struct {
      int32_t Imm;
    } ScalarImmType;

    struct {
      ZRegister zm;
      SVEModType mod;
      uint8_t scale;
    } ScalarVectorType;

    struct {
      // rn will be a ZRegister
      uint32_t Imm;
    } VectorImmType;
  };

  Register rn;
  Type MemType;
  Data MetaType;
};

/* This `ExtendedMemOperand` class is used for the helper load-store instructions.
 * Load-store instructions are quite expressive, so having a helper that handles these differences is worth it.
 */
class ExtendedMemOperand final {
public:
  ExtendedMemOperand(XRegister rn, XRegister rm = XReg::zr, ExtendedType Option = ExtendedType::LSL_64, uint32_t Shift = 0)
    : rn {rn}
    , MetaType {.ExtendedType {
        .Header = {.MemType = TYPE_EXTENDED},
        .rm = rm,
        .Option = Option,
        .Shift = Shift,
      }} {}
  ExtendedMemOperand(XRegister rn, IndexType Index = IndexType::OFFSET, int32_t Imm = 0)
    : rn {rn}
    , MetaType {.ImmType {
        .Header = {.MemType = TYPE_IMM},
        .Index = Index,
        .Imm = Imm,
      }} {}

  Register rn;
  enum Type {
    TYPE_EXTENDED,
    TYPE_IMM,
  };
  struct HeaderStruct {
    Type MemType;
  };
  union {
    HeaderStruct Header;
    struct {
      HeaderStruct Header;
      Register rm;
      ExtendedType Option;
      uint32_t Shift;
    } ExtendedType;
    struct {
      HeaderStruct Header;
      IndexType Index;
      int32_t Imm;
    } ImmType;
  } MetaType;
};

template<uint32_t op0, uint32_t op1, uint32_t CRn, uint32_t CRm, uint32_t op2>
inline constexpr uint32_t GenSystemReg = op0 << 19 | op1 << 16 | CRn << 12 | CRm << 8 | op2 << 5;

// This `SystemRegister` enum is used for the mrs/msr instructions.
enum class SystemRegister : uint32_t {
  CTR_EL0 = GenSystemReg<0b11, 0b011, 0b0000, 0b0000, 0b001>,
  DCZID_EL0 = GenSystemReg<0b11, 0b011, 0b0000, 0b0000, 0b111>,
  TPIDR_EL0 = GenSystemReg<0b11, 0b011, 0b1101, 0b0000, 0b010>,
  RNDR = GenSystemReg<0b11, 0b011, 0b0010, 0b0100, 0b000>,
  RNDRRS = GenSystemReg<0b11, 0b011, 0b0010, 0b0100, 0b001>,
  NZCV = GenSystemReg<0b11, 0b011, 0b0100, 0b0010, 0b000>,
  FPCR = GenSystemReg<0b11, 0b011, 0b0100, 0b0100, 0b000>,
  TPIDRRO_EL0 = GenSystemReg<0b11, 0b011, 0b1101, 0b0000, 0b011>,
  CNTFRQ_EL0 = GenSystemReg<0b11, 0b011, 0b1110, 0b0000, 0b000>,
  CNTVCT_EL0 = GenSystemReg<0b11, 0b011, 0b1110, 0b0000, 0b010>,
  CNTVCTSS_EL0 = GenSystemReg<0b11, 0b011, 0b1110, 0b0000, 0b110>,
};

template<uint32_t op1, uint32_t CRm, uint32_t op2>
inline constexpr uint32_t GenDCReg = op1 << 16 | CRm << 8 | op2 << 5;

// This `DataCacheOperation` enum is used for the dc instruction.
enum class DataCacheOperation : uint32_t {
  IVAC = GenDCReg<0b000, 0b0110, 0b001>,
  ISW = GenDCReg<0b000, 0b0110, 0b010>,
  CSW = GenDCReg<0b000, 0b1010, 0b010>,
  CISW = GenDCReg<0b000, 0b1110, 0b010>,
  ZVA = GenDCReg<0b011, 0b0100, 0b001>,
  CVAC = GenDCReg<0b011, 0b1010, 0b001>,
  CVAU = GenDCReg<0b011, 0b1011, 0b001>,
  CIVAC = GenDCReg<0b011, 0b1110, 0b001>,

  // MTE2
  IGVAC = GenDCReg<0b000, 0b0110, 0b011>,
  IGSW = GenDCReg<0b000, 0b0110, 0b100>,
  IGDVAC = GenDCReg<0b000, 0b0110, 0b101>,
  IGDSW = GenDCReg<0b000, 0b0110, 0b110>,
  CGSW = GenDCReg<0b000, 0b1010, 0b100>,
  CGDSW = GenDCReg<0b000, 0b1010, 0b110>,
  CIGSW = GenDCReg<0b000, 0b1110, 0b100>,
  CIGDSW = GenDCReg<0b000, 0b1110, 0b110>,

  // MTE
  GVA = GenDCReg<0b011, 0b0100, 0b011>,
  GZVA = GenDCReg<0b011, 0b0100, 0b100>,
  CGVAC = GenDCReg<0b011, 0b1010, 0b011>,
  CGDVAC = GenDCReg<0b011, 0b1010, 0b101>,
  CGVAP = GenDCReg<0b011, 0b1100, 0b011>,
  CGDVAP = GenDCReg<0b011, 0b1100, 0b101>,
  CGVADP = GenDCReg<0b011, 0b1101, 0b011>,
  CGDVADP = GenDCReg<0b011, 0b1101, 0b101>,
  CIGVAC = GenDCReg<0b011, 0b1110, 0b011>,
  CIGDVAC = GenDCReg<0b011, 0b1110, 0b101>,

  // DPB
  CVAP = GenDCReg<0b011, 0b1100, 0b001>,

  // DPB2
  CVADP = GenDCReg<0b011, 0b1101, 0b001>,
};

template<uint32_t CRm, uint32_t op2>
inline constexpr uint32_t GenHintBarrierReg = CRm << 8 | op2 << 5;

// This `HintRegister` enum is used for the hint instruction.
enum class HintRegister : uint32_t {
  NOP = GenHintBarrierReg<0b0000, 0b000>,
  YIELD = GenHintBarrierReg<0b0000, 0b001>,
  WFE = GenHintBarrierReg<0b0000, 0b010>,
  WFI = GenHintBarrierReg<0b0000, 0b011>,
  SEV = GenHintBarrierReg<0b0000, 0b100>,
  SEVL = GenHintBarrierReg<0b0000, 0b101>,
  DGH = GenHintBarrierReg<0b0000, 0b110>,
  CSDB = GenHintBarrierReg<0b0010, 0b100>,
};

// This `BarrierRegister` enum is used for the various barrier instructions.
enum class BarrierRegister : uint32_t {
  CLREX = GenHintBarrierReg<0b0000, 0b010>,
  TCOMMIT = GenHintBarrierReg<0b0000, 0b011>,
  DSB = GenHintBarrierReg<0b0000, 0b100>,
  DMB = GenHintBarrierReg<0b0000, 0b101>,
  ISB = GenHintBarrierReg<0b0000, 0b110>,
  SB = GenHintBarrierReg<0b0000, 0b111>,
};

// This `BarrierScope` enum is used for the dsb/dmb instructions.
enum class BarrierScope : uint32_t {
  // Outer shareable
  OSHLD = 0b0001,
  OSHST = 0b0010,
  OSH = 0b0011,
  // Non shareable
  NSHLD = 0b0101,
  NSHST = 0b0110,
  NSH = 0b0111,
  // Inner shareable
  ISHLD = 0b1001,
  ISHST = 0b1010,
  ISH = 0b1011,
  // Full System visibility
  LD = 0b1101,
  ST = 0b1110,
  SY = 0b1111,
};

// This `Prefetch` enum is used for prefetch instructions.
enum class Prefetch : uint32_t {
  // Prefetch for load
  PLDL1KEEP = 0b00000,
  PLDL1STRM = 0b00001,
  PLDL2KEEP = 0b00010,
  PLDL2STRM = 0b00011,
  PLDL3KEEP = 0b00100,
  PLDL3STRM = 0b00101,

  // Preload instructions
  PLIL1KEEP = 0b01000,
  PLIL1STRM = 0b01001,
  PLIL2KEEP = 0b01010,
  PLIL2STRM = 0b01011,
  PLIL3KEEP = 0b01100,
  PLIL3STRM = 0b01101,

  // Preload for store
  PSTL1KEEP = 0b10000,
  PSTL1STRM = 0b10001,
  PSTL2KEEP = 0b10010,
  PSTL2STRM = 0b10011,
  PSTL3KEEP = 0b10100,
  PSTL3STRM = 0b10101,
};

// This `PredicatePattern` enun is used for some SVE instructions.
enum class PredicatePattern : uint32_t {
  SVE_POW2 = 0b00000,
  SVE_VL1 = 0b00001,
  SVE_VL2 = 0b00010,
  SVE_VL3 = 0b00011,
  SVE_VL4 = 0b00100,
  SVE_VL5 = 0b00101,
  SVE_VL6 = 0b00110,
  SVE_VL7 = 0b00111,
  SVE_VL8 = 0b01000,
  SVE_VL16 = 0b01001,
  SVE_VL32 = 0b01010,
  SVE_VL64 = 0b01011,
  SVE_VL128 = 0b01100,
  SVE_VL256 = 0b01101,
  SVE_MUL4 = 0b11101,
  SVE_MUL3 = 0b11110,
  SVE_ALL = 0b11111,
};

// Used with SVE FP immediate arithmetic instructions
enum class SVEFAddSubImm : uint32_t {
  _0_5,
  _1_0,
};
enum class SVEFMulImm : uint32_t {
  _0_5,
  _2_0,
};
enum class SVEFMaxMinImm : uint32_t {
  _0_0,
  _1_0,
};

/* This `BackwardLabel` struct is used for retaining a location for PC-Relative instructions.
 * This is specifically a label for a target that is logically `below` an instruction that uses it.
 * Which means that a branch would jump backwards.
 */
struct BackwardLabel {
  uint8_t* Location {};
};

/* This `ForwardLabel` struct is used for retaining a location for PC-Relative instructions.
 * This is specifically a label for a target that is logically `above` an instruction that uses it.
 * Which means that a branch would jump forwards.
 */
struct ForwardLabel {
  enum class InstType {
    UNKNOWN,
    ADR,
    ADRP,
    B,
    BC,
    TEST_BRANCH,
    RELATIVE_LOAD,
    LONG_ADDRESS_GEN,
  };

  struct Reference {
    uint8_t* Location {};
    InstType Type = InstType::UNKNOWN;
  };

  // The first element is stored separately to avoid allocations for simple cases
  Reference FirstInst;

  fextl::vector<Reference> Insts;
};

/* This `BiDirectionalLabel` struct used for retaining a location for PC-Relative instructions.
 * This is specifically a label for a target that is in either direction of an instruction that uses it.
 * Which means a branch could jump backwards or forwards depending on situation.
 */
struct BiDirectionalLabel {
  BackwardLabel Backward;
  ForwardLabel Forward;
};

static inline void AddLocationToLabel(ForwardLabel* Label, ForwardLabel::Reference&& Location) {
  if (Label->FirstInst.Location == nullptr) {
    Label->FirstInst = Location;
  } else {
    Label->Insts.push_back(Location);
  }
}

// Some FCMA ASIMD instructions support a rotation argument.
enum class Rotation : uint32_t {
  ROTATE_0 = 0b00,
  ROTATE_90 = 0b01,
  ROTATE_180 = 0b10,
  ROTATE_270 = 0b11,
};

// Concept for contraining some instructions to accept only an XRegister or WRegister.
// Particularly for operations that differ encodings depending on which one is used.
template<typename T>
concept IsXOrWRegister = std::is_same_v<T, XRegister> || std::is_same_v<T, WRegister>;

// Concept for contraining some instructions to accept only a QRegister or DRegister.
template<typename T>
concept IsQOrDRegister = std::is_same_v<T, QRegister> || std::is_same_v<T, DRegister>;

// Whether or not a given set of vector registers are sequential
// in increasing order as far as the register file is concerned (modulo its size)
//
// For example, a set of registers like:
//
// v1,  v2, v3 and
// v31, v0, v1
//
// would both be considered sequential sequences, and some instructions in particular
// limit register lists to these kind of sequences.
//
template<typename T, typename... Args>
constexpr bool AreVectorsSequential(T first, const Args&... args) {
  // Ensure we always have a pair of registers to compare against.
  static_assert(sizeof...(args) >= 1, "Number of arguments must be greater than 1");

  const auto fn = [](auto& lhs, const auto& rhs) {
    const auto result = ((lhs.Idx() + 1) % 32) == rhs.Idx();
    lhs = rhs;
    return result;
  };

  return (fn(first, args) && ...);
}

// Returns if the immediate can fit in to add/sub immediate instruction encodings.
constexpr bool IsImmAddSub(uint64_t imm) {
  constexpr uint64_t U12Mask = 0xFFF;
  auto FitsWithin12Bits = [](uint64_t imm) {
    return (imm & ~U12Mask) == 0;
  };
  // Can fit in to the instruction encoding:
  // - if only bits [11:0] are set.
  // - if only bits [23:12] are set.
  return FitsWithin12Bits(imm) || (FitsWithin12Bits(imm >> 12) && (imm & U12Mask) == 0);
}

// This is an emitter that is designed around the smallest code bloat as possible.
// Eschewing most developer convenience in order to keep code as small as possible.

// Choices:
// - Size of ops passed as an argument rather than template to let the compiler use csel instead of branching.
// - Registers are unsized so they can be passed in a GPR and not need conversion operations
class Emitter : public ARMEmitter::Buffer {
public:
  Emitter() = default;

  Emitter(uint8_t* Base, uint64_t BaseSize)
    : Buffer(Base, BaseSize) {}

  // Bind a backward label to an address.
  // Address that is bound is the current emitter location.
  void Bind(BackwardLabel* Label) {
    LOGMAN_THROW_A_FMT(Label->Location == nullptr, "Trying to bind a label twice");
    Label->Location = GetCursorAddress<uint8_t*>();
  }

  void Bind(const ForwardLabel::Reference* Label) {
    uint8_t* CurrentAddress = GetCursorAddress<uint8_t*>();
    // Patch up the instructions
    switch (Label->Type) {
    case ForwardLabel::InstType::ADR: {
      uint32_t* Instruction = reinterpret_cast<uint32_t*>(Label->Location);
      int64_t Imm = reinterpret_cast<int64_t>(CurrentAddress) - reinterpret_cast<int64_t>(Instruction);
      LOGMAN_THROW_A_FMT(IsADRRange(Imm), "Unscaled offset too large");
      uint32_t InstMask = 0b11 << 29 | 0b1111'1111'1111'1111'111 << 5;
      uint32_t Offset = static_cast<uint32_t>(Imm) & 0x3F'FFFF;
      uint32_t Inst = *Instruction & ~InstMask;
      Inst |= (Offset & 0b11) << 29;
      Inst |= (Offset >> 2) << 5;
      *Instruction = Inst;
      break;
    }
    case ForwardLabel::InstType::ADRP: {
      uint32_t* Instruction = reinterpret_cast<uint32_t*>(Label->Location);
      int64_t Imm = reinterpret_cast<int64_t>(CurrentAddress) - reinterpret_cast<int64_t>(Instruction);
      LOGMAN_THROW_A_FMT(IsADRPRange(Imm) && IsADRPAligned(Imm), "Unscaled offset too large");
      Imm >>= 12;
      uint32_t InstMask = 0b11 << 29 | 0b1111'1111'1111'1111'111 << 5;
      uint32_t Offset = static_cast<uint32_t>(Imm) & 0x3F'FFFF;
      uint32_t Inst = *Instruction & ~InstMask;
      Inst |= (Offset & 0b11) << 29;
      Inst |= (Offset >> 2) << 5;
      *Instruction = Inst;
      break;
    }

    case ForwardLabel::InstType::B: {
      uint32_t* Instruction = reinterpret_cast<uint32_t*>(Label->Location);
      int64_t Imm = reinterpret_cast<int64_t>(CurrentAddress) - reinterpret_cast<int64_t>(Instruction);
      LOGMAN_THROW_A_FMT(Imm >= -134217728 && Imm <= 134217724 && ((Imm & 0b11) == 0), "Unscaled offset too large");
      Imm >>= 2;
      uint32_t InstMask = 0x3FF'FFFF;
      uint32_t Offset = static_cast<uint32_t>(Imm) & InstMask;
      uint32_t Inst = *Instruction & ~InstMask;
      Inst |= Offset;
      *Instruction = Inst;

      break;
    }

    case ForwardLabel::InstType::TEST_BRANCH: {
      uint32_t* Instruction = reinterpret_cast<uint32_t*>(Label->Location);
      int64_t Imm = reinterpret_cast<int64_t>(CurrentAddress) - reinterpret_cast<int64_t>(Instruction);
      LOGMAN_THROW_A_FMT(Imm >= -32768 && Imm <= 32764 && ((Imm & 0b11) == 0), "Unscaled offset too large");
      Imm >>= 2;
      uint32_t InstMask = 0x3FFF;
      uint32_t Offset = static_cast<uint32_t>(Imm) & InstMask;
      uint32_t Inst = *Instruction & ~(InstMask << 5);
      Inst |= Offset << 5;
      *Instruction = Inst;

      break;
    }
    case ForwardLabel::InstType::BC:
    case ForwardLabel::InstType::RELATIVE_LOAD: {
      uint32_t* Instruction = reinterpret_cast<uint32_t*>(Label->Location);
      int64_t Imm = reinterpret_cast<int64_t>(CurrentAddress) - reinterpret_cast<int64_t>(Instruction);
      LOGMAN_THROW_A_FMT(Imm >= -1048576 && Imm <= 1048575 && ((Imm & 0b11) == 0), "Unscaled offset too large");
      Imm >>= 2;
      uint32_t InstMask = 0x7'FFFF;
      uint32_t Offset = static_cast<uint32_t>(Imm) & InstMask;
      uint32_t Inst = *Instruction & ~(InstMask << 5);
      Inst |= Offset << 5;
      *Instruction = Inst;
      break;
    }
    case ForwardLabel::InstType::LONG_ADDRESS_GEN: {
      uint32_t* Instructions = reinterpret_cast<uint32_t*>(Label->Location);
      int64_t ImmInstOne = reinterpret_cast<int64_t>(CurrentAddress) - reinterpret_cast<int64_t>(&Instructions[0]);
      int64_t ImmInstTwo = reinterpret_cast<int64_t>(CurrentAddress) - reinterpret_cast<int64_t>(&Instructions[1]);
      auto OriginalOffset = GetCursorOffset();

      auto InstOffset = GetCursorOffsetFromAddress(Instructions);
      SetCursorOffset(InstOffset);

      // We encoded the destination register in to the first instruction space.
      // Read it back.
      ARMEmitter::Register DestReg(Instructions[0]);

      if (IsADRRange(ImmInstTwo)) {
        // If within ADR range from the second instruction, then we can emit NOP+ADR
        nop();
        adr(DestReg, static_cast<uint32_t>(ImmInstTwo) & 0x7FFF);
      } else if (IsADRPRange(ImmInstOne)) {

        // If within ADRP range from the first instruction, then we are /definitely/ in range for the second instruction.
        // First check if we are in non-offset range for second instruction.
        if (IsADRPAligned(reinterpret_cast<uint64_t>(CurrentAddress))) {
          // We can emit nop + adrp
          nop();
          adrp(DestReg, static_cast<uint32_t>(ImmInstTwo >> 12) & 0x7FFF);
        } else {
          // Not aligned, need adrp + add
          adrp(DestReg, static_cast<uint32_t>(ImmInstOne >> 12) & 0x7FFF);
          add(ARMEmitter::Size::i64Bit, DestReg, DestReg, ImmInstOne & 0xFFF);
        }
      } else {
        LOGMAN_MSG_A_FMT("Unscaled offset is too large");
        FEX_UNREACHABLE;
      }

      SetCursorOffset(OriginalOffset);
      break;
    }
    default: LOGMAN_MSG_A_FMT("Unexpected inst type in label fixup");
    }
  }

  // Bind a forward label to a location.
  // This walks all the instructions in the label's vector.
  // Then backpatching all instructions that have used the label.
  void Bind(ForwardLabel* Label) {
    if (Label->FirstInst.Location) {
      Bind(&Label->FirstInst);
    }
    for (auto& Inst : Label->Insts) {
      Bind(&Inst);
    }
  }

  // Bind a bidirectional location to a location.
  // Binds both forwards and backwards depending on how the label was used.
  void Bind(BiDirectionalLabel* Label) {
    if (!Label->Backward.Location) {
      Bind(&Label->Backward);
    }
    Bind(&Label->Forward);
  }

#include <CodeEmitter/VixlUtils.inl>

public:

// This symbol is used to allow external tooling (IDEs, clang-format, ...) to process the included files individually:
// If defined, the files will inject member functions into this class.
// If not, the files will wrap the member functions in a class so that tooling will process them properly.
#define INCLUDED_BY_EMITTER

  // TODO: Implement SME when it matters.
#include <CodeEmitter/ALUOps.inl>
#include <CodeEmitter/BranchOps.inl>
#include <CodeEmitter/LoadstoreOps.inl>
#include <CodeEmitter/SystemOps.inl>
#include <CodeEmitter/ScalarOps.inl>
#include <CodeEmitter/ASIMDOps.inl>
#include <CodeEmitter/SVEOps.inl>

#undef INCLUDED_BY_EMITTER

protected:
  template<typename T>
  uint32_t Encode_ra(T Reg) const {
    return Reg.Idx() << 10;
  }
  uint32_t Encode_ra(uint32_t Reg) const {
    return Reg << 10;
  }
  template<typename T>
  uint32_t Encode_rt2(T Reg) const {
    return Reg.Idx() << 10;
  }
  uint32_t Encode_rt2(uint32_t Reg) const {
    return Reg << 10;
  }
  template<typename T>
  uint32_t Encode_rm(T Reg) const {
    return Reg.Idx() << 16;
  }
  uint32_t Encode_rm(uint32_t Reg) const {
    return Reg << 16;
  }
  template<typename T>
  uint32_t Encode_rs(T Reg) const {
    return Reg.Idx() << 16;
  }
  uint32_t Encode_rs(uint32_t Reg) const {
    return Reg << 16;
  }
  template<typename T>
  uint32_t Encode_rn(T Reg) const {
    return Reg.Idx() << 5;
  }
  uint32_t Encode_rn(uint32_t Reg) const {
    return Reg << 5;
  }
  template<typename T>
  uint32_t Encode_rd(T Reg) const {
    return Reg.Idx();
  }
  uint32_t Encode_rd(uint32_t Reg) const {
    return Reg;
  }
  template<typename T>
  uint32_t Encode_rt(T Reg) const {
    return Reg.Idx();
  }
  uint32_t Encode_rt(Prefetch Reg) const {
    return FEXCore::ToUnderlying(Reg);
  }
  uint32_t Encode_rt(uint32_t Reg) const {
    return Reg;
  }
  template<typename T>
  uint32_t Encode_pd(T Reg) const {
    return FEXCore::ToUnderlying(Reg);
  }
};
} // namespace ARMEmitter
